#pragma once

#include "SSD1306/oled_SSD1306.hpp"
#include "cpp/DmaI2C.hpp"
#include "cpp/DmaUart.hpp"
#include "cpp/ExtiInput.hpp"
#include "cpp/Gpio.hpp"
#include "cpp/Stm32Rcc.hpp"
#include "cpp/Stm32Spi.hpp"
#include "cpp/Stm32Timer.hpp"
#include "cpp/systick.hpp"
#include "rtc/cpp/Stm32RTC.hpp"

template <typename T>
struct peripherals_regs_table {
    T*       instance;
    uint32_t enableBit;
    uint32_t resetBit;
};

struct Drivers {
    Stm32GpioPin gpio_led;
    DmaUart      uart2;
    DmaI2C       i2c1;
    Stm32Spi     spi1;
    MySysTick    my_systick;
    Stm32Rtc     rtc;
    Stm32Timer   timer;
    SysClock     sysclock;
    ExtiInput    user_button;
    OLED_Display disp;
};

Drivers& getDrivers();