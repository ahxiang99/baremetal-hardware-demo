#pragma once

#include "cpp/Stm32GpioPin.hpp"
#include "low-level/syscfg_registers.h"

class ExtiInput {
    SYSCFG_TypeDef* instance;
    EXTI_TypeDef*   EXTI_instance;
    GPIO_Config     m_config;

   public:
    ExtiInput() {}

    void initialize();
    void configure(const GPIO_Config& config);
    void handleInterrupt();

   private:
    void         enableClock();
    void         configureExtiCr();
    void         configureExtiMask();
    void         configureGpioPin();
    void         configureNvic();

    Stm32GpioPin gpio_;
    GPIO_Config  config_;
    uint8_t      line_{};
};