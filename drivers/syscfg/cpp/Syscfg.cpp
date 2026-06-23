#include "Syscfg.hpp"

#include "RegisterUtils.hpp"
#include "cpp/Stm32GpioPin.hpp"
#include "low-level/rcc_bitfields.h"
#include "pch.hpp"

void Syscfg::initialize() {
    enableClock();
    configureGpioPin();
    configureEXTICR();
    configureEXTIMask();
    My_NVIC_EnableIRQ(EXTI15_10_IRQn);
}
void Syscfg::enableClock() {
    volatile uint32_t* enableReg = &RCC->APB2ENR;
    RegisterUtils::setBits(*enableReg, RCC_APB2ENR_SYSCFG_EN);
}
void Syscfg::configureEXTICR() {
    volatile uint32_t* enableReg = &instance->EXTICR4;
    RegisterUtils::setBits(*enableReg, 2 << 4);
}
void Syscfg::configureEXTIMask() {
    RegisterUtils::setBits(EXTI_instance->IMR, 1 << 13);
    RegisterUtils::setBits(EXTI_instance->FTSR, 1 << 13);
    RegisterUtils::clearBits(EXTI_instance->RTSR, 1 << 13);
}
void Syscfg::configure(const GPIO_Config& config) {
    instance      = SYSCFG;
    EXTI_instance = EXTI;
    m_config      = config;
}
void Syscfg::configureGpioPin() {
    Stm32GpioPin temp;
    temp.Init(m_config);
}
void Syscfg::handleInterrupt() {
    const uint32_t PR = EXTI_instance->PR;
    if (PR & (1U << 13)) {
        RegisterUtils::setBits(EXTI_instance->PR, (1U << 13));
    }
}
