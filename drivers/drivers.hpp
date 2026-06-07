#pragma once

#include "cpp/DmaI2C.hpp"
#include "cpp/DmaUart.hpp"
#include "cpp/Gpio.hpp"
#include "cpp/Stm32Timer.hpp"
#include "cpp/systick.hpp"

struct Drivers {
    MySysTick    my_systick;
    Stm32GpioPin gpio_led;
    DmaUart      uart2;
    Stm32Timer   timer;
    DmaI2C       i2c1;
};

Drivers& getDrivers();