#ifndef DMA_TYPES_H
#define DMA_TYPES_H

#include <stdint.h>

#include "DMA_registers.h"

typedef enum { DMA_P1, DMA_P2, DMA_Count } DMA_Num_t;

typedef enum { DMA_CH0, DMA_CH1, DMA_CH2, DMA_CH3, DMA_CH4, DMA_CH5, DMA_CH6, DMA_CH7, DMA_CH_Count } DMA_CH_t;

typedef enum { DMA_SM0, DMA_SM1, DMA_SM2, DMA_SM3, DMA_SM4, DMA_SM5, DMA_SM6, DMA_SM7, DMA_SM_Count } DMA_SM_t;

typedef enum { DMA_PERIPH_TO_MEMORY, DMA_MEMORY_TO_PERIPH, DMA_MEMORY_TO_MEMORY } DMA_Direction_t;

typedef struct {
    DMA_Num_t       DMA_Num;
    DMA_CH_t        DMA_CH;
    DMA_SM_t        DMA_SM;
    DMA_Direction_t Direction;
} DMA_InitTypeDef;

#endif