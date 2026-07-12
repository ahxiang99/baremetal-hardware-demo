#pragma once

#include "low-level/rcc_registers.h"

#define __IO volatile

typedef struct {
    __IO uint32_t CR1;
    __IO uint32_t CR2;
    __IO uint32_t SR;
    __IO uint32_t DR;
    __IO uint32_t CRC_PR;
    __IO uint32_t RXCRCR;
    __IO uint32_t TXCRCR;
    __IO uint32_t I2SCFGR;
    __IO uint32_t I2SPR;
} SPI_TypeDef;

#define SPI1 ((SPI_TypeDef*)(APB2PERIPH_BASE + 0x3000U))
#define SPI2 ((SPI_TypeDef*)(APB1PERIPH_BASE + 0x3800U))
#define SPI3 ((SPI_TypeDef*)(APB1PERIPH_BASE + 0x3C00U))
#define SPI4 ((SPI_TypeDef*)(APB2PERIPH_BASE + 0x3400U))