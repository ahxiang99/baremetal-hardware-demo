#include "systick.hpp"

#include <cstdint>

#include "low-level/nvic.h"

extern MySysTick my_systick;

MySysTick::MySysTick() {}

void MySysTick::init() {
    // Construct 1ms heartbeat
    SysTick->LOAD = 16000 - 1;  // Set reload register
    SysTick->VAL  = 0;          // Clear current value register
    SysTick->CTRL = 0x07U;      // Enable SysTick, use processor clock, no interrupt
    My_NVIC_EnableIRQ(15);
}

MySysTick::~MySysTick() {
    SysTick->CTRL = 0x00U;  // Disable SysTick
}

uint32_t MySysTick::get_ticks() const {
    return tickCount;
}

void MySysTick::tick() {
    tickCount = tickCount + 1;
}

void MySysTick::delay_ms(uint32_t ms) const {
    uint32_t _start = tickCount;
    while ((tickCount - _start) < ms);
}

extern "C" void SysTick_Handler() {
    my_systick.tick();
}