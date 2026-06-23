#pragma once

#include "cpp/Stm32GpioPin.hpp"
#include "low-level/syscfg_registers.h"

class Syscfg {
    SYSCFG_TypeDef* instance;
    EXTI_TypeDef*   EXTI_instance;
    GPIO_Config     m_config;

   public:
    Syscfg() {}

    void initialize();
    void configure(const GPIO_Config& config);
    void handleInterrupt();

   private:
    void enableClock();
    void configureEXTICR();
    void configureEXTIMask();
    void configureGpioPin();
};