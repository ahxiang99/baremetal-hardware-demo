#include "gpio.h"

#include "drivers/common/bit_utils.h"
#include "low-level/gpio_types.h"
#include "low-level/rcc.h"
#include "low-level/rcc_bitfields.h"

GPIO_TypeDef* GPIO_HardwareInit(gpio_port_t _port) {
    switch (_port) {
        case GPIO_PA:
            SET_BIT(RCC->AHB1ENR, RCC_AHB1ENR_GPIOA_EN);
            return GPIOA;
            break;
        case GPIO_PB:
            SET_BIT(RCC->AHB1ENR, RCC_AHB1ENR_GPIOB_EN);
            return GPIOB;
            break;
        case GPIO_PC:
            SET_BIT(RCC->AHB1ENR, RCC_AHB1ENR_GPIOC_EN);
            return GPIOC;
            break;
        case GPIO_PD:
            SET_BIT(RCC->AHB1ENR, RCC_AHB1ENR_GPIOD_EN);
            return GPIOD;
            break;
        case GPIO_PE:
            SET_BIT(RCC->AHB1ENR, RCC_AHB1ENR_GPIOE_EN);
            return GPIOE;
            break;
        case GPIO_PH:
            SET_BIT(RCC->AHB1ENR, RCC_AHB1ENR_GPIOH_EN);
            return GPIOH;
            break;
        default:
            return 0U;
    }
}

GPIO_TypeDef* GPIO_GetBaseAddress(gpio_port_t _port) {
    switch (_port) {
        case GPIO_PA:
            return GPIOA;
            break;
        case GPIO_PB:
            return GPIOB;
            break;
        case GPIO_PC:
            return GPIOC;
            break;
        case GPIO_PD:
            return GPIOD;
            break;
        case GPIO_PE:
            return GPIOE;
            break;
        case GPIO_PH:
            return GPIOH;
            break;
        default:
            return 0U;
    }
}

gpio_status_t GPIO_PinSetConfig(GPIO_TypeDef* GPIOx, const GPIO_InitTypeDef* IO_cfg) {
    uint32_t temp = 0;
    for (uint32_t i = 0; i < GPIO_NUMBER; ++i) {
        uint32_t pin_mask  = (1 << i);
        uint32_t currentIO = IO_cfg->PIN & pin_mask;

        if (currentIO == pin_mask) {
            // Configure MODER
            temp = GPIOx->MODER;
            temp &= ~(GPIO_MODER_MODE0 << (i * 2U));
            temp |= (IO_cfg->MODE << (i * 2U));
            GPIOx->MODER = temp;

            // Configure OTYPER
            temp = GPIOx->OTYPER;
            temp &= ~(GPIO_OTYPER_OTYPER0 << i);
            temp |= (IO_cfg->OTYPE << i);
            GPIOx->OTYPER = temp;

            // Configure OSPEEDR
            temp = GPIOx->OSPEEDR;
            temp &= ~(GPIO_OSPEEDR_OSPEEDR0 << (i * 2U));
            temp |= (IO_cfg->SPD << (i * 2U));
            GPIOx->OSPEEDR = temp;

            // Configure PUPDR
            temp = GPIOx->PUPDR;
            temp &= ~(GPIO_PUPDR_PUPDR0 << (i * 2U));
            temp |= (IO_cfg->PUPD << (i * 2U));
            GPIOx->PUPDR = temp;

            // Configure Alternate Function
            temp = GPIOx->AFR[i >> 3U];
            temp &= ~(0xFU << (i & 0x07U) * 4U);
            temp |= (IO_cfg->Alternate << (i & 0x07U) * 4U);
            GPIOx->AFR[i >> 3U] = temp;
        }
    }
    return GPIO_OK;
}

gpio_status_t GPIO_WriteOutputPin(GPIO_TypeDef* GPIOx, const uint16_t GPIO_PIN, const gpio_pin_state_t state) {
    if (state != GPIO_PIN_RESET) {
        SET_BIT(GPIOx->BSRR, GPIO_PIN);
    } else {
        GPIOx->BSRR = (uint32_t)GPIO_PIN << 16U;
    }
    return GPIO_OK;
}

gpio_status_t GPIO_ReadInputPin(GPIO_TypeDef* GPIOx, const uint16_t GPIO_PIN, gpio_pin_state_t* state) {
    if ((GPIOx->IDR & GPIO_PIN) != GPIO_PIN_RESET) {
        *state = GPIO_PIN_SET;
    } else {
        *state = GPIO_PIN_RESET;
    }
    return GPIO_OK;
}

gpio_status_t GPIO_ToggleOutputPin(GPIO_TypeDef* GPIOx, const uint16_t GPIO_PIN) {
    GPIOx->ODR ^= GPIO_PIN;
    return GPIO_OK;
}

gpio_status_t GPIO_HardwareReset(gpio_port_t _port) {
    switch (_port) {
        case GPIO_PA:
            SET_BIT(RCC->AHB1RSTR, RCC_AHB1RSTR_GPIOA_RST);
            break;
        case GPIO_PB:
            SET_BIT(RCC->AHB1RSTR, RCC_AHB1RSTR_GPIOB_RST);
            break;
        case GPIO_PC:
            SET_BIT(RCC->AHB1RSTR, RCC_AHB1RSTR_GPIOC_RST);
            break;
        case GPIO_PD:
            SET_BIT(RCC->AHB1RSTR, RCC_AHB1RSTR_GPIOD_RST);
            break;
        case GPIO_PE:
            SET_BIT(RCC->AHB1RSTR, RCC_AHB1RSTR_GPIOE_RST);
            break;
        case GPIO_PH:
            SET_BIT(RCC->AHB1RSTR, RCC_AHB1RSTR_GPIOH_RST);
            break;
        default:
            return GPIO_ERR;
    }
    return GPIO_OK;
}