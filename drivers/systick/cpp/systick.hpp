#pragma once

#include <cstdint>

#include "low-level/systick.h"

class MySysTick {
   private:
    SysTick_TypeDef*      Instance;
    std::atomic<uint32_t> tickCount;

   public:
    MySysTick();
    ~MySysTick();
    void     init();
    void     tick();
    uint32_t get_ticks() const;
    void     delay_ms(uint32_t ms) const;
};

extern "C" void SysTick_Handler();