#include "Middleware/cli.hpp"
#include "Middleware/logger.hpp"
#include "Register.hpp"
#include "cpp/Stm32GpioPin.hpp"
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
    cmd.setUart(&g.uart2);

    if (!g.timer.isRunning()) {
        g.timer.start(500);
    }

    while (1) {
#if SENSOR_ENABLE
        g.i2c1.processRx();
        temp_sensor.ProcessData();
#endif

#if CLI_ENABLE
        uart2.processRx();
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
    GPIO_Config uart2_gpio_cfg;
    uart2_gpio_cfg.port  = GPIO_Port::GPIO_PA;
    uart2_gpio_cfg.pin   = GPIO_PIN_2 | GPIO_PIN_3;
    uart2_gpio_cfg.mode  = GPIO_Moder::GPIO_MODE_ALTFN;
    uart2_gpio_cfg.ospdr = GPIO_OSPDR::GPIO_OSPEEDR_VHS;
    uart2_gpio_cfg.otype = GPIO_OType::GPIO_OTYPER_PP;
    uart2_gpio_cfg.pupdr = GPIO_PUPDR::GPIO_PUPDR_NOPULL;
    uart2_gpio_cfg.afr   = GPIO_AFR::GPIO_AF7_USART1_2;

    /* Configure i2c1 Pin */
    GPIO_Config i2c1_gpio_config;
    i2c1_gpio_config.port  = GPIO_Port::GPIO_PB;
    i2c1_gpio_config.pin   = GPIO_PIN_8 | GPIO_PIN_9;
    i2c1_gpio_config.mode  = GPIO_Moder::GPIO_MODE_ALTFN;
    i2c1_gpio_config.otype = GPIO_OType::GPIO_OTYPER_OD;
    i2c1_gpio_config.ospdr = GPIO_OSPDR::GPIO_OSPEEDR_VHS;
    i2c1_gpio_config.afr   = GPIO_AFR::GPIO_AF4_I2C1_3;
    i2c1_gpio_config.pupdr = GPIO_PUPDR::GPIO_PUPDR_PULLUP;

    /* Configure Uart */
    UartConfig uart2_cfg;
    uart2_cfg.dev_num  = UartNum::USART_D2;
    uart2_cfg.baudRate = UartBaudRate::_9600;
#if CLI_ENABLE
    uart2_cfg.comm = UartComm::RX_TX;
#else
    uart2_cfg.comm = UartComm::TX_ONLY;
#endif
    uart2_cfg.parity   = UartParity::NONE;
    uart2_cfg.stopbits = UartStopBit::STOP_1;
    g.uart2.setVariable(USART2, uart2_cfg);
    g.uart2.initialize();

    /* Logger Init */
    Logger::Init(g.uart2);
    Logger::set_level(LogLevel::INFO);

    Stm32GpioPin temp;
    Startup(temp, uart2_gpio_cfg, i2c1_gpio_config);
    LOG_INFO("Booting...");
    LOG_INFO("System Initialized via IUart interface!");
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
    GPIO_Config gpio_led_cfg{.port  = GPIO_Port::GPIO_PA,
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
    uart2.onDataReceived([](void* ctx, const uint8_t* data, size_t len) { static_cast<Cli*>(ctx)->onUartData(data, len); }, &cmd);
#endif
#if SENSOR_ENABLE
    getDrivers().i2c1.onDataReceived(
        [](void* ctx1, void* ctx2) {
            auto* temp_sensor = static_cast<Sht40ad1b*>(ctx1);
            if (temp_sensor->getState() == SensorState::WAIT_DATA) {
                temp_sensor->setState(SensorState::DATA_READY);
            }
#if CLI_ENABLE
            auto* cmd = static_cast<Cli*>(ctx1);
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