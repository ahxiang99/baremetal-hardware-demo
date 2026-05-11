#ifndef RCC_REGISTERS_H
#define RCC_REGISTERS_H

#include "rcc.h"

#define PERIPH_BASE 0X40000000U
#define AHB1PERIPH_BASE (PERIPH_BASE + 0x00020000U)
#define APB1PERIPH_BASE (PERIPH_BASE + 0x00000000U)
#define APB2PERIPH_BASE (PERIPH_BASE + 0x00010000U)
#define RCC_BASE (AHB1PERIPH_BASE + 0x3800U)
#define RCC ((RCC_TypeDef*)RCC_BASE)

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

#endif