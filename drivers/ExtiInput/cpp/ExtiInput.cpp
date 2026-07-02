#include "ExtiInput.hpp"

#include "RegisterUtils.hpp"
#include "cpp/Stm32GpioPin.hpp"
#include "drivers.hpp"
#include "low-level/rcc_bitfields.h"
#include "pch.hpp"

void ExtiInput::initialize() {
    enableClock();
    configureGpioPin();
    configureExtiCr();
    configureExtiMask();
    configureNvic();
}
void ExtiInput::enableClock() {
    RegisterUtils::setBits(RCC->APB2ENR, RCC_APB2ENR_SYSCFG_EN);
}
void ExtiInput::configureExtiCr() {
    volatile uint32_t* enableReg = &instance->EXTICR4;
    RegisterUtils::setBits(*enableReg, 2 << 4);
}
void ExtiInput::configureExtiMask() {
    RegisterUtils::setBits(EXTI_instance->IMR, 1 << 13);
    RegisterUtils::setBits(EXTI_instance->FTSR, 1 << 13);
    RegisterUtils::clearBits(EXTI_instance->RTSR, 1 << 13);
}
void ExtiInput::configure(const GPIO_Config& config) {
    instance      = SYSCFG;
    EXTI_instance = EXTI;
    m_config      = config;
}
void ExtiInput::configureGpioPin() {
    gpio_.Init(m_config);
}
void ExtiInput::handleInterrupt() {
    const uint32_t PR = EXTI_instance->PR;
    if (PR & (1U << 13)) {
        RegisterUtils::setBits(EXTI_instance->PR, (1U << 13));

        uint32_t now = getDrivers().my_systick.get_ticks();
        if (now - m_last_press > 200) {
            m_last_press = now;
            if (m_callback) m_callback(m_ctx);
        }
    }
}
void ExtiInput::configureNvic() {
    My_NVIC_EnableIRQ(EXTI15_10_IRQn);
}
void ExtiInput::setCallback(void (*fn)(void*), void* ctx) {
    m_callback = fn;
    m_ctx      = ctx;
}
