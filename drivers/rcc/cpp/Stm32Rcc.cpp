#include "Stm32Rcc.hpp"

#include <cstdint>

#include "RegisterUtils.hpp"
#include "low-level/rcc_bitfields.h"

SysClock::SysClock() : m_Instance(RCC), m_Flash(FLASH)
{
}

Result<> SysClock::initialize(const SysClockConfig &config, const ClockTree &clock)
{
	m_Config = config;
	m_SysClock = clock;

	switch (m_Config.Src) {
	case SysClockSource::HSI:
		break;
	case SysClockSource::HSE:
		break;
	case SysClockSource::PLL: {
		/* Set PLL Setting */
		RegisterUtils::modify(m_Instance->PLLCFGR, RCC_PLLCFGR_PLLM_Msk, m_Config.PllCfg.pll_m << RCC_PLLCFGR_PLLM_Pos);
		RegisterUtils::modify(m_Instance->PLLCFGR, RCC_PLLCFGR_PLLN_Msk, m_Config.PllCfg.pll_n << RCC_PLLCFGR_PLLN_Pos);
		if (m_Config.PllCfg.pll_p == 2) {
			RegisterUtils::clearBits(m_Instance->PLLCFGR, RCC_PLLCFGR_PLLP_Msk);
		} else if (m_Config.PllCfg.pll_p == 4) {
			RegisterUtils::setBits(m_Instance->PLLCFGR, (1U << RCC_PLLCFGR_PLLP_Pos));
		} else if (m_Config.PllCfg.pll_p == 6) {
			RegisterUtils::setBits(m_Instance->PLLCFGR, (2U << RCC_PLLCFGR_PLLP_Pos));
		} else {
			RegisterUtils::setBits(m_Instance->PLLCFGR, (3U << RCC_PLLCFGR_PLLP_Pos));
		}

		/* Always use HSI PLL Src */
		RegisterUtils::clearBits(m_Instance->PLLCFGR, 1 << RCC_PLLCFGR_PLLSRC_Pos);

		/* Configure Prescalar */
		configurePrescaler();

		/* Program Flash Before Switching CPU Frequency */
		RegisterUtils::modify(m_Flash->ACR, FLASH_ACR_LATENCY_Msk, m_Config.flashLatency << FLASH_ACR_LATENCY_Pos);

		/* Set PLL ON */
		RegisterUtils::setBits(m_Instance->CR, RCC_CR_PLLON);

		/* Wait until PLL RDY */
		while (!(m_Instance->CR & RCC_CR_PLLRDY))
			;

		/* Change PLL to SysClk */
		RegisterUtils::modify(m_Instance->CFGR, RCC_CFGR_SW_Msk, static_cast<uint8_t>(m_Config.Src) << RCC_CFGR_SW_Pos);

		/* Wait until SWS change to PLL */
		uint32_t expectedSws = static_cast<uint8_t>(m_Config.Src);
		while ((m_Instance->CFGR & RCC_CFGR_SWS_Msk) != (expectedSws << RCC_CFGR_SWS_Pos))
			;
		break;
	}
	case SysClockSource::COUNT:
		return Fail(Err::InvalidParam);
		break;
	}
	return Ok();
}
void SysClock::configurePrescaler()
{
	uint8_t ahb_pre = static_cast<uint8_t>(m_Config.ahb_div);
	uint8_t apb1_pre = static_cast<uint8_t>(m_Config.apb1_div);
	uint8_t apb2_pre = static_cast<uint8_t>(m_Config.apb2_div);

	switch (m_Config.ahb_div) {
	case AHB_ClockDivision::DIV_1:
		RegisterUtils::clearBits(m_Instance->CFGR, RCC_CFGR_HPRE_Msk);
		break;
	case AHB_ClockDivision::DIV_2:
	case AHB_ClockDivision::DIV_4:
	case AHB_ClockDivision::DIV_8:
	case AHB_ClockDivision::DIV_16:
	case AHB_ClockDivision::DIV_64:
	case AHB_ClockDivision::DIV_128:
	case AHB_ClockDivision::DIV_256:
	case AHB_ClockDivision::DIV_512:
		RegisterUtils::modify(m_Instance->CFGR, RCC_CFGR_HPRE_Msk, (ahb_pre << RCC_CFGR_HPRE_Pos));
		break;
	}

	switch (m_Config.apb1_div) {
	case APB_ClockDivision::DIV_1:
		RegisterUtils::clearBits(m_Instance->CFGR, RCC_CFGR_PPRE1_Msk);
		break;
	case APB_ClockDivision::DIV_2:
	case APB_ClockDivision::DIV_4:
	case APB_ClockDivision::DIV_8:
	case APB_ClockDivision::DIV_16:
		RegisterUtils::modify(m_Instance->CFGR, RCC_CFGR_PPRE1_Msk, (apb1_pre << RCC_CFGR_PPRE1_Pos));
		break;
	}

	switch (m_Config.apb2_div) {
	case APB_ClockDivision::DIV_1:
		RegisterUtils::clearBits(m_Instance->CFGR, RCC_CFGR_PPRE2_Msk);
		break;
	case APB_ClockDivision::DIV_2:
	case APB_ClockDivision::DIV_4:
	case APB_ClockDivision::DIV_8:
	case APB_ClockDivision::DIV_16:
		RegisterUtils::modify(m_Instance->CFGR, RCC_CFGR_PPRE2_Msk, (apb2_pre << RCC_CFGR_PPRE2_Pos));
		break;
	}
}

ClockTree SysClock::getSysClock()
{
	return m_SysClock;
}
