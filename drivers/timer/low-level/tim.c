#include "tim.h"

#include "bit_utils.h"
#include "low-level/gpio.h"
#include "low-level/nvic.h"
#include "low-level/rcc.h"
#include "tim_bitfields.h"
#include "tim_registers.h"
#include "tim_types.h"

void TIM_Init(TIM_TypeDef* instance, TIM_InitTypeDef* pConfig) {
    if (!instance || !pConfig) return;

    // 1. Enable Timer Clock
    if (pConfig->Instance >= TIMER_COUNT) return;
    SET_BIT(*tim_table[pConfig->Instance].RCC_APBx, tim_table[pConfig->Instance].RCC_APBXENR_BIT);

    // 2. Configure Timer
    // Config
    uint32_t tmp = instance->CR1;
    tmp &= ~(TIM_CR1_DIR | TIM_CR1_CMS | TIM_CR1_CKD | TIM_CR1_ARPE);  // Clear bits
    tmp |= pConfig->CounterMode | pConfig->ClockDivision | pConfig->AutoReloadPreload;
    instance->CR1 = tmp;

    // Counter
    instance->ARR = pConfig->Counter;
    // Prescaler
    instance->PSC = pConfig->Prescaler;

    // 3. Clear pending flags and enable Update Interrupt
    SET_BIT(instance->EGR, TIM_EGR_UG);   // Force update to load PSC/ARR
    CLEAR_BIT(instance->SR, TIM_SR_UIF);  // Clear the flag that UG just set
    SET_BIT(instance->DIER, TIM_DIER_UIE);

    // Enable Interrupt
    My_NVIC_EnableIRQ(29);
    SET_BIT(instance->CR1, TIM_CR1_CEN);
}
