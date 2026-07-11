#include "Sht40ad1b.hpp"
#include "TaskMonitoring.hpp"
#include "cli.hpp"
#include "cpp/Stm32GpioPin.hpp"
#include "cpp/Stm32I2C.hpp"
#include "cpp/Stm32Spi.hpp"
#include "oled_SSD1306.hpp"
#include "pch.hpp"
#include "semphr.h"
#include "stts2h.hpp"

static volatile uint32_t& CPACR = *reinterpret_cast<volatile uint32_t*>(0xE000ED88);

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

/* Main Program Start Here */
int main() {
    *reinterpret_cast<volatile uint32_t*>(0xE000ED08) = 0x08004000;
    Drivers& g                                        = getDrivers();
    Init_Driver(g);

    RegisterCallback();
    stts_temp.initialize();

    SemaphoreHandle_t i2c_mutex = xSemaphoreCreateMutex();
    if (i2c_mutex == NULL) {
        // Heap exhausted — can't create mutex
        __asm volatile("bkpt #0");
        while (1);
    }

    xTaskCreate(wwdgTask, "WWDG", 512, NULL, 5, NULL);

    xTaskCreate(ledTask, "LED", 1024, NULL, 4, NULL);

    xTaskCreate(uartTask, "UART", 256, NULL, 3, NULL);

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

    /* Configure MCU Frequency */
    constexpr SysClockConfig sys_cfg_84{
        SysClockSource::PLL, {HSI_Freq_Hz, 8, 84, 2},
         AHB_ClockDivision::DIV_1, APB_ClockDivision::DIV_2, APB_ClockDivision::DIV_1, 3
    };

    static_assert(isValidPllConfig(sys_cfg_84.PllCfg), "PLL config invalid");
    constexpr ClockTree trial_clock = calcClockTree_v2(sys_cfg_84);

    // Validate peripheral clocks:
    static_assert(trial_clock.sysclk == 84'000'000, "SYSCLK must be 84MHz");
    static_assert(trial_clock.apb1 <= 42'000'000, "APB1 overclock!");
    static_assert(trial_clock.apb2 <= 84'000'000, "APB2 overclock!");

    g.sysclock.initialize(sys_cfg_84, trial_clock);

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

    /* Configure spi1 : To use PB3 SPI_SCK, PB4 SPI1_MISO, PB5 SPI_MOSI */
    constexpr GPIO_Config spi1_gpio_config{.pin   = GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5,
                                           .port  = GPIO_Port::GPIO_PB,
                                           .mode  = GPIO_Moder::GPIO_MODE_ALTFN,
                                           .otype = GPIO_OType::GPIO_OTYPER_PP,
                                           .ospdr = GPIO_OSPDR::GPIO_OSPEEDR_VHS,
                                           .pupdr = GPIO_PUPDR::GPIO_PUPDR_NOPULL,
                                           .afr   = GPIO_AFR::GPIO_AF5_SPI};

    /* Logger Init */
    Logger::Init(UartRef::from(g.uart2));
    Logger::set_level(LogLevel::INFO);

    Stm32GpioPin temp;
    Startup(temp, uart2_gpio_cfg, i2c1_gpio_config, spi1_gpio_config);

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

    /* SPI1 Config */
    Stm32Spi::Config spi_cfg{.dev               = Stm32Spi::SpiDev::SPI_D1,
                             .mode              = Stm32Spi::Mode::Master,
                             .cpol              = Stm32Spi::ClockPolarity::IdleLow,
                             .cpha              = Stm32Spi::ClockPhase::FirstEdge,
                             .dataSize          = Stm32Spi::DataSize::Bits8,
                             .bitOrder          = Stm32Spi::BitOrder::MsbFirst,
                             .baudRatePrescalar = 5,
                             .nssSoftware       = true};

    g.spi1.initialize(spi_cfg);

    g.disp.Initialize(g.spi1);

    LOG_INFO("Initialized Done...");
}