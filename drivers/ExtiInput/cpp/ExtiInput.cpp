#include "ExtiInput.hpp"

#include <atomic>

#include "RegisterUtils.hpp"
#include "cpp/Stm32GpioPin.hpp"
#include "low-level/gpio_bitfields.h"
#include "low-level/gpio_types.h"
#include "low-level/nvic.h"
#include "low-level/rcc_bitfields.h"
#include "pch.hpp"

struct Exti_Table {
    uint32_t           GpioPin;
    volatile uint32_t* ExtiReg;
    uint32_t           NvicIRQ;
    uint32_t           ExitRegPos;
};

static const Exti_Table exti_gpio_table[GPIO_PIN_COUNT]{
    {GPIO_PIN_0,  &SYSCFG_PTR->EXTICR1, EXTI0_IRQn,     0 },
    {GPIO_PIN_1,  &SYSCFG_PTR->EXTICR1, EXTI1_IRQn,     4 },
    {GPIO_PIN_2,  &SYSCFG_PTR->EXTICR1, EXTI2_IRQn,     8 },
    {GPIO_PIN_3,  &SYSCFG_PTR->EXTICR1, EXTI3_IRQn,     12},
    {GPIO_PIN_4,  &SYSCFG_PTR->EXTICR2, EXTI4_IRQn,     0 },
    {GPIO_PIN_5,  &SYSCFG_PTR->EXTICR2, EXTI9_5_IRQn,   4 },
    {GPIO_PIN_6,  &SYSCFG_PTR->EXTICR2, EXTI9_5_IRQn,   8 },
    {GPIO_PIN_7,  &SYSCFG_PTR->EXTICR2, EXTI9_5_IRQn,   12},
    {GPIO_PIN_8,  &SYSCFG_PTR->EXTICR3, EXTI9_5_IRQn,   0 },
    {GPIO_PIN_9,  &SYSCFG_PTR->EXTICR3, EXTI9_5_IRQn,   4 },
    {GPIO_PIN_10, &SYSCFG_PTR->EXTICR3, EXTI15_10_IRQn, 8 },
    {GPIO_PIN_11, &SYSCFG_PTR->EXTICR3, EXTI15_10_IRQn, 12},
    {GPIO_PIN_12, &SYSCFG_PTR->EXTICR4, EXTI15_10_IRQn, 0 },
    {GPIO_PIN_13, &SYSCFG_PTR->EXTICR4, EXTI15_10_IRQn, 4 },
    {GPIO_PIN_14, &SYSCFG_PTR->EXTICR4, EXTI15_10_IRQn, 8 },
    {GPIO_PIN_15, &SYSCFG_PTR->EXTICR4, EXTI15_10_IRQn, 12},
};

ExtiInput::ExtiInput() : syscfg_instance_(SYSCFG_PTR), exti_instance_(EXTI_PTR) {}

void ExtiInput::initialize(const GPIO_Config& config) {
    pin_config_ = config;
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
    uint8_t mask{0};
    if (pin_config_.port == GPIO_Port::GPIO_PH) {
        mask = 0x7U;
    } else {
        mask = static_cast<uint8_t>(pin_config_.port);
    }
    exti_cr = exti_gpio_table[std::countr_zero(pin_config_.pin)].ExtiReg;
    RegisterUtils::setBits(*exti_cr, mask << exti_gpio_table[std::countr_zero(pin_config_.pin)].ExitRegPos);
}
void ExtiInput::configureExtiMask() {
    RegisterUtils::setBits(exti_instance_->IMR, pin_config_.pin);
    if (pin_config_.pupdr == GPIO_PUPDR::GPIO_PUPDR_PULLUP) {
        /* Initial State = 1 due to Pull Up Resistor, Trigger Interrupt when fall edge */
        RegisterUtils::setBits(exti_instance_->FTSR, pin_config_.pin);
        RegisterUtils::clearBits(exti_instance_->RTSR, pin_config_.pin);
    } else if (pin_config_.pupdr == GPIO_PUPDR::GPIO_PUPDR_PULLDOWN) {
        /* Initial State = 0 due to Pull Down Resistor, Trigger Interrupt when rising edge */
        RegisterUtils::setBits(exti_instance_->RTSR, pin_config_.pin);
        RegisterUtils::clearBits(exti_instance_->FTSR, pin_config_.pin);
    }
}
void ExtiInput::configureGpioPin() {
    gpio_.initialize(pin_config_);
}
void ExtiInput::handleInterrupt() {
    clearMask();
    flag_.store(true, std::memory_order_release);
}
void ExtiInput::configureNvic() {
    My_NVIC_EnableIRQ(exti_gpio_table[std::countr_zero(pin_config_.pin)].NvicIRQ);
}
void ExtiInput::clearMask() {
    RegisterUtils::setBits(exti_instance_->PR, pin_config_.pin);
}
void ExtiInput::setFnCallback(ExtiFn fn, void* ctx) {
    fn_  = fn;
    ctx_ = ctx;
}
void ExtiInput::processEvent() {
    if (flag_.load(std::memory_order_acquire) == true) {
        flag_.store(false, std::memory_order_relaxed);
        if (fn_) fn_(ctx_);
    }
}
