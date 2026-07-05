#pragma once
#include "low-level/rcc_registers.h"
#define __IO volatile

typedef struct {
    __IO uint32_t ACR;
    __IO uint32_t KEYR;
    __IO uint32_t OPTKEYR;
    __IO uint32_t SR;
    __IO uint32_t CR;
    __IO uint32_t OPTCR;
} Flash_TypeDef;

#define FLASH_OFFSET (0x3C00U)
#define FLASH_BASE (AHB1PERIPH_BASE + FLASH_OFFSET)
#define FLASH ((Flash_TypeDef*)(FLASH_BASE))