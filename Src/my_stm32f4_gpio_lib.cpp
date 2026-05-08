#include "Inc/libs/my_stm32f4_gpio_lib.h"

#include <stdint.h>
#include <sys/types.h>

#define GPIO_NUMBER 16U
#define GPIO_MODER_MODE0 0x03U << 0U
#define GPIO_MODE 0x03U
#define GPIO_PUPDR_PUPDR0 0x03U << 0U
#define GPIO_OSPEEDR_OSPEED0 0x03U << 0U
#define GPIO_OTYPER_OT0 0x01U << 0U
#define GPIO_OUTPUT_TYPE 0x10U

Gpio::Gpio(PORT_NameType port, GPIO_TypeDef* GPIOx, GPIO_InitTypeDef* GPIO_Init) : GPIO_port(port), GPIO_regsStruct(*GPIOx), GPIO_initStruct(*GPIO_Init) {
    // Constructor implementation2
    switch (GPIO_port) {
        case GPIOA_PORT:
            LIB_RCC_GPIOA_CLK_ENABLE();
            break;
        case GPIOB_PORT:
            LIB_RCC_GPIOB_CLK_ENABLE();
            break;
        case GPIOC_PORT:
            LIB_RCC_GPIOC_CLK_ENABLE();
            break;
        case GPIOD_PORT:
            LIB_RCC_GPIOD_CLK_ENABLE();
            break;
        case GPIOE_PORT:
            LIB_RCC_GPIOE_CLK_ENABLE();
            break;
        case GPIOH_PORT:
            LIB_RCC_GPIOH_CLK_ENABLE();
            break;
        default:
            // Handle invalid port
            break;
    }

    for (uint8_t i = 0; i < GPIO_NUMBER; ++i) {
        uint32_t pinMask    = (1U << i);
        uint32_t current_IO = GPIO_initStruct.Pin & pinMask;

        if (current_IO == pinMask) {
            LIB_GPIO_WriteRegister(GPIOx, GPIO_MODER, i, GPIO_initStruct.Mode);
            LIB_GPIO_WriteRegister(GPIOx, GPIO_PULL, i, GPIO_initStruct.Pull);
            LIB_GPIO_WriteRegister(GPIOx, GPIO_OSPEEDR, i, GPIO_initStruct.Speed);
            LIB_GPIO_WriteRegister(GPIOx, GPIO_ALTFN, i, GPIO_initStruct.Alternate);
            LIB_GPIO_WriteRegister(GPIOx, GPIO_OTYPER, i, GPIO_initStruct.OutputType);
        }
    }
}

GPIO_PinStateTypeDef Gpio::LIB_GPIO_ReadPin(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin) {
    GPIO_PinStateTypeDef bitstatus;

    if ((GPIOx->IDR & GPIO_Pin) != (uint32_t)GPIO_PIN_RESET) {
        bitstatus = GPIO_PIN_SET;
    } else {
        bitstatus = GPIO_PIN_RESET;
    }
    return bitstatus;
}

void Gpio::LIB_GPIO_WriteOutputPin(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin, GPIO_PinStateTypeDef PinState) {
    if (PinState != GPIO_PIN_RESET) {
        GPIOx->BSRR = GPIO_Pin; /* Set the selected data port bits */
    } else {
        GPIOx->BSRR = (uint32_t)GPIO_Pin << 16U; /* Reset the selected data port bits */
    }
}

void Gpio::LIB_GPIO_WriteRegister(GPIO_TypeDef* GPIOx, uint32_t Register, uint32_t GPIO_Pin, uint32_t PinState) {
    switch (Register) {
        case GPIO_MODER:                                            /* Read the current mode register value */
            GPIOx->MODER &= ~(GPIO_MODER_MODE0 << (GPIO_Pin * 2U)); /* Clear the mode bits for the selected pin */
            GPIOx->MODER |= (PinState << (GPIO_Pin * 2U));          /* Set the mode bits for the selected pin */
            break;
        case GPIO_PULL:
            GPIOx->PUPDR &= ~(GPIO_PUPDR_PUPDR0 << GPIO_Pin * 2U); /* Clear the pull-up/pull-down bits for the selected pin */
            GPIOx->PUPDR |= (PinState << GPIO_Pin * 2U);           /* Set the pull-up/pull-down bits for the selected pin */
            break;
        case GPIO_OSPEEDR:
            GPIOx->OSPEEDR &= ~(GPIO_OSPEEDR_OSPEED0 << GPIO_Pin * 2U); /* Clear the speed bits for the selected pin */
            GPIOx->OSPEEDR |= (PinState << GPIO_Pin * 2U);              /* Set the speed bits for the selected pin */
            break;
        case GPIO_ALTFN:
            // Implementation for alternate function register
            GPIOx->AFR[GPIO_Pin >> 3U] &= ~(0xFU << (GPIO_Pin & 0x07U) * 4U);    /* Clear the alternate function bits for the selected pin */
            GPIOx->AFR[GPIO_Pin >> 3U] |= PinState << ((GPIO_Pin & 0x07U) * 4U); /* Set the alternate function bits for the selected pin */
            break;
        case GPIO_OTYPER:
            GPIOx->OTYPER &= (PinState << GPIO_Pin);
            GPIOx->OTYPER |= (PinState << GPIO_Pin);
            break;
    }
}

void Gpio::LIB_GPIO_TogglePin(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin) {
    GPIOx->ODR ^= GPIO_Pin; /* Toggle the selected data port bits */
}

void Gpio::LIB_RCC_GPIOA_CLK_ENABLE() const {
    RCC->AHB1ENR |= GPIOAEN; /* Enable the GPIOA clock */
}
void Gpio::LIB_RCC_GPIOB_CLK_ENABLE() const {
    RCC->AHB1ENR |= GPIOBEN; /* Enable the GPIOB clock */
}
void Gpio::LIB_RCC_GPIOC_CLK_ENABLE() const {
    RCC->AHB1ENR |= GPIOCEN; /* Enable the GPIOC clock */
}
void Gpio::LIB_RCC_GPIOD_CLK_ENABLE() const {
    RCC->AHB1ENR |= GPIODEN; /* Enable the GPIOD clock */
}
void Gpio::LIB_RCC_GPIOE_CLK_ENABLE() const {
    RCC->AHB1ENR |= GPIOEEN; /* Enable the GPIOE clock */
}
void Gpio::LIB_RCC_GPIOH_CLK_ENABLE() const {
    RCC->AHB1ENR |= GPIOHEN; /* Enable the GPIOH clock */
}

GPIO_InitTypeDef __GPIO_PIN_PARAMS(pinDataType _Pin, pinDataType _Mode, pinDataType _Pull, pinDataType _Speed, pinDataType _Alternate) {
    GPIO_InitTypeDef GPIO_InitStruct;
    GPIO_InitStruct.Pin       = _Pin;
    GPIO_InitStruct.Mode      = _Mode;
    GPIO_InitStruct.Pull      = _Pull;
    GPIO_InitStruct.Speed     = _Speed;
    GPIO_InitStruct.Alternate = _Alternate;
    return GPIO_InitStruct;
}