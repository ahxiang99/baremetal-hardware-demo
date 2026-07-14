#pragma once

#define __IO volatile

#include <stdint.h>

#include "low-level/rcc_registers.h"

typedef struct {
    __IO uint32_t CR;
    __IO uint32_t CSR;
} PWR_TypeDef;

#define PWR_OFFSET (0x7000U)

#define _PWR ((PWR_TypeDef*)(APB1PERIPH_BASE + PWR_OFFSET))