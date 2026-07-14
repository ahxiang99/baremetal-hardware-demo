#include "Stm32GpioPin.hpp"

#include <cstdint>

#include "RegisterUtils.hpp"
#include "logger.hpp"
#include "low-level/gpio_bitfields.h"
#include "low-level/gpio_registers.h"
#include "pch.hpp"

namespace {

static const peripherals_regs_table<GPIO_TypeDef> gpio_table[static_cast<uint8_t>(GPIO_Port::GPIO_PORT_COUNT)] = {
    {GPIOA, RCC_AHB1ENR_GPIOA_EN, RCC_AHB1RSTR_GPIOA_RST},
    {GPIOB, RCC_AHB1ENR_GPIOB_EN, RCC_AHB1RSTR_GPIOB_RST},
    {GPIOC, RCC_AHB1ENR_GPIOC_EN, RCC_AHB1RSTR_GPIOC_RST},
    {GPIOD, RCC_AHB1ENR_GPIOD_EN, RCC_AHB1RSTR_GPIOD_RST},
    {GPIOE, RCC_AHB1ENR_GPIOE_EN, RCC_AHB1RSTR_GPIOE_RST},
    {GPIOH, RCC_AHB1ENR_GPIOH_EN, RCC_AHB1RSTR_GPIOH_RST},
};

Result<> validate(const GPIO_Config& cfg) {
    if (static_cast<uint8_t>(cfg.port) >= static_cast<uint8_t>(GPIO_Port::GPIO_PORT_COUNT)) {
        return Fail(Err::InvalidPort);
    }
    if ((cfg.pin & 0xFFFFU) == 0) {
        return Fail(Err::InvalidPinMask);
    }
    if (cfg.afr != GPIO_AFR::GPIO_AF0_SYSTEM && cfg.mode != GPIO_Moder::GPIO_MODE_ALTFN) {
        return Fail(Err::AfOnNonAltFn);
    }
    return Ok();
}

GPIO_TypeDef* enableAndGet(GPIO_Port port) {
    const auto& entry = gpio_table[static_cast<uint8_t>(port)];
    RegisterUtils::setBits(RCC->AHB1ENR, entry.enableBit);
    (void)RCC->AHB1ENR;
    return entry.instance;
}

void applyPinConfig(GPIO_TypeDef* gpio, const GPIO_Config& cfg) {
    for (uint32_t i = 0; i < GPIO_PIN_COUNT; ++i) {
        if (!(cfg.pin & (1u << i))) continue;

        RegisterUtils::modify(gpio->MODER, (3U << (i * 2)), static_cast<uint32_t>(cfg.mode) << (i * 2));
        RegisterUtils::modify(gpio->OTYPER, (1U << i), static_cast<uint32_t>(cfg.otype) << i);
        RegisterUtils::modify(gpio->OSPEEDR, (3U << (i * 2)), static_cast<uint32_t>(cfg.ospdr) << (i * 2));
        RegisterUtils::modify(gpio->PUPDR, (3U << (i * 2)), static_cast<uint32_t>(cfg.pupdr) << (i * 2));
        if (cfg.mode == GPIO_Moder::GPIO_MODE_ALTFN) {  // guarded now
            const uint32_t sh = (i & 7u) * 4u;
            RegisterUtils::modify(gpio->AFR[i >> 3], 0xFu << sh, static_cast<uint32_t>(cfg.afr) << sh);
        }
    }
}

}  // namespace

Result<> Gpio::configureMux(const GPIO_Config& cfg) {
    TRY(validate(cfg));
    applyPinConfig(enableAndGet(cfg.port), cfg);
    return Ok();
}

Result<> Stm32GpioPin::initialize(const GPIO_Config& cfg) {
    TRY(validate(cfg));
    if (__builtin_popcount(cfg.pin & 0xFFFFU) != 1) {
        return Fail(Err::InvalidPinMask);
    }
    gpio_ = enableAndGet(cfg.port);
    if (gpio_ == nullptr) return Fail(Err::NullInstance);

    pin_ = cfg.pin & 0xFFFFU;
    applyPinConfig(gpio_, cfg);
    LOG_DEBUG("GPIO pin {} on port {} ready", pin_, static_cast<uint32_t>(cfg.port));
    return Ok();
}

void Stm32GpioPin::write(GPIO_State s) {
    gpio_->BSRR = (s == GPIO_State::LOW) ? (pin_ << 16) : pin_;
}

GPIO_State Stm32GpioPin::read() const {
    return (gpio_->IDR & pin_) ? GPIO_State::HIGH : GPIO_State::LOW;
}

void Stm32GpioPin::toggle() {
    if ((gpio_->ODR & static_cast<uint32_t>(pin_))) {
        write(GPIO_State::LOW);
    } else {
        write(GPIO_State::HIGH);
    }
}