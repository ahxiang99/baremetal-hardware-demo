#ifndef TIM_REGISTERS_H
#define TIM_REGISTERS_H

#include <stdint.h>

#include "low-level/rcc_registers.h"

typedef struct {
    volatile uint32_t CR1;
    volatile uint32_t CR2;
    volatile uint32_t SMCR;
    volatile uint32_t DIER;
    volatile uint32_t SR;
    volatile uint32_t EGR;
    volatile uint32_t CCMR1;
    volatile uint32_t CCMR2;
    volatile uint32_t CCER;
    volatile uint32_t CNT;
    volatile uint32_t PSC;
    volatile uint32_t ARR;
    uint32_t          Reserved;
    volatile uint32_t CCR1;
    volatile uint32_t CCR2;
    volatile uint32_t CCR3;
    volatile uint32_t CCR4;
    uint32_t          Reserved_1;
    volatile uint32_t DCR;
    volatile uint32_t DMAR;
    volatile uint32_t TIM2_OR;
    volatile uint32_t TIM5_OR;
} TIM_TypeDef;

#define TIM1 ((TIM_TypeDef*)(APB2PERIPH_BASE + 0x0000U))
#define TIM2 ((TIM_TypeDef*)(APB1PERIPH_BASE + 0x0000U))
#define TIM3 ((TIM_TypeDef*)(APB1PERIPH_BASE + 0x0400U))
#define TIM4 ((TIM_TypeDef*)(APB1PERIPH_BASE + 0x0800U))
#define TIM5 ((TIM_TypeDef*)(APB1PERIPH_BASE + 0x0C00U))

#define TIM8 ((TIM_TypeDef*)(APB2PERIPH_BASE + 0x0400U))
#define TIM9 ((TIM_TypeDef*)(APB2PERIPH_BASE + 0x4000U))
#define TIM10 ((TIM_TypeDef*)(APB2PERIPH_BASE + 0x4400U))
#define TIM11 ((TIM_TypeDef*)(APB2PERIPH_BASE + 0x4800U))

#endif