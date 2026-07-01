#ifndef SYSCFG_REGISTER_H
#define SYSCFG_REGISTER_H

#include <stdint.h>

#include "low-level/rcc_registers.h"

typedef struct {
    volatile uint32_t MEMRMP;
    volatile uint32_t PMC;
    volatile uint32_t EXTICR1;
    volatile uint32_t EXTICR2;
    volatile uint32_t EXTICR3;
    volatile uint32_t EXTICR4;
    volatile uint32_t CMPCR;
} SYSCFG_TypeDef;

typedef struct {
    volatile uint32_t IMR;
    volatile uint32_t EMR;
    volatile uint32_t RTSR;
    volatile uint32_t FTSR;
    volatile uint32_t SWIER;
    volatile uint32_t PR;
} EXTI_TypeDef;

#define SYSCFG_OFFSET (0x3800U)
#define SYSCFG_BASE (APB2PERIPH_BASE + SYSCFG_OFFSET)
#define SYSCFG ((SYSCFG_TypeDef*)(SYSCFG_BASE))

#define EXTI_OFFSET (0x3C00U)
#define EXTI_BASE (APB2PERIPH_BASE + EXTI_OFFSET)
#define EXTI ((EXTI_TypeDef*)(EXTI_BASE))

#endif