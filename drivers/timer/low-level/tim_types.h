#ifndef TIM_TYPES_H
#define TIM_TYPES_H

#include <stdint.h>

#include "low-level/rcc_bitfields.h"
#include "low-level/rcc_registers.h"
#include "tim_bitfields.h"
#include "tim_registers.h"

typedef enum { APB2_TIMER_1, APB1_TIMER_2, APB1_TIMER_3, APB1_TIMER_4, APB1_TIMER_5, APB2_TIMER_9, APB2_TIMER_10, APB2_TIMER_11, TIMER_COUNT } TIM_Num_t;

typedef enum { TIM_COUNTERMODE_UP, TIM_COUNTERMODE_DOWN, TIM_COUNTERMODE_CENTERALIGNED1, TIM_COUNTERMODE_CENTERALIGNED2, TIM_COUNTERMODE_CENTERALIGNED3 } TIM_CounterMode_t;

typedef enum { TIM_CLOCKDIVISION_DIV1 = (0 << 8), TIM_CLOCKDIVISION_DIV2 = (1 << 8), TIM_CLOCKDIVISION_DIV4 = (2 << 8) } TIM_ClockDivision_t;
typedef enum { TIM_AUTORELOAD_PRELOAD_DISABLE, TIM_AUTORELOAD_PRELOAD_ENABLE = TIM_CR1_ARPE } TIM_AutoReloadPreload_t;

typedef struct {
    TIM_TypeDef*       instance;
    volatile uint32_t* RCC_APBx;
    uint32_t           RCC_APBXENR_BIT;
    uint32_t           RCC_APBXRSTR_BIT;
} tim_info_t;

static const tim_info_t tim_table[TIMER_COUNT] = {
    {TIM1,  &(RCC->APB2ENR), RCC_APB2ENR_TIM1_EN,  RCC_APB2RSTR_TIM1_RST },
    {TIM2,  &(RCC->APB1ENR), RCC_APB1ENR_TIM2_EN,  RCC_APB1RSTR_TIM2_RST },
    {TIM3,  &(RCC->APB1ENR), RCC_APB1ENR_TIM3_EN,  RCC_APB1RSTR_TIM3_RST },
    {TIM4,  &(RCC->APB1ENR), RCC_APB1ENR_TIM4_EN,  RCC_APB1RSTR_TIM4_RST },
    {TIM5,  &(RCC->APB1ENR), RCC_APB1ENR_TIM5_EN,  RCC_APB1RSTR_TIM5_RST },
    {TIM9,  &(RCC->APB2ENR), RCC_APB2ENR_TIM9_EN,  RCC_APB2RSTR_TIM9_RST },
    {TIM10, &(RCC->APB2ENR), RCC_APB2ENR_TIM10_EN, RCC_APB2RSTR_TIM10_RST},
    {TIM11, &(RCC->APB2ENR), RCC_APB2ENR_TIM11_EN, RCC_APB2RSTR_TIM11_RST},
};

typedef struct {
    TIM_Num_t               Instance;
    uint32_t                Counter;
    uint32_t                Prescaler;
    TIM_CounterMode_t       CounterMode;
    TIM_ClockDivision_t     ClockDivision;
    TIM_AutoReloadPreload_t AutoReloadPreload;
} TIM_InitTypeDef;

#endif