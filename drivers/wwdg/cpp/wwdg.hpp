#pragma once
#include <cmath>
#include <cstdint>

#include "low-level/wwdg_bitfields.h"
#include "low-level/wwdg_registers.h"

class WindowWatchDog
{
      public:
	WindowWatchDog();

	Result<> initialize(const uint32_t &apb1Hz);
	void resetCounter() const;
	float_t getTimeoutMs() const;

      private:
	/* Lower bound (exclusive) of the legal refresh range T[6:0]; refreshing at/below this
	   resets the WWDG counter itself would already be too late — a reset fires at 0x3F. */
	static constexpr uint8_t CRITICAL_VALUE = 0X40U;
	static constexpr uint8_t RELOAD_VALUE = 0X7FU;
	static constexpr uint8_t WINDOW_VALUE = 0X5AU;
	static constexpr uint8_t PRESCALER = 3U;

	WWDG_TypeDef *m_Instance;
	float_t t_max{0.0f};
};
