#ifndef GPIO_REGISTERS_H
#define GPIO_REGISTERS_H

#define GPIOA_BASE (AHB1PERIPH_BASE + 0x0000U)
#define GPIOB_BASE (AHB1PERIPH_BASE + 0x0400U)
#define GPIOC_BASE (AHB1PERIPH_BASE + 0x0800U)
#define GPIOD_BASE (AHB1PERIPH_BASE + 0x0C00U)
#define GPIOE_BASE (AHB1PERIPH_BASE + 0x1000U)
#define GPIOH_BASE (AHB1PERIPH_BASE + 0x1C00U)

#include <stdint.h>

#define __IO volatile

typedef struct {
    __IO uint32_t MODER;   /*!< GPIO port mode register,               Address offset: 0x00      */
    __IO uint32_t OTYPER;  /*!< GPIO port output type register,        Address offset: 0x04      */
    __IO uint32_t OSPEEDR; /*!< GPIO port output speed register,       Address offset: 0x08      */
    __IO uint32_t PUPDR;   /*!< GPIO port pull-up/pull-down register,  Address offset: 0x0C      */
    __IO uint32_t IDR;     /*!< GPIO port input data register,         Address offset: 0x10      */
    __IO uint32_t ODR;     /*!< GPIO port output data register,        Address offset: 0x14      */
    __IO uint32_t BSRR;    /*!< GPIO port bit set/reset register,      Address offset: 0x18      */
    __IO uint32_t LCKR;    /*!< GPIO port configuration lock register, Address offset: 0x1C      */
    __IO uint32_t AFR[2];  /*!< GPIO altern ate function registers,      Address offset: 0x20-0x24 */
} GPIO_TypeDef;

/* GPIOx Base Address*/
#define GPIOA ((GPIO_TypeDef*)GPIOA_BASE)
#define GPIOB ((GPIO_TypeDef*)GPIOB_BASE)
#define GPIOC ((GPIO_TypeDef*)GPIOC_BASE)
#define GPIOD ((GPIO_TypeDef*)GPIOD_BASE)
#define GPIOE ((GPIO_TypeDef*)GPIOE_BASE)
#define GPIOH ((GPIO_TypeDef*)GPIOH_BASE)

#endif