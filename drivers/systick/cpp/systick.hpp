#ifndef SYSTICK_HPP
#define SYSTICK_HPP

#include <sys/types.h>

#include <cstdint>

#include "low-level/systick.h"

class MySysTick {
   private:
    constexpr static uint32_t SYSTICK_CLK_FREQ = 16000000;  // Assuming 16 MHz clock
    volatile uint32_t         tickCount;

   public:
    MySysTick();
    ~MySysTick();
    void     init();
    void     tick();
    uint32_t get_ticks() const;
    void     delay_ms(uint32_t ms) const;
};

extern MySysTick timer;

extern "C" void  SysTick_Handler();

#endif  // SYSTICK_HPP
