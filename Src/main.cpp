#include <sys/_intsup.h>

#include "Middleware/cli.hpp"
#include "Middleware/logger.hpp"
#include "RingBuffer.hpp"
#include "Sensor.hpp"
#include "Sht40ad1b.hpp"
#include "cpp/Dma.hpp"
#include "cpp/IGpio.hpp"
#include "cpp/ITimer.hpp"
#include "drivers/drivers.hpp"
#include "low-level/DMA_registers.h"
#include "low-level/tim_types.h"
#include "pch.hpp"

#define CLI_ENABLE 0
#define SENSOR_ENABLE 1

#define CPACR (*((volatile uint32_t*)0xE000ED88))

// Global Variables

/* Function Prototype */
void Init_Driver(Drivers& g_Drivers);

/* Main Program Start Here */
int main() {
    Drivers&  g_Drivers = getDriver();
    Cli       cmd;
    Sht40ad1b temp_sensor(g_Drivers.i2c1, g_Drivers.my_systick, "Sht40");

    Init_Driver(g_Drivers);

#if CLI_ENABLE
    uart2.onDataReceived([](void* ctx, const uint8_t* data,
                            size_t len) { static_cast<Cli*>(ctx)->onUartData(data, len); },
                         &cmd);
#endif
#if SENSOR_ENABLE
    g_Drivers.i2c1.onDataReceived(
        [](void* ctx1, void* ctx2) {
            auto* temp_sensor = static_cast<Sht40ad1b*>(ctx1);
            if (temp_sensor->getState() == SensorState::WAIT_DATA) {
                temp_sensor->setState(SensorState::DATA_READY);
            }
#if CLI_ENABLE
            auto* cmd = static_cast<Cli*>(ctx2);
            cmd->setState(CliState::WaitingForInput);
#endif
        },
        &temp_sensor, &cmd);
#endif

    cmd.setUart(&g_Drivers.uart2);
    cmd.setSensor(&temp_sensor);

    if (!g_Drivers.timer.isRunning()) {
        g_Drivers.timer.start(500);
    }

    uint32_t measure_start = g_Drivers.my_systick.get_ticks();

    while (1) {
#if SENSOR_ENABLE
        g_Drivers.i2c1.processRx();
        temp_sensor.ProcessData();
        if ((g_Drivers.my_systick.get_ticks() - measure_start) > 500) {
            Packet<Sht40_Sensordata> sensor_data{temp_sensor.getValue()};
            g_Drivers.uart2.send(sensor_data.raw(), sensor_data.size());
            measure_start = g_Drivers.my_systick.get_ticks();
        }
#endif

#if CLI_ENABLE
        uart2.processRx();
        if (cmd.getState() == CliState::WaitingForInput) {
            cmd.get_input();
            cmd.setState(CliState::Processing);
        }
#endif

        if (g_Drivers.timer.isElapsed()) {
            g_Drivers.gpio_led.Toggle();
            g_Drivers.timer.start(500);
#if !CLI_ENABLE && SENSOR_ENABLE
            temp_sensor.read();
#endif
        }
    }
    return 0;
}

/* Function Definition */

void Init_Driver(Drivers& g_Drivers) {
    // Enable FPU by setting bits 20, 21, 22, and
    // 23
    CPACR |= ((3UL << 20) | (3UL << 22));
    // Manual Barrier instructions(Assembly)
    __asm volatile("dsb 0xf" ::: "memory");
    __asm volatile("isb 0xf" ::: "memory");

    /* MySysTick Init*/
    g_Drivers.my_systick.init();

    /* Configure Uart2 Pin */
    constexpr GPIO_Config uart2_gpio_cfg{
        .port  = GPIO_Port::GPIO_PA,
        .pin   = GPIO_PIN_2 | GPIO_PIN_3,
        .mode  = GPIO_Moder::GPIO_MODE_ALTFN,
        .otype = GPIO_OType::GPIO_OTYPER_PP,
        .ospdr = GPIO_OSPDR::GPIO_OSPEEDR_VHS,
        .pupdr = GPIO_PUPDR::GPIO_PUPDR_NOPULL,
        .afr   = GPIO_AFR::GPIO_AF7_USART1_2,
    };
    Stm32GpioPin gpio_uart2(GPIOA, uart2_gpio_cfg);

    gpio_uart2.Init();

    /* Configure Uart */
    constexpr UartConfig uart2_cfg{
        .dev_num  = UartNum::USART_D2,
        .baudRate = UartBaudRate::_9600,
#if CLI_ENABLE
        .comm = UartComm::RX_TX,
#else
        .comm     = UartComm::TX_ONLY,
        .parity   = UartParity::NONE,
        .stopbits = UartStopBit::STOP_1
#endif
    };

    constexpr GPIO_Config i2c_gpio_cfg{
        .port  = GPIO_Port::GPIO_PB,
        .pin   = GPIO_PIN_8 | GPIO_PIN_9,
        .mode  = GPIO_Moder::GPIO_MODE_ALTFN,
        .otype = GPIO_OType::GPIO_OTYPER_OD,
        .ospdr = GPIO_OSPDR::GPIO_OSPEEDR_VHS,
        .pupdr = GPIO_PUPDR::GPIO_PUPDR_PULLUP,
        .afr   = GPIO_AFR::GPIO_AF4_I2C1_3,
    };
    Stm32GpioPin gpio_i2c(GPIOB, i2c_gpio_cfg);

    gpio_i2c.Init();

    /* This DMA Config is for USART2 Tx */
    const DMA_Config txdma_cfg{.DMA_BaseAddress = (DMA1),
                               .Stream          = DMA_Stream::Stream_6,
                               .Channel         = DMA_Channel::Channel_4,
                               .Direction       = DMA_Direction::DMA_MEMORY_TO_PERIPH,
                               .Mode            = DMA_Mode::Normal};

    /* This DMA Config is for USART2 Rx */
    const DMA_Config rxdma_cfg{
        .DMA_BaseAddress = DMA1,
        .Stream          = DMA_Stream::Stream_5,
        .Channel         = DMA_Channel::Channel_4,
        .Direction       = DMA_Direction::DMA_PERIPH_TO_MEMORY,
        .Mode            = DMA_Mode::Circular,
    };

    g_Drivers.uart2.configure(uart2_cfg, txdma_cfg, rxdma_cfg);
    g_Drivers.uart2.initialize();

    /* Logger Init */
    Logger::Init(g_Drivers.uart2);
    Logger::set_level(LogLevel::INFO);

    LOG_INFO("Booting...");
    LOG_INFO(
        "System Initialized via IUart "
        "interface!");
    LOG_INFO("USART2 Initialized");

    constexpr I2C_Config i2c_config{.ClockFreq      = I2C_Clk_Freq::_100KHz,
                                    .OwnAddress1    = 0,
                                    .AddressingMode = I2C_Addressing_Mode::AddressMode_7Bit,
                                    .OwnAddress2    = 0};

    g_Drivers.i2c1.configure(i2c_config);
    g_Drivers.i2c1.initialize();

    LOG_INFO("I2C1 Initialized");

    /* Timer */
    constexpr TimerConfig timer_cfg{.Instance          = APB1_TIMER_3,
                                    .CounterMode       = TIM_COUNTERMODE_UP,
                                    .ClockDivision     = TIM_CLOCKDIVISION_DIV1,
                                    .AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE};
    g_Drivers.timer.setVariable(TIM3, timer_cfg);

    /* Blinking LED */
    constexpr GPIO_Config gpio_led_cfg{.port  = GPIO_Port::GPIO_PA,
                                       .pin   = GPIO_PIN_5,
                                       .mode  = GPIO_Moder::GPIO_MODE_OUTPUT,
                                       .otype = GPIO_OType::GPIO_OTYPER_PP,
                                       .ospdr = GPIO_OSPDR::GPIO_OSPEEDR_LS,
                                       .pupdr = GPIO_PUPDR::GPIO_PUPDR_NOPULL,
                                       .afr   = GPIO_AFR::GPIO_AF0_SYSTEM};

    g_Drivers.gpio_led.configure(GPIOA, gpio_led_cfg);
    g_Drivers.gpio_led.Init();
    g_Drivers.gpio_led.Toggle();
    LOG_INFO("Initialized Done...");
}

/* Interrupt Handler Function Start Here*/

extern "C" void TIM3_IRQHandler(void) {
    getDriver().timer.handleInterrupt();
}

extern "C" void DMA1_Stream0_IRQHandler(void) {
    getDriver().i2c1.handleRxDmaInterrupt();
}

extern "C" void DMA1_Stream7_IRQHandler(void) {
    getDriver().i2c1.handleTxDmaInterrupt();
}

extern "C" void DMA1_Stream6_IRQHandler(void) {
    getDriver().uart2.handleTxDmaInterrupt();
}

extern "C" void DMA1_Stream5_IRQHandler(void) {
    getDriver().uart2.handleRxDmaInterrupt();
}

extern "C" void USART2_IRQHandler(void) {
    getDriver().uart2.handleInterrupt();
}

extern "C" void I2C1_EV_IRQHandler(void) {
    getDriver().i2c1.handleEVInterrupt();
}

extern "C" void I2C1_ER_IRQHandler(void) {
    getDriver().i2c1.handleERInterrupt();
}

void HardFault_Handler(void) {
    /* Put a breakpoint on the line below */
    volatile int loop = 1;
    while (loop) {
        // If the debugger stops here, a HardFault
        // occurred!
    }
}