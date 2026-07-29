#pragma once
#include "low-level/systick.h"

class MySysTick
{
      private:
	std::atomic<uint32_t> tickCount;
	SysTick_TypeDef *Instance;

      public:
	MySysTick();
	~MySysTick();
	[[nodiscard]] Result<> initialize() const;
	void tick();
	[[nodiscard]] uint32_t get_ticks() const;
	void delay_ms(uint32_t ms) const;
};

extern "C" void SysTick_Handler();
