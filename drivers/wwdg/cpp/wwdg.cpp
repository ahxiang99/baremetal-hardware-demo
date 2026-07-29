#include "cpp/wwdg.hpp"

#include <cmath>

#include "RegisterUtils.hpp"
#include "drivers.hpp"
#include "logger.hpp"
#include "low-level/rcc_bitfields.h"
#include "low-level/wwdg_bitfields.h"
#include "pch.hpp"

WindowWatchDog::WindowWatchDog() : m_Instance(WWDG_BASE)
{
}

Result<> WindowWatchDog::initialize(const uint32_t &apb1Hz)
{
	/*Enable WatchDog Timer at RCC*/
	RegisterUtils::setBits(RCC->APB1ENR, RCC_APB1ENR_WWDG_EN);

	/*Set Timer Base and Window Value.
	  WINDOW_VALUE < RELOAD_VALUE so a refresh attempted before the counter has
	  counted down into (CRITICAL_VALUE, WINDOW_VALUE] is rejected by hardware and
	  resets the MCU — catches a runaway loop that refreshes too fast, not just a
	  stuck one. */
	m_Instance->CFR = (PRESCALER << WWDG_CFR_WDGTB_Pos) | (WINDOW_VALUE << WWDG_CFR_W_Pos);

	/*Enable WatchDog*/
	m_Instance->CR = WWDG_CR_WDGA | RELOAD_VALUE;

	/* Natural (never-refreshed) reset fires when the counter passes CRITICAL_VALUE, i.e.
	   after (RELOAD_VALUE + 1 - CRITICAL_VALUE) decrements — not a full reload-to-zero
	   count. This is the real deadline the caller must refresh within. */
	constexpr float_t decrementsToReset = static_cast<float_t>(RELOAD_VALUE + 1U - CRITICAL_VALUE);
	t_max = (4096.0f * static_cast<float_t>(1U << PRESCALER) * decrementsToReset / static_cast<float_t>(apb1Hz)) * 1000.0f;
	LOG_INFO("WWDG configured: max time-to-reset ~{} ms", t_max);
	return Ok();
}
void WindowWatchDog::resetCounter() const
{
	/* Hardware rejects (and resets on) a refresh made while T[6:0] > WINDOW_VALUE;
	   the caller is responsible for refreshing on a cadence that lands inside
	   (CRITICAL_VALUE, WINDOW_VALUE]. */
	m_Instance->CR = WWDG_CR_WDGA | RELOAD_VALUE;
}

float_t WindowWatchDog::getTimeoutMs() const
{
	return t_max;
}
