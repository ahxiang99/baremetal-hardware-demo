#include "Stm32GpioPin.hpp"

#include <cstdint>

#include "IGpio.hpp"
#include "RegisterUtils.hpp"

struct GPIO_Port_Table {
    GPIO_TypeDef* instance;
    uint32_t      RCC_AHB1ENR_BIT;
    uint32_t      RCC_AHB1RSTR_BIT;
};

static const GPIO_Port_Table gpio_table[static_cast<uint8_t>(GPIO_Port::GPIO_PORT_COUNT)] = {
    {GPIOA, RCC_AHB1ENR_GPIOA_EN, RCC_AHB1RSTR_GPIOA_RST},
    {GPIOB, RCC_AHB1ENR_GPIOB_EN, RCC_AHB1RSTR_GPIOB_RST},
    {GPIOC, RCC_AHB1ENR_GPIOC_EN, RCC_AHB1RSTR_GPIOC_RST},
    {GPIOD, RCC_AHB1ENR_GPIOD_EN, RCC_AHB1RSTR_GPIOD_RST},
    {GPIOE, RCC_AHB1ENR_GPIOE_EN, RCC_AHB1RSTR_GPIOE_RST},
    {GPIOH, RCC_AHB1ENR_GPIOH_EN, RCC_AHB1RSTR_GPIOH_RST},
};

bool Stm32GpioPin::Init() {
    enableGpioClock();
    configurePin();
    return true;
}

void Stm32GpioPin::Write(GPIO_State state) {
    if (state != GPIO_State::LOW) {
        gpio_->BSRR = static_cast<uint32_t>(config_.pin);
    } else {
        gpio_->BSRR = static_cast<uint32_t>(config_.pin) << 16U;
    }
}

GPIO_State Stm32GpioPin::Read() {
    if ((gpio_->IDR & static_cast<uint32_t>(config_.pin)) != static_cast<uint32_t>(GPIO_State::LOW)) {
        return GPIO_State::HIGH;
    } else {
        return GPIO_State::LOW;
    }
}
void Stm32GpioPin::Toggle() {
    if ((gpio_->ODR & static_cast<uint32_t>(config_.pin))) {
        Write(GPIO_State::LOW);
    } else {
        Write(GPIO_State::HIGH);
    }
}

void Stm32GpioPin::enableGpioClock() {
    volatile uint32_t* enrReg    = &RCC->AHB1ENR;
    uint32_t           enableBit = gpio_table[static_cast<uint8_t>(config_.port)].RCC_AHB1ENR_BIT;
    RegisterUtils::setBits(*enrReg, enableBit);
}

void Stm32GpioPin::configurePin() {
    uint32_t temp = 0;
    for (auto i = 0; i < 16; ++i) {
        uint32_t pin_mask   = 1 << i;
        uint32_t currentpin = config_.pin & pin_mask;

        if (currentpin == pin_mask) {
            temp = gpio_->MODER;
            RegisterUtils::clearBits(temp, (0x11 << 0) << (i * 2));
            RegisterUtils::setBits(temp, static_cast<uint32_t>(config_.mode) << (i * 2));
            gpio_->MODER = temp;

            temp         = gpio_->OTYPER;
            RegisterUtils::clearBits(temp, 1 << i);
            RegisterUtils::setBits(temp, static_cast<uint32_t>(config_.otype) << i);
            gpio_->OTYPER = temp;

            temp          = gpio_->OSPEEDR;
            RegisterUtils::clearBits(temp, (0x11 << 0) << (i * 2));
            RegisterUtils::setBits(temp, static_cast<uint32_t>(config_.ospdr) << (i * 2));
            gpio_->OSPEEDR = temp;

            temp           = gpio_->PUPDR;
            RegisterUtils::clearBits(temp, (0x11 << 0) << (i * 2));
            RegisterUtils::setBits(temp, static_cast<uint32_t>(config_.pupdr) << (i * 2));
            gpio_->PUPDR = temp;

            temp         = gpio_->AFR[i >> 3U];
            temp &= ~(0xFU << (i & 0x07U) * 4U);
            temp |= (static_cast<uint32_t>(config_.afr) << (i & 0x07U) * 4U);
            gpio_->AFR[i >> 3U] = temp;
        }
    }
}
void Stm32GpioPin::setVariable(GPIO_TypeDef* gpio, const GPIO_Config& config) {
    gpio_   = gpio;
    config_ = config;
}
