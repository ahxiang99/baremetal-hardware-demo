#pragma once

#include "cpp/DmaI2C.hpp"
#include "cpp/DmaUart.hpp"
#include "cpp/ExtiInput.hpp"
#include "cpp/Gpio.hpp"
#include "cpp/Stm32Rcc.hpp"
#include "cpp/Stm32Timer.hpp"
#include "cpp/systick.hpp"
#include "cpp/wwdg.hpp"
#include "wwdg/cpp/wwdg.hpp"

struct Drivers {
    MySysTick      my_systick;
    SysClock       sysclock;
    Stm32GpioPin   gpio_led;
    ExtiInput      gpio_button;
    DmaUart        uart2;
    Stm32Timer     timer;
    DmaI2C         i2c1;
    WindowWatchDog wwdg;
};

Drivers& getDrivers();

template <typename T>
struct peripherals_regs_table {
    T*       instance;
    uint32_t enableBit;
    uint32_t resetBit;
};