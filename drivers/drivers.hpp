#pragma once

#include "SSD1306/oled_SSD1306.hpp"
#include "cpp/DmaI2C.hpp"
#include "cpp/DmaUart.hpp"
#include "cpp/ExtiInput.hpp"
#include "cpp/Gpio.hpp"
#include "cpp/Stm32Spi.hpp"
#include "cpp/Stm32Timer.hpp"
#include "cpp/systick.hpp"

struct Drivers {
    MySysTick    my_systick;
    Stm32GpioPin gpio_led;
    ExtiInput    gpio_button;
    DmaUart      uart2;
    Stm32Timer   timer;
    DmaI2C       i2c1;
    Stm32Spi     spi1;
    OLED_Display disp;
};

Drivers& getDrivers();

template <typename T>
struct peripherals_regs_table {
    T*       instance;
    uint32_t enableBit;
    uint32_t resetBit;
};