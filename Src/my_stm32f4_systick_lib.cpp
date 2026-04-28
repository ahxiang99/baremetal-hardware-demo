#include "Inc/libs/my_stm32f4_systick_lib.h"

#include "my_stm32f4_systick_driver.h"

MySysTick::MySysTick(uint32_t _seconds) {
    LIB_SET_TICKCOUNT(_seconds);

    SysTick->LOAD = tickCount - 1;  // Set reload register
    SysTick->VAL  = 0;              // Clear current value register
    SysTick->CTRL = 0x05U;          // Enable SysTick, use processor clock, no interrupt

    while (!(SysTick->CTRL & 0x10000)) {
        // Wait until the COUNTFLAG is set
    }
    SysTick->CTRL = 0x00U;  // Disable SysTick
}

void MySysTick::LIB_SET_TICKCOUNT(uint32_t seconds) {
    tickCount = seconds * SYSTICK_CLK_FREQ;
}