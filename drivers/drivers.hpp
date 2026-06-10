#pragma once

#include "cpp/DmaI2C.hpp"
#include "cpp/DmaUart.hpp"
#include "cpp/IGpio.hpp"
#include "cpp/Stm32GpioPin.hpp"
#include "cpp/Stm32Timer.hpp"
#include "cpp/systick.hpp"

template <typename T>
struct peripherals_regs_table {
    T*       instance;
    uint32_t enableBit;
    uint32_t resetBit;
};

/* Consoildation all the drivers in the drivers header file */

struct Drivers {
    MySysTick    my_systick;
    Stm32GpioPin gpio_led;
    DmaUart      uart2;
    Stm32Timer   timer;
    DmaI2C       i2c1;
};

Drivers& getDriver();