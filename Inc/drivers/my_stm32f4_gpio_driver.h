#ifndef __MY_STM32F4_GPIO_DRIVER_H__
#define __MY_STM32F4_GPIO_DRIVER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <cstdint>
#define PERIPH_BASE 0X40000000U
#define AHB1PERIPH_BASE (PERIPH_BASE + 0x00020000U)
#define RCC_BASE (AHB1PERIPH_BASE + 0x3800U)

#define GPIOA_BASE (AHB1PERIPH_BASE + 0x0000U)
#define GPIOB_BASE (AHB1PERIPH_BASE + 0x0400U)
#define GPIOC_BASE (AHB1PERIPH_BASE + 0x0800U)
#define GPIOD_BASE (AHB1PERIPH_BASE + 0x0C00U)
#define GPIOE_BASE (AHB1PERIPH_BASE + 0x1000U)
#define GPIOH_BASE (AHB1PERIPH_BASE + 0x1C00U)

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
    __IO uint32_t AFR[2];  /*!< GPIO alternate function registers,      Address offset: 0x20-0x24 */
} GPIO_TypeDef;

typedef struct {
    __IO uint32_t CR;           /*!< RCC clock control register,             Address offset: 0x00 */
    __IO uint32_t PLLCFGR;      /*!< RCC PLL configuration register,         Address offset: 0x04 */
    __IO uint32_t CFGR;         /*!< RCC clock configuration register,       Address offset: 0x08 */
    __IO uint32_t CIR;          /*!< RCC clock interrupt register,           Address offset: 0x0C */
    __IO uint32_t AHB1RSTR;     /*!< RCC AHB1 peripheral reset register,     Address offset: 0x10 */
    __IO uint32_t AHB2RSTR;     /*!< RCC AHB2 peripheral reset register,     Address offset: 0x14 */
    uint32_t      RESERVED0;    /*!< Reserved, Address offset: 0x18                               */
    uint32_t      RESERVED1;    /*!< Reserved, Address offset: 0x1C                               */
    __IO uint32_t APB1RSTR;     /*!< RCC APB1 peripheral reset register,     Address offset: 0x20 */
    __IO uint32_t APB2RSTR;     /*!< RCC APB2 peripheral reset register,     Address offset: 0x24 */
    uint32_t      RESERVED2[2]; /*!< Reserved, Address offset: 0x28-0x2C                       */
    __IO uint32_t AHB1ENR;      /*!< RCC AHB1 peripheral clock enable register,  Address offset: 0x30 */
    __IO uint32_t AHB2ENR;      /*!< RCC AHB2 peripheral clock enable register,  Address offset: 0x34 */
    uint32_t      RESERVED3;    /*!< Reserved, Address offset: 0x38                               */
    uint32_t      RESERVED4;    /*!< Reserved, Address offset: 0x3C                               */
    __IO uint32_t APB1ENR;      /*!< RCC APB1 peripheral clock enable register,  Address offset: 0x40 */
    __IO uint32_t APB2ENR;      /*!< RCC APB2 peripheral clock enable register,  Address offset: 0x44 */
    uint32_t      RESERVED5[2]; /*!< Reserved, Address offset: 0x48-0x4C                       */
    __IO uint32_t AHB1LPENR;    /*!< RCC AHB1 peripheral clock enable in low power mode register,
                                   Address offset: 0x50 */
    __IO uint32_t AHB2LPENR;    /*!< RCC AHB2 peripheral clock enable in low power mode register,
                                   Address offset: 0x54 */
    uint32_t      RESERVED6;    /*!< Reserved, Address offset: 0x58                               */
    uint32_t      RESERVED7;    /*!< Reserved, Address offset: 0x5C                               */
    __IO uint32_t APB1LPENR;    /*!< RCC APB1 peripheral clock enable in low power mode register,
                                   Address offset: 0x60 */
    __IO uint32_t APB2LPENR;    /*!< RCC APB2 peripheral clock enable in low power mode register,
                                   Address offset: 0x64 */
    uint32_t      RESERVED8[2]; /*!< Reserved, Address offset: 0x68-0x6C                       */
    __IO uint32_t BDCR;         /*!< RCC Backup domain control register,      Address offset: 0x70 */
    __IO uint32_t CSR;          /*!< RCC clock control & status register,       Address offset: 0x74 */
    uint32_t      RESERVED9[2]; /*!< Reserved, Address offset: 0x78-0x7C                       */
    __IO uint32_t SSCGR;        /*!< RCC spread spectrum clock generation register, Address offset: 0x80 */
    __IO uint32_t PLLI2SCFGR;   /*!< RCC PLLI2S configuration register, Address offset: 0x84 */
    uint32_t      RESERVED10;   /*!< Reserved, Address offset: 0x88                               */
    __IO uint32_t DCKCFGR;      /*!< RCC Dedicated Clocks Configuration Register, Address offset: 0x8C */

} RCC_TypeDef;

#define RCC ((RCC_TypeDef*)RCC_BASE)
#define GPIOA ((GPIO_TypeDef*)GPIOA_BASE)
#define GPIOB ((GPIO_TypeDef*)GPIOB_BASE)
#define GPIOC ((GPIO_TypeDef*)GPIOC_BASE)
#define GPIOD ((GPIO_TypeDef*)GPIOD_BASE)
#define GPIOE ((GPIO_TypeDef*)GPIOE_BASE)
#define GPIOH ((GPIO_TypeDef*)GPIOH_BASE)

/* GPIO Bit Mask */
#define GPIOAEN (1 << 0)
#define GPIOBEN (1 << 1)
#define GPIOCEN (1 << 2)
#define GPIODEN (1 << 3)
#define GPIOEEN (1 << 4)
#define GPIOHEN (1 << 7)

/* GPIO PIN Definition */
#define GPIO_PIN_O ((uint16_t)0x0001)   /* Pin 0 selected    */
#define GPIO_PIN_1 ((uint16_t)0x0002)   /* Pin 1 selected    */
#define GPIO_PIN_2 ((uint16_t)0x0004)   /* Pin 2 selected    */
#define GPIO_PIN_3 ((uint16_t)0x0008)   /* Pin 3 selected    */
#define GPIO_PIN_4 ((uint16_t)0x0010)   /* Pin 4 selected    */
#define GPIO_PIN_5 ((uint16_t)0x0020)   /* Pin 5 selected    */
#define GPIO_PIN_6 ((uint16_t)0x0040)   /* Pin 6 selected    */
#define GPIO_PIN_7 ((uint16_t)0x0080)   /* Pin 7 selected    */
#define GPIO_PIN_8 ((uint16_t)0x0100)   /* Pin 8 selected    */
#define GPIO_PIN_9 ((uint16_t)0x0200)   /* Pin 9 selected    */
#define GPIO_PIN_10 ((uint16_t)0x0400)  /* Pin 10 selected   */
#define GPIO_PIN_11 ((uint16_t)0x0800)  /* Pin 11 selected   */
#define GPIO_PIN_12 ((uint16_t)0x1000)  /* Pin 12 selected   */
#define GPIO_PIN_13 ((uint16_t)0x2000)  /* Pin 13 selected   */
#define GPIO_PIN_14 ((uint16_t)0x4000)  /* Pin 14 selected   */
#define GPIO_PIN_15 ((uint16_t)0x8000)  /* Pin 15 selected   */
#define GPIO_PIN_ALL ((uint16_t)0xFFFF) /* All pins selected */

#define GPIO_MODE_INPUT 0x00U
#define GPIO_MODE_OUTPUT 0x01U
#define GPIO_MODE_ALTFN 0x02U
#define GPIO_MODE_ANALOG 0x03U

#define GPIO_NOPULL 0x00U
#define GPIO_PULLUP 0x01U
#define GPIO_PULLDOWN 0x02U

#ifdef __cplusplus
}
#endif
#endif /* __MY_STM32F4_GPIO_DRIVER_H */