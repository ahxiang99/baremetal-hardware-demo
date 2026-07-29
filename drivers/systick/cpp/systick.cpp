#include "systick.hpp"

#include "pch.hpp"

MySysTick::MySysTick() : Instance(SysTickPtr)
{
}

Result<> MySysTick::initialize() const
{
	// Construct 1ms heartbeat
	Instance->LOAD = (getDrivers().sysclock.getSysClock().sysclk / 1000) - 1; // Set reload register
	Instance->VAL = 0;                                                        // Clear current value register
	Instance->CTRL = SYST_CSR_ENABLE | SYST_CSR_TICKINT | SYST_CSR_CLKSOURCE; // Enable SysTick, use processor clock, no interrupt
	My_NVIC_EnableIRQ(15);
	return Ok();
}

MySysTick::~MySysTick()
{
	Instance->CTRL = 0x00U; // Disable SysTick
}

uint32_t MySysTick::get_ticks() const
{
	return tickCount.load(std::memory_order_relaxed);
}

void MySysTick::tick()
{
	++tickCount;
}

void MySysTick::delay_ms(uint32_t ms) const
{
	const uint32_t start = tickCount.load(std::memory_order_relaxed);
	while ((tickCount.load(std::memory_order_relaxed) - start) < ms)
		;
}

extern "C" void SysTick_Handler()
{
	getDrivers().my_systick.tick();
}
