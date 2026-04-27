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
    // Constructor implementation
    switch (GPIO_port) {
        case GPIOA_PORT:
            __LIB_RCC_GPIOA_CLK_ENABLE();
            break;
        case GPIOB_PORT:
            __LIB_RCC_GPIOB_CLK_ENABLE();
            break;
        case GPIOC_PORT:
            __LIB_RCC_GPIOC_CLK_ENABLE();
            break;
        case GPIOD_PORT:
            __LIB_RCC_GPIOD_CLK_ENABLE();
            break;
        case GPIOE_PORT:
            __LIB_RCC_GPIOE_CLK_ENABLE();
            break;
        case GPIOH_PORT:
            __LIB_RCC_GPIOH_CLK_ENABLE();
            break;
        default:
            // Handle invalid port
            break;
    }
    uint32_t position;
    uint32_t temp       = 0x0U;
    uint32_t ioposition = 0x00U;
    uint32_t iocurrent  = 0x00U;
    for (position = 0U; position < GPIO_NUMBER; position++) {
        ioposition = 0x01U << position;                       /* Get the IO position = 00X0 0000 */
        iocurrent  = (uint32_t)(GPIO_Init->Pin) & ioposition; /* Get the current IO position = 00X0 0000 */

        if (iocurrent == ioposition) {
            /* Check if Alternate function mode is selected */
            if ((GPIO_Init->Mode == GPIO_MODE_ALTFN_PP) || (GPIO_Init->Mode == GPIO_MODE_ALTFN_OD)) {
                temp = GPIOx->AFR[position >> 3U];
                temp &= ~(0xFU << ((position & 0x07U) * 4U));
                temp |= GPIO_Init->Alternate << ((position & 0x07U) * 4U);
                GPIOx->AFR[position >> 3U] = temp;
            }

            /* Configure IO Direction mode (Input, Output, Alternate or Analog) */
            temp = GPIOx->MODER;
            temp &= ~(GPIO_MODER_MODE0 << (position * 2U));
            temp |= ((GPIO_Init->Mode & GPIO_MODE) << (position * 2U));
            GPIOx->MODER = temp;

            /* Alternate Function Configuration */
            if (GPIO_Init->Mode == GPIO_MODE_OUTPUT_PP || GPIO_Init->Mode == GPIO_MODE_OUTPUT_OD || GPIO_Init->Mode == GPIO_MODE_ALTFN_PP || GPIO_Init->Mode == GPIO_MODE_ALTFN_OD) {
                temp = GPIOx->OSPEEDR;
                temp &= ~(GPIO_OSPEEDR_OSPEED0 << (position * 2U));
                temp |= (GPIO_Init->Speed << (position * 2U));
                GPIOx->OSPEEDR = temp;

                temp           = GPIOx->OTYPER;
                temp &= ~(GPIO_OTYPER_OT0 << (position * 1U));
                temp |= (((GPIO_Init->Mode & GPIO_OUTPUT_TYPE) >> 4U) << (position * 2U));
                GPIOx->OTYPER = temp;
            }

            /* Configure Pull-up or Pull down for the current IO */
            temp = GPIOx->PUPDR;
            temp &= ~(GPIO_PUPDR_PUPDR0 << (position * 2U));
            temp |= ((GPIO_Init->Pull) << (position * 2U));
            GPIOx->PUPDR = temp;
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

void Gpio::LIB_GPIO_WritePin(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin, GPIO_PinStateTypeDef PinState) {
    if (PinState != GPIO_PIN_RESET) {
        GPIOx->BSRR = GPIO_Pin; /* Set the selected data port bits */
    } else {
        GPIOx->BSRR = (uint32_t)GPIO_Pin << 16U; /* Reset the selected data port bits */
    }
}

void Gpio::LIB_GPIO_TogglePin(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin) {
    GPIOx->ODR ^= GPIO_Pin; /* Toggle the selected data port bits */
}

void Gpio::__LIB_RCC_GPIOA_CLK_ENABLE() const {
    RCC->AHB1ENR |= GPIOAEN; /* Enable the GPIOA clock */
}
void Gpio::__LIB_RCC_GPIOB_CLK_ENABLE() const {
    RCC->AHB1ENR |= GPIOBEN; /* Enable the GPIOB clock */
}
void Gpio::__LIB_RCC_GPIOC_CLK_ENABLE() const {
    RCC->AHB1ENR |= GPIOCEN; /* Enable the GPIOC clock */
}
void Gpio::__LIB_RCC_GPIOD_CLK_ENABLE() const {
    RCC->AHB1ENR |= GPIODEN; /* Enable the GPIOD clock */
}
void Gpio::__LIB_RCC_GPIOE_CLK_ENABLE() const {
    RCC->AHB1ENR |= GPIOEEN; /* Enable the GPIOE clock */
}
void Gpio::__LIB_RCC_GPIOH_CLK_ENABLE() const {
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