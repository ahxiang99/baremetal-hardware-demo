#include "gpio.h"

#include "drivers/common/bit_utils.h"
#include "low-level/gpio_types.h"
#include "low-level/rcc.h"
#include "low-level/rcc_bitfields.h"

status_t GPIO_HardwareInit(gpio_port_t port, GPIO_TypeDef** instance) {
    assert(port < GPIO_PORT_COUNT);
    if (port >= GPIO_PORT_COUNT) return ERR_INVALID_ARG;
    SET_BIT(RCC->AHB1ENR, gpio_port_table[port].RCC_AHB1ENR_BIT);
    *instance = gpio_port_table[port].instance;
    return STATUS_OK;
}

status_t GPIO_HardwareReset(gpio_port_t port) {
    assert(port < GPIO_PORT_COUNT);
    if (port >= GPIO_PORT_COUNT) return ERR_INVALID_ARG;
    SET_BIT(RCC->AHB1RSTR, gpio_port_table[port].RCC_AHB1RSTR_BIT);
    return STATUS_OK;
}

status_t GPIO_PinSetConfig(GPIO_TypeDef* p_Instance, const GPIO_InitTypeDef* p_Config) {
    assert(p_Instance != NULL);
    assert(p_Config != NULL);

    if (!p_Config || !p_Instance) return ERR_NULLPTR;

    uint32_t temp = 0;
    for (uint32_t i = 0; i < GPIO_PIN_COUNT; ++i) {
        uint32_t pin_mask  = (1 << i);
        uint32_t currentIO = p_Config->PIN & pin_mask;

        if (currentIO == pin_mask) {
            // Configure MODER
            temp = p_Instance->MODER;
            temp &= ~(GPIO_MODER_MODE0 << (i * 2U));
            temp |= (p_Config->MODE << (i * 2U));
            p_Instance->MODER = temp;

            // Configure OTYPER
            temp = p_Instance->OTYPER;
            temp &= ~(GPIO_OTYPER_OTYPER0 << i);
            temp |= (p_Config->OTYPE << i);
            p_Instance->OTYPER = temp;

            // Configure OSPEEDR
            temp = p_Instance->OSPEEDR;
            temp &= ~(GPIO_OSPEEDR_OSPEEDR0 << (i * 2U));
            temp |= (p_Config->SPD << (i * 2U));
            p_Instance->OSPEEDR = temp;

            // Configure PUPDR
            temp = p_Instance->PUPDR;
            temp &= ~(GPIO_PUPDR_PUPDR0 << (i * 2U));
            temp |= (p_Config->PUPD << (i * 2U));
            p_Instance->PUPDR = temp;

            // Configure Alternate Function
            temp = p_Instance->AFR[i >> 3U];
            temp &= ~(0xFU << (i & 0x07U) * 4U);
            temp |= (p_Config->Alternate << (i & 0x07U) * 4U);
            p_Instance->AFR[i >> 3U] = temp;
        }
    }
    return STATUS_OK;
}

status_t GPIO_WriteOutputPin(GPIO_TypeDef* p_Instance, const uint16_t GPIO_PIN, const gpio_pin_state_t state) {
    assert(p_Instance != NULL);
    if (!p_Instance) return ERR_NULLPTR;

    if (state != GPIO_PIN_RESET) {
        p_Instance->BSRR = (uint32_t)GPIO_PIN;
    } else {
        p_Instance->BSRR = (uint32_t)GPIO_PIN << 16U;
    }
    return STATUS_OK;
}

status_t GPIO_ReadInputPin(GPIO_TypeDef* p_Instance, const uint16_t GPIO_PIN, gpio_pin_state_t* state) {
    assert(p_Instance != NULL);
    assert(state != NULL);

    if (!p_Instance) return ERR_NULLPTR;

    if (READ_BIT(p_Instance->IDR, GPIO_PIN) != GPIO_PIN_RESET) {
        *state = GPIO_PIN_SET;
    } else {
        *state = GPIO_PIN_RESET;
    }
    return STATUS_OK;
}

status_t GPIO_ToggleOutputPin(GPIO_TypeDef* p_Instance, const uint16_t GPIO_PIN) {
    assert(p_Instance != NULL);

    if (!p_Instance) return ERR_NULLPTR;

    if (READ_BIT(p_Instance->ODR, GPIO_PIN)) {
        p_Instance->BSRR = (uint32_t)GPIO_PIN << 16U;
    } else {
        p_Instance->BSRR = GPIO_PIN;
    }

    return STATUS_OK;
}
