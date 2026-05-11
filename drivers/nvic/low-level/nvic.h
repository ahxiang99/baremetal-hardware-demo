#ifndef __MY_STM32F4_NVIC_DRIVER_H__
#define __MY_STM32F4_NVIC_DRIVER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define NVIC_BASE (0xE000E100)
#define NVIC_ICER0 ((volatile uint32_t*)0xE000E180)
#define NVIC_ICER1 ((volatile uint32_t*)0xE000E184)
#define NVIC_ISER0 ((volatile uint32_t*)(NVIC_BASE + 0x00U))  // Interrupt Set-Enable Register 0
#define NVIC_ISER1 ((volatile uint32_t*)(NVIC_BASE + 0x04U))  // Interrupt Set-Enable Register 1

/* Custom NVIC Enable Function */
static inline void My_NVIC_EnableIRQ(int8_t IRQn) {
    if (IRQn >= 0 && IRQn <= 31) {
        *NVIC_ISER0 |= (1 << IRQn);
    } else if (IRQn >= 32 && IRQn <= 63) {
        *NVIC_ISER1 |= (1 << (IRQn - 32));
    }
}

static inline void My_NVIC_DisableIRQ(int8_t IRQn) {
    if (IRQn < 0) return;  // Negative numbers are System Exceptions, not IRQs

    if (IRQn <= 31) {
        // Writing a 1 to this bit DISABLES the interrupt
        *NVIC_ICER0 = (1UL << IRQn);
    } else if (IRQn >= 32 && IRQn <= 63) {
        // Offset by 32 for the second register
        *NVIC_ICER1 = (1UL << (IRQn - 32));
    }
}

#ifdef __cplusplus
}
#endif
#endif