#pragma once

#define __IO volatile

#include <stdint.h>

#include "low-level/rcc_registers.h"

typedef struct {
    __IO uint32_t TR;
    __IO uint32_t DR;
    __IO uint32_t CR;
    __IO uint32_t ISR;
    __IO uint32_t PRER;
    __IO uint32_t WUTR;
    __IO uint32_t CALIBR;
    __IO uint32_t ALRMAR;
    __IO uint32_t ALRMBR;
    __IO uint32_t WPR;
    __IO uint32_t SSR;
    __IO uint32_t TSTR;
    __IO uint32_t TSDR;
    __IO uint32_t TSSSR;
    __IO uint32_t CALR;
    __IO uint32_t TAFCR;
    __IO uint32_t ALRMASSR;
    __IO uint32_t ALRMBSSR;
    __IO uint32_t BKP0R[19];
} RTC_TypeDef;

#define RTC_OFFSET (0x2800U)
#define RTC ((RTC_TypeDef*)(APB1PERIPH_BASE + RTC_OFFSET))