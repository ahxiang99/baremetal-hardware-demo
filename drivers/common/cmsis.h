#pragma once

static inline void __enable_irq(void) {
    __asm volatile("cpsie i" : : : "memory");
}

static inline void __disable_irq(void) {
    __asm volatile("cpsid i" : : : "memory");  // Disable interrupts (IRQ)
}