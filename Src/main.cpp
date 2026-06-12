#include "Middleware/cli.hpp"
#include "Middleware/logger.hpp"
#include "Sht40ad1b.hpp"
#include "cpp/Dma.hpp"
#include "cpp/Stm32GpioPin.hpp"
#include "cpp/UartConcepts.hpp"
#include "cpp/UartRef.hpp"
#include "drivers.hpp"
#include "pch.hpp"

#define CLI_ENABLE 0
#define SENSOR_ENABLE 1

#define CPACR (*((volatile uint32_t*)0xE000ED88))

// Global Variables
Sht40ad1b temp_sensor(getDrivers().i2c1, "SHT40");
Cli       cmd;

/* Function Prototype */
void Init_Driver(Drivers& g);
void RegisterCallback();

/* Main Program Start Here */
int main() {
    Drivers& g = getDrivers();
    Init_Driver(g);
    RegisterCallback();

    if (!g.timer.isRunning()) {
        g.timer.start(500);
    }

    uint32_t measure_start = g.my_systick.get_ticks();

    while (1) {
#if SENSOR_ENABLE
        g.i2c1.processRx();
        temp_sensor.ProcessData();

        if (g.my_systick.get_ticks() - measure_start > 500) {
            Packet<Sht40ad1b::SensorData> sensor_data(temp_sensor.getValue());
            g.uart2.send(sensor_data.raw(), sensor_data.size());
            measure_start = g.my_systick.get_ticks();
        }

#endif

#if CLI_ENABLE
        g.uart2.processRx();
        if (cmd.getState() == CliState::WaitingForInput) {
            cmd.get_input();
            cmd.setState(CliState::Processing);
        }
#endif
        if (g.timer.isElapsed()) {
            g.gpio_led.Toggle();
            g.timer.start(500);
#if !CLI_ENABLE && SENSOR_ENABLE
            temp_sensor.read();
#endif
        }
    }
    return 0;
}

/* Function Definition */

void Init_Driver(Drivers& g) {
    // Enable FPU by setting bits 20, 21, 22, and 23
    CPACR |= ((3UL << 20) | (3UL << 22));
    // Manual Barrier        instructions(Assembly)
    __asm volatile("dsb 0xf" ::: "memory");
    __asm volatile("isb 0xf" ::: "memory");

    /* MySysTick Init*/
    g.my_systick.init();

    /* Configure Uart2 Pin */
    constexpr GPIO_Config uart2_gpio_cfg{.port  = GPIO_Port::GPIO_PA,
                                         .pin   = GPIO_PIN_2 | GPIO_PIN_3,
                                         .mode  = GPIO_Moder::GPIO_MODE_ALTFN,
                                         .otype = GPIO_OType::GPIO_OTYPER_PP,
                                         .ospdr = GPIO_OSPDR::GPIO_OSPEEDR_VHS,
                                         .pupdr = GPIO_PUPDR::GPIO_PUPDR_NOPULL,
                                         .afr   = GPIO_AFR::GPIO_AF7_USART1_2};

    constexpr UartConfig  uart2_cfg{.dev_num  = UartNum::USART_D2,
                                    .baudRate = UartBaudRate::_9600,
#if CLI_ENABLE
                                   .comm = UartComm::RX_TX,
#else
                                   .comm = UartComm::TX_ONLY,
#endif
                                   .parity   = UartParity::NONE,
                                   .stopbits = UartStopBit::USART_CR2_STOP_1};

    /* This DMA Config is for USART2 Tx */
    const DMA_Config hdmatx_cfg{.DMA_BaseAddress = DMA1, .Stream = DMA_Stream::Stream_6, .Channel = DMA_Channel::Channel_4, .Direction = DMA_Direction::DMA_MEMORY_TO_PERIPH, .Mode = DMA_Mode::Normal};

    /* This DMA Config is for USART2 Rx */
    const DMA_Config hdmarx_cfg{
        .DMA_BaseAddress = DMA1, .Stream = DMA_Stream::Stream_5, .Channel = DMA_Channel::Channel_4, .Direction = DMA_Direction::DMA_PERIPH_TO_MEMORY, .Mode = DMA_Mode::Circular};

    /* Configure i2c1 Pin */
    constexpr GPIO_Config i2c1_gpio_config{.port  = GPIO_Port::GPIO_PB,
                                           .pin   = GPIO_PIN_8 | GPIO_PIN_9,
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
    g.uart2.initialize();
    LOG_INFO("Booting...");
    LOG_INFO("USART2 Initialized");

    I2C_Config i2c_config;
    i2c_config.ClockFreq      = I2C_Clk_Freq::_100KHz;
    i2c_config.AddressingMode = I2C_Addressing_Mode::AddressMode_7Bit;
    i2c_config.OwnAddress1    = 0;
    i2c_config.OwnAddress2    = 0;
    g.i2c1.setVariable(I2C1, i2c_config);
    g.i2c1.initialize();
    LOG_INFO("I2C1 Initialized");

    /* Blinking LED */
    constexpr GPIO_Config gpio_led_cfg{.port  = GPIO_Port::GPIO_PA,
                                       .pin   = GPIO_PIN_5,
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

    LOG_INFO("Initialized Done...");
}

void RegisterCallback() {
#if CLI_ENABLE
    getDrivers().uart2.onDataReceived([](void* ctx, const uint8_t* data, size_t len) { static_cast<Cli*>(ctx)->onUartData(data, len); }, &cmd);
    cmd.setUart(UartRef::from(g.uart2));
    cmd.setSensor(&temp_sensor);
#endif

#if SENSOR_ENABLE
    getDrivers().i2c1.onDataReceived(
        [](void* ctx1, void* ctx2) {
            auto* temp_sensor = static_cast<Sht40ad1b*>(ctx1);
            if (temp_sensor->getState() == Sht40ad1b::SensorState::WAIT_DATA) {
                temp_sensor->setState(Sht40ad1b::SensorState::DATA_READY);
            }
#if CLI_ENABLE
            auto* cmd = static_cast<Cli*>(ctx2);
            cmd->setState(CliState::WaitingForInput);
#endif
        },
        &temp_sensor, &cmd);
#endif
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

void HardFault_Handler(void) {
    /* Put a breakpoint on the line below */
    volatile int loop = 1;
    while (loop) {
        // If the debugger stops here, a HardFault occurred!
    }
}