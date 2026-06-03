#ifndef DMA_REGISTERS_H
#define DMA_REGISTERS_H

#include <stdint.h>

#include "low-level/rcc_registers.h"

typedef struct {
    volatile uint32_t CR;    // Address Offset: 0x00
    volatile uint32_t NDTR;  // Address Offset: 0x04
    volatile uint32_t PAR;   // Address Offset: 0x08
    volatile uint32_t M0AR;  // Address Offset: 0x0C
    volatile uint32_t M1AR;  // Address Offset: 0x10
    volatile uint32_t FCR;   // Address Offset: 0x14
} DMA_Stream_TypeDef;

typedef struct {
    volatile uint32_t  LISR;    // Address Offset: 0x00
    volatile uint32_t  HISR;    // Address Offset: 0x04
    volatile uint32_t  LIFCR;   // Address Offset: 0x08
    volatile uint32_t  HIFCR;   // Address Offset: 0x0C
    DMA_Stream_TypeDef SMx[8];  // Address Offset: 0x10
} DMA_TypeDef;

#define DMA1 ((DMA_TypeDef*)(AHB1PERIPH_BASE + 0x6000))

#endif