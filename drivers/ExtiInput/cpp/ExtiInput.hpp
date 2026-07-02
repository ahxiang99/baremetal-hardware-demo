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
    void setCallback(void (*fn)(void* ctx), void* ctx);

   private:
    void         enableClock();
    void         configureExtiCr();
    void         configureExtiMask();
    void         configureGpioPin();
    void         configureNvic();

    Stm32GpioPin gpio_;
    GPIO_Config  config_;
    uint8_t      line_{};

    uint32_t     m_last_press{0};
    void (*m_callback)(void* ctx) = nullptr;
    void* m_ctx                   = nullptr;
};