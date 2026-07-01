#include "systick.hpp"

#include "FreeRTOS.h"
#include "pch.hpp"
#include "task.h"

MySysTick::MySysTick() {}

void MySysTick::init() {
    // Construct 1ms heartbeat
    SysTick->LOAD = (HSI_Freq_Hz / 1000) - 1;  // Set reload register
    SysTick->VAL  = 0;                         // Clear current value register
    SysTick->CTRL = 0x07U;                     // Enable SysTick, use processor clock, no interrupt
    My_NVIC_EnableIRQ(15);
}

MySysTick::~MySysTick() {
    SysTick->CTRL = 0x00U;  // Disable SysTick
}

uint32_t MySysTick::get_ticks() const {
    return tickCount.load(std::memory_order_relaxed);
}

void MySysTick::tick() {
    tickCount++;
}

void MySysTick::delay_ms(uint32_t ms) const {
    uint32_t _start = tickCount.load(std::memory_order_relaxed);
    while ((tickCount.load(std::memory_order_relaxed) - _start) < ms);
}

extern "C" void                xPortSysTickHandler(void);

extern "C" volatile BaseType_t xTaskSchedulerRunning;  // FreeRTOS internal flag

extern "C" void                SysTick_Handler() {
    if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED) {
        xPortSysTickHandler();  // ← only call when scheduler is running
    }
    getDrivers().my_systick.tick();
}