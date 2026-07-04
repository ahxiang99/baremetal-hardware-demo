#include <atomic>

#include "AppMode.hpp"
#include "FreeRTOS.h"
#include "Middleware/cli.hpp"
#include "Middleware/logger.hpp"
#include "RingBuffer.hpp"
#include "SensorPacket.hpp"
#include "Sht40ad1b.hpp"
#include "cpp/Dma.hpp"
#include "cpp/ExtiInput.hpp"
#include "cpp/II2C.hpp"
#include "cpp/Stm32GpioPin.hpp"
#include "cpp/Stm32I2C.hpp"
#include "cpp/UartConcepts.hpp"
#include "cpp/UartRef.hpp"
#include "drivers.hpp"
#include "low-level/nvic.h"
#include "low-level/rcc_bitfields.h"
#include "low-level/syscfg_registers.h"
#include "pch.hpp"
#include "portmacro.h"
#include "projdefs.h"
#include "semphr.h"
#include "stts2h.hpp"
#include "task.h"

constexpr bool            kSensorEnable = true;

static volatile uint32_t& CPACR         = *reinterpret_cast<volatile uint32_t*>(0xE000ED88);

// Global Variables
Sht40ad1b            temp_sensor(I2C_Ref::from(getDrivers().i2c1), "SHT40");
Stts2h               stts_temp(I2C_Ref::from(getDrivers().i2c1), Stts2h::SensorMode::ONE_SHOT);
Cli                  cmd;
std::atomic_bool     g_cmd_complete{true};
std::atomic<AppMode> g_AppMode{AppMode::Console};

/* Function Prototype */
void Init_Driver(Drivers& g);
void SetNVICPriority();
void RegisterCallback();

void ledTask(void* params) {
    Drivers&         g             = getDrivers();
    TickType_t       xLastWakeTime = xTaskGetTickCount();
    const TickType_t xPeriod       = pdMS_TO_TICKS(100);
    while (1) {
        g.gpio_led.Toggle();
        vTaskDelayUntil(&xLastWakeTime, xPeriod);  // replaces g.timer.start(100)
    }
}

void sht40_Task(void* params) {
    SemaphoreHandle_t       mutex         = static_cast<SemaphoreHandle_t>(params);
    Drivers&                g             = getDrivers();
    TickType_t              xLastWakeTime = xTaskGetTickCount();
    static std::atomic_bool cmd_complete{true};

    while (1) {
        xSemaphoreTake(mutex, portMAX_DELAY);  // <- This Line Step Over, Go to Hardfault
        g.i2c1.complete_flag_ = &cmd_complete;
        g_cmd_complete.store(false, std::memory_order_relaxed);

        if (g_AppMode.load(std::memory_order_relaxed) != AppMode::CliMode) {
            temp_sensor.read();
        }

        while (!temp_sensor.isIdle()) {
            g.i2c1.processRx();
            temp_sensor.ProcessData();
            vTaskDelay(pdMS_TO_TICKS(10));
        }

        xSemaphoreGive(mutex);

        const AppMode curMode = g_AppMode.load(std::memory_order_relaxed);
        switch (curMode) {
            case AppMode::SendPacket: {
                PacketV2<Sht40ad1b::SensorData, PacketType::VERSION_1> sht40_data{temp_sensor.getValue()};
                g.uart2.send(sht40_data.raw(), sht40_data.size());
                break;
            }
            case AppMode::Console:
                if (stts_temp.getState() == Stts2h::SensorState::IDLE) {
                    LOG_PRINT("STH40: Temp:{}, Rh:{}", temp_sensor.getValue().temperature, temp_sensor.getValue().humidity);
                }
                break;
            case AppMode::CliMode:
                if (cmd.getState() == CliState::Completed) {
                    LOG_PRINT("STH40: Temp:{}, Rh:{}", temp_sensor.getValue().temperature, temp_sensor.getValue().humidity);
                    cmd.setState(CliState::WaitingForInput);
                }
                break;
            case AppMode::COUNT:
                break;
        }

        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(300));
    }
}

void stts2h_Task(void* params) {
    SemaphoreHandle_t       mutex         = static_cast<SemaphoreHandle_t>(params);
    Drivers&                g             = getDrivers();
    TickType_t              xLastWakeTime = xTaskGetTickCount();
    static std::atomic_bool cmd_complete{true};

    while (1) {
        xSemaphoreTake(mutex, portMAX_DELAY);

        g.i2c1.complete_flag_ = &cmd_complete;
        g_cmd_complete.store(false, std::memory_order_relaxed);

        if (g_AppMode.load(std::memory_order_relaxed) != AppMode::CliMode) {
            stts_temp.read();
        }

        while (!stts_temp.isIdle()) {
            g.i2c1.processRx();
            stts_temp.processData();
            vTaskDelay(pdMS_TO_TICKS(10));
        }

        xSemaphoreGive(mutex);
        const AppMode curMode = g_AppMode.load(std::memory_order_relaxed);
        switch (curMode) {
            case AppMode::SendPacket: {
                PacketV2<float_t, PacketType::VERSION_2> stts2h_data{stts_temp.getTemp()};
                g.uart2.send(stts2h_data.raw(), stts2h_data.size());
                break;
            }
            case AppMode::Console:
                if (stts_temp.getState() == Stts2h::SensorState::IDLE) {
                    LOG_PRINT("STTS2H: Temp:{}", stts_temp.getTemp());
                }
                break;
            case AppMode::CliMode:
                if (cmd.getState() == CliState::Completed) {
                    LOG_PRINT("STTS2H: Temp:{}", stts_temp.getTemp());
                    cmd.setState(CliState::WaitingForInput);
                }
                break;

            case AppMode::COUNT:
                break;
        }

        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(1000));
    }
}

// Task 1 — sensorTask: owns I2C + sensor reading + packet sending

void sensorTask(void* params) {
    Drivers&                      g = getDrivers();
    SpscRingBuffer<I2CCommand, 4> cmd_queue;
    while (1) {
        /* Timer to keep firing read command when it is elapsed.*/
        if (g_AppMode.load(std::memory_order_relaxed) != AppMode::CliMode) {
            cmd_queue.push({[](void* ctx) { static_cast<Sht40ad1b*>(ctx)->read(); }, &temp_sensor});
            cmd_queue.push({[](void* ctx) { static_cast<Stts2h*>(ctx)->read(); }, &stts_temp});
        }

        /* Command Queue */
        I2CCommand icmd;
        while (cmd_queue.pop(icmd)) {
            g_cmd_complete.store(false, std::memory_order_relaxed);
            getDrivers().i2c1.complete_flag_ = &g_cmd_complete;
            icmd.fn(icmd.ctx);  // Read Call

            bool sensor_done = false;
            while (!sensor_done) {
                g.i2c1.processRx();
                temp_sensor.ProcessData();  // State is not update
                stts_temp.processData();
                // Both sensors idle = current command fully complete:
                bool sht40_idle  = temp_sensor.isIdle();
                bool stts2h_idle = stts_temp.isIdle();
                if (sht40_idle && stts2h_idle) break;
                // Done when complete_flag is true AND no more I2C activity:
                if (g_cmd_complete.load(std::memory_order_acquire)) {
                    sensor_done = true;
                }
                vTaskDelay(pdMS_TO_TICKS(10));  // yield while polling
            }
        }

        if constexpr (kSensorEnable) {
            /* Send data packet to PC via Uart2 */
            if (g_AppMode.load(std::memory_order_relaxed) == AppMode::SendPacket) {
                PacketV2<Sht40ad1b::SensorData, PacketType::VERSION_1> sht40_data{temp_sensor.getValue()};
                g.uart2.send(sht40_data.raw(), sht40_data.size());
                PacketV2<float_t, PacketType::VERSION_2> stts2h_data{stts_temp.getTemp()};
                g.uart2.send(stts2h_data.raw(), stts2h_data.size());
            } else if (g_AppMode.load(std::memory_order_relaxed) == AppMode::Console) {
                if (temp_sensor.getState() == Sht40ad1b::SensorState::IDLE) {
                    LOG_INFO("STH40: Temp:{}, Rh:{}", temp_sensor.getValue().temperature, temp_sensor.getValue().humidity);
                }
                if (stts_temp.getState() == Stts2h::SensorState::IDLE) {
                    LOG_INFO("STTS2H: Temp:{}", stts_temp.getTemp());
                }
            } else if (g_AppMode.load(std::memory_order_relaxed) == AppMode::CliMode) {
                if (cmd.getState() == CliState::Completed) {
                    LOG_INFO("STH40: Temp:{}, Rh:{}", temp_sensor.getValue().temperature, temp_sensor.getValue().humidity);
                    cmd.setState(CliState::WaitingForInput);
                }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(500));  // ← yield while waiting
    }
}

// Task 2 — uartTask: owns CLI input (only when kCliEnable = true)
void uartTask(void* params) {
    /* Command Line Interface Input Processing */
    while (1) {
        if (g_AppMode.load(std::memory_order_relaxed) == AppMode::CliMode) {
            Drivers& g = getDrivers();
            g.uart2.processRx();
            if (cmd.getState() == CliState::WaitingForInput) {
                cmd.get_input();
                cmd.setState(CliState::Processing);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

/* Main Program Start Here */
int main() {
    Drivers& g = getDrivers();
    Init_Driver(g);

    RegisterCallback();
    stts_temp.initialize();

    SemaphoreHandle_t i2c_mutex = xSemaphoreCreateMutex();
    if (i2c_mutex == NULL) {
        // Heap exhausted — can't create mutex
        __asm volatile("bkpt #0");
        while (1);
    }

    xTaskCreate(ledTask, "LED", 1024, NULL, 4, NULL);

    xTaskCreate(uartTask, "UART", 1024, NULL, 3, NULL);

    xTaskCreate(sht40_Task, "Sht40", 1024, i2c_mutex, 2, NULL);

    xTaskCreate(stts2h_Task, "Stts2h", 1024, i2c_mutex, 1, NULL);

    vTaskStartScheduler();
    while (1);  // heap exhaustion
    return 0;
}

/* Function Body */

void RegisterCallback() {
    getDrivers().uart2.onDataReceived([](void* ctx, const uint8_t* data, size_t len) { static_cast<Cli*>(ctx)->onUartData(data, len); }, &cmd);
    cmd.setUart(UartRef::from(getDrivers().uart2));
    cmd.setSensor(&temp_sensor);

    if constexpr (kSensorEnable) {
        getDrivers().i2c1.addReceiver(temp_sensor);
        getDrivers().i2c1.addReceiver(stts_temp);
        getDrivers().i2c1.addReceiver(cmd);
    }

    getDrivers().gpio_button.setCallback(onButtonPress, &g_AppMode);
}

void SetNVICPriority() {
    My_NVIC_SetPriority(DMA1_Stream0_IRQn, 6);  // I2C RX DMA
    My_NVIC_SetPriority(DMA1_Stream7_IRQn, 6);  // I2C TX DMA
    My_NVIC_SetPriority(DMA1_Stream6_IRQn, 6);  // UART TX DMA
    My_NVIC_SetPriority(DMA1_Stream5_IRQn, 6);  // UART RX DMA
    My_NVIC_SetPriority(USART2_IRQn, 6);        // UART
    My_NVIC_SetPriority(I2C1_EV_IRQn, 6);       // I2C event
    My_NVIC_SetPriority(I2C1_ER_IRQn, 6);       // I2C error
    My_NVIC_SetPriority(TIM3_IRQn, 6);          // Timer
    My_NVIC_SetPriority(EXTI15_10_IRQn, 7);     // Button
}

/* Interrupt Handler Function Start Here*/

extern "C" void TIM3_IRQHandler(void) {
    getDrivers().timer.handleInterrupt();
}

extern "C" void DMA1_Stream0_IRQHandler(void) {
    getDrivers().i2c1.handleRxDmaInterrupt();
}

extern "C" void DMA1_Stream7_IRQHandler(void) {
    getDrivers().i2c1.handleTxDmaInterrupt();
}

extern "C" void DMA1_Stream6_IRQHandler(void) {
    getDrivers().uart2.handleTxDmaInterrupt();
}

extern "C" void DMA1_Stream5_IRQHandler(void) {
    getDrivers().uart2.handleRxDmaInterrupt();
}

extern "C" void USART2_IRQHandler(void) {
    getDrivers().uart2.handleInterrupt();
}

extern "C" void I2C1_EV_IRQHandler(void) {
    getDrivers().i2c1.handleEVInterrupt();
}

extern "C" void I2C1_ER_IRQHandler(void) {
    getDrivers().i2c1.handleERInterrupt();
}

extern "C" void EXTI15_10_IRQHandler(void) {
    getDrivers().gpio_button.handleInterrupt();
}

extern "C" void HardFault_Handler(void) {
    // SCB registers — Cortex-M4 System Control Block
    volatile uint32_t hfsr = *reinterpret_cast<volatile uint32_t*>(0xE000ED2C);  // Hard Fault Status
    volatile uint32_t cfsr = *reinterpret_cast<volatile uint32_t*>(0xE000ED28);  // Configurable Fault Status
    volatile uint32_t bfar = *reinterpret_cast<volatile uint32_t*>(0xE000ED38);  // Bus Fault Address
    volatile uint32_t mmar = *reinterpret_cast<volatile uint32_t*>(0xE000ED34);  // MemManage Fault Address
    volatile uint32_t lr   = 0;
    __asm volatile("mov %0, lr" : "=r"(lr));  // link register — tells which context
    (void)hfsr;
    (void)cfsr;
    (void)bfar;
    (void)mmar;
    volatile int loop = 1;
    while (loop) {
    }
}

void Init_Driver(Drivers& g) {
    // Enable FPU by setting bits 20, 21, 22, and 23
    CPACR |= ((3UL << 20) | (3UL << 22));
    // Manual Barrier        instructions(Assembly)
    __asm volatile("dsb 0xf" ::: "memory");
    __asm volatile("isb 0xf" ::: "memory");

    SetNVICPriority();
    /* MySysTick Init*/
    g.my_systick.init();

    /* Configure Uart2 Pin */

    constexpr GPIO_Config uart2_gpio_cfg{.pin   = GPIO_PIN_2 | GPIO_PIN_3,
                                         .port  = GPIO_Port::GPIO_PA,
                                         .mode  = GPIO_Moder::GPIO_MODE_ALTFN,
                                         .otype = GPIO_OType::GPIO_OTYPER_PP,
                                         .ospdr = GPIO_OSPDR::GPIO_OSPEEDR_VHS,
                                         .pupdr = GPIO_PUPDR::GPIO_PUPDR_NOPULL,
                                         .afr   = GPIO_AFR::GPIO_AF7_USART1_2};

    constexpr UartConfig  uart2_cfg{.dev_num = UartNum::USART_D2, .baudRate = UartBaudRate::_9600, .comm = UartComm::RX_TX, .parity = UartParity::NONE, .stopbits = UartStopBit::USART_CR2_STOP_1};

    /* This DMA Config is for USART2 Tx */
    const DMA_Config hdmatx_cfg{.Peripheral      = DMA_Peripheral::USART2_TX,
                                .DMA_BaseAddress = DMA1,
                                .Stream          = DMA_Stream::Stream_6,
                                .Channel         = DMA_Channel::Channel_4,
                                .Direction       = DMA_Direction::DMA_MEMORY_TO_PERIPH,
                                .Mode            = DMA_Mode::Normal};

    /* This DMA Config is for USART2 Rx */
    const DMA_Config hdmarx_cfg{.Peripheral      = DMA_Peripheral::USART2_RX,
                                .DMA_BaseAddress = DMA1,
                                .Stream          = DMA_Stream::Stream_5,
                                .Channel         = DMA_Channel::Channel_4,
                                .Direction       = DMA_Direction::DMA_PERIPH_TO_MEMORY,
                                .Mode            = DMA_Mode::Circular};

    /* Configure i2c1 Pin */
    constexpr GPIO_Config i2c1_gpio_config{.pin   = GPIO_PIN_8 | GPIO_PIN_9,
                                           .port  = GPIO_Port::GPIO_PB,
                                           .mode  = GPIO_Moder::GPIO_MODE_ALTFN,
                                           .otype = GPIO_OType::GPIO_OTYPER_OD,
                                           .ospdr = GPIO_OSPDR::GPIO_OSPEEDR_VHS,
                                           .pupdr = GPIO_PUPDR::GPIO_PUPDR_PULLUP,
                                           .afr   = GPIO_AFR::GPIO_AF4_I2C1_3

    };

    /* Logger Init */
    Logger::Init(UartRef::from(g.uart2));
    Logger::set_level(LogLevel::INFO);

    Stm32GpioPin temp;
    Startup(temp, uart2_gpio_cfg, i2c1_gpio_config);

    /* Configure Uart */
    g.uart2.configure(uart2_cfg, hdmatx_cfg, hdmarx_cfg);
    auto result = g.uart2.initialize();
    if (!result.isOk()) {
        while (1);
    }
    LOG_INFO("Booting...");
    g.my_systick.delay_ms(10);
    LOG_INFO("USART2 Initialized");
    g.my_systick.delay_ms(10);
    constexpr I2C_Config i2c_config{
        .DevNum          = I2C_DeviceNum::I2C_D1,
        .ClockFreq       = I2C_FreqHz::_100KHz,
        .OwnAddress1     = 0,
        .AddressingMode  = I2C_Addressing_Mode::AddressMode_7Bit,
        .DualAddressMode = 0,
        .OwnAddress2     = 0,
    };
    g.i2c1.configure(i2c_config);
    g.i2c1.initialize();
    LOG_INFO("I2C1 Initialized");

    /* Blinking LED */
    constexpr GPIO_Config gpio_led_cfg{.pin   = GPIO_PIN_5,
                                       .port  = GPIO_Port::GPIO_PA,
                                       .mode  = GPIO_Moder::GPIO_MODE_OUTPUT,
                                       .otype = GPIO_OType::GPIO_OTYPER_PP,
                                       .ospdr = GPIO_OSPDR::GPIO_OSPEEDR_LS,
                                       .pupdr = GPIO_PUPDR::GPIO_PUPDR_NOPULL,
                                       .afr   = GPIO_AFR::GPIO_AF0_SYSTEM};

    g.gpio_led.Init(gpio_led_cfg);

    /* Timer */
    TimerConfig timer_cfg;
    timer_cfg.Instance          = APB1_TIMER_3;
    timer_cfg.CounterMode       = TIM_COUNTERMODE_UP;
    timer_cfg.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
    timer_cfg.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    g.timer.setVariable(TIM3, timer_cfg);

    /* Configure for Button Input */
    constexpr GPIO_Config gpio_button_cfg{.pin   = GPIO_PIN_13,
                                          .port  = GPIO_Port::GPIO_PC,
                                          .mode  = GPIO_Moder::GPIO_MODE_INPUT,
                                          .otype = GPIO_OType::GPIO_OTYPER_PP,
                                          .ospdr = GPIO_OSPDR::GPIO_OSPEEDR_LS,
                                          .pupdr = GPIO_PUPDR::GPIO_PUPDR_PULLUP,
                                          .afr   = GPIO_AFR::GPIO_AF0_SYSTEM};

    /* GPIO C Pin 13 */
    g.gpio_button.configure(gpio_button_cfg);
    g.gpio_button.initialize();

    LOG_INFO("Initialized Done...");
}