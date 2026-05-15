#ifndef TIM_H
#define TIM_H

#include "tim_bitfields.h"
#include "tim_registers.h"
#include "tim_types.h"

#ifdef __cplusplus
extern "C" {
#endif

void TIM_Init(TIM_TypeDef* instance, TIM_InitTypeDef* pConfig);
void TIM3_IRQHandler(void);

#ifdef __cplusplus
}
#endif

#endif