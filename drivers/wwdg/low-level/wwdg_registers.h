#pragma once

#include <stdint.h>

#include "low-level/rcc_registers.h"

#define __IO volatile

typedef struct {
    __IO uint32_t CR;
    __IO uint32_t CFR;
    __IO uint32_t SR;
} WWDG_TypeDef;

#define WWDG_OFFSET 0x2C00U
#define WWDG_BASE ((WWDG_TypeDef*)(APB1PERIPH_BASE + WWDG_OFFSET))