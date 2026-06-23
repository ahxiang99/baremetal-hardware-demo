#include <atomic>

#include "Middleware/cli.hpp"
#include "Middleware/logger.hpp"
#include "RingBuffer.hpp"
#include "SensorPacket.hpp"
#include "Sht40ad1b.hpp"
#include "cpp/Dma.hpp"
#include "cpp/II2C.hpp"
#include "cpp/Stm32GpioPin.hpp"
#include "cpp/Stm32I2C.hpp"
#include "cpp/Syscfg.hpp"
#include "cpp/UartConcepts.hpp"
#include "cpp/UartRef.hpp"
#include "drivers.hpp"
#include "low-level/nvic.h"
#include "low-level/rcc_bitfields.h"
#include "low-level/syscfg_registers.h"
#include "pch.hpp"
#include "stts2h.hpp"

constexpr bool            kCliEnable    = false;
constexpr bool            kSensorEnable = true;
constexpr bool            kSendPacket   = true;

static volatile uint32_t& CPACR         = *reinterpret_cast<volatile uint32_t*>(0xE000ED88);

// Global Variables
Sht40ad1b        temp_sensor(I2C_Ref::from(getDrivers().i2c1), "SHT40");
Stts2h           stts_temp(I2C_Ref::from(getDrivers().i2c1));
Cli              cmd;
std::atomic_bool g_cmd_complete{true};

/* Function Prototype */
void Init_Driver(Drivers& g);
void RegisterCallback();

/* Main Program Start Here */
int main() {
    Drivers& g = getDrivers();
    Init_Driver(g);
    RegisterCallback();

    SpscRingBuffer<I2CCommand, 4> cmd_queue;
    if (!g.timer.isRunning()) {
        g.timer.start(500);
    }

    uint32_t measure_start = g.my_systick.get_ticks();
    stts_temp.initialize();

    while (1) {
        /* Timer to keep firing read command when it is elapsed.*/
        if (g.timer.isElapsed()) {
            g.gpio_led.Toggle();
            g.timer.start(250);
            if constexpr (!kCliEnable && kSensorEnable) {
                cmd_queue.push({[](void* ctx) { static_cast<Sht40ad1b*>(ctx)->read(); }, &temp_sensor});
                cmd_queue.push({[](void* ctx) { static_cast<Stts2h*>(ctx)->read(); }, &stts_temp});
            }
        }

        /* To process i2c and Sensor Data*/
        if constexpr (kSensorEnable) {
            g.i2c1.processRx();
            temp_sensor.ProcessData();
            stts_temp.processData();

            /* Send data packet to PC via Uart2 */
            if (g.my_systick.get_ticks() - measure_start > 300) {
                if constexpr (kSendPacket) {
                    PacketV2<Sht40ad1b::SensorData, PacketType::VERSION_1> sht40_data{temp_sensor.getValue()};
                    g.uart2.send(sht40_data.raw(), sht40_data.size());
                    PacketV2<float_t, PacketType::VERSION_2> stts2h_data{stts_temp.getTemp()};
                    g.uart2.send(stts2h_data.raw(), stts2h_data.size());

                } else {
                    LOG_INFO("STH40: Temp:{}, Rh:{}", temp_sensor.getValue().temperature, temp_sensor.getValue().humidity);
                    LOG_INFO("STTS2H: Temp:{}", stts_temp.getTemp());
                }
                measure_start = g.my_systick.get_ticks();
            }
        }

        /* Command Queue */
        if (g_cmd_complete.load(std::memory_order_acquire)) {
            I2CCommand cmd;
            if (cmd_queue.pop(cmd)) {
                g_cmd_complete.store(false, std::memory_order_relaxed);
                getDrivers().i2c1.complete_flag_ = &g_cmd_complete;
                cmd.fn(cmd.ctx);
            }
        }

        /* Command Line Interface Input Processing */
        if constexpr (kCliEnable) {
            g.uart2.processRx();
            if (cmd.getState() == CliState::WaitingForInput) {
                cmd.get_input();
                cmd.setState(CliState::Processing);
            }
        }
    }
    return 0;
}

/* Function Body */

void RegisterCallback() {
    if constexpr (kCliEnable) {
        getDrivers().uart2.onDataReceived([](void* ctx, const uint8_t* data, size_t len) { static_cast<Cli*>(ctx)->onUartData(data, len); }, &cmd);
        cmd.setUart(UartRef::from(getDrivers().uart2));
        cmd.setSensor(&temp_sensor);
    }

    if constexpr (kSensorEnable) {
        getDrivers().i2c1.addReceiver(temp_sensor);
        getDrivers().i2c1.addReceiver(stts_temp);
        getDrivers().i2c1.addReceiver(cmd);
    }
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
    /* Put a breakpoint on the line below */
    volatile int loop = 1;
    while (loop) {
        // If the debugger stops here, a HardFault occurred!
    }
}

void Init_Driver(Drivers& g) {
    // Enable FPU by setting bits 20, 21, 22, and 23
    CPACR |= ((3UL << 20) | (3UL << 22));
    // Manual Barrier        instructions(Assembly)
    __asm volatile("dsb 0xf" ::: "memory");
    __asm volatile("isb 0xf" ::: "memory");

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

    constexpr UartConfig  uart2_cfg{
         .dev_num = UartNum::USART_D2, .baudRate = UartBaudRate::_9600, .comm = kCliEnable ? UartComm::RX_TX : UartComm::TX_ONLY, .parity = UartParity::NONE, .stopbits = UartStopBit::USART_CR2_STOP_1};

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
    LOG_INFO("USART2 Initialized");

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