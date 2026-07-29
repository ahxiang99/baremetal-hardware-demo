#include "Stm32I2C.hpp"

#include <cstring>

#include <atomic>

#include "RegisterUtils.hpp"
#include "drivers.hpp"
#include "logger.hpp"
#include "low-level/i2c_bitfields.h"
#include "low-level/i2c_registers.h"
#include "low-level/nvic.h"
#include "low-level/rcc.h"
#include "low-level/rcc_bitfields.h"
#include "pch.hpp"

namespace
{

struct i2c_table_info {
	I2C_TypeDef *instance;
	uint32_t enableBit;
	uint32_t disableBit;
	int8_t irqNumEv;
	int8_t irqNumEr;
};

const i2c_table_info i2c_table[static_cast<uint8_t>(i2c_device_t::COUNT)]{
	{I2C1, RCC_APB1ENR_I2C1_EN, RCC_APB1RSTR_I2C1_RST, I2C1_EV_IRQn, I2C1_ER_IRQn},
	{I2C2, RCC_APB1ENR_I2C2_EN, RCC_APB1RSTR_I2C2_RST, I2C2_EV_IRQn, I2C2_ER_IRQn},
	{I2C3, RCC_APB1ENR_I2C3_EN, RCC_APB1RSTR_I2C3_RST, I2C3_EV_IRQn, I2C3_ER_IRQn}};

I2C_TypeDef *enableAndGet(i2c_device_t dev)
{
	const auto &entry = i2c_table[static_cast<uint8_t>(dev)];
	RegisterUtils::setBits(RCC->APB1ENR, entry.enableBit);
	return entry.instance;
}
} // namespace
Result<> Stm32I2C::initialize(const i2c_config_t &cfg)
{
	i2c_ = enableAndGet(cfg.DevNum);
	if (i2c_ == nullptr) {
		return Fail(Err::NullInstance);
	}

	disable_i2c();

	set_freq();
	set_addressing_mode(cfg.AddressingMode);
	set_ccr_and_trise(cfg.ClockFreq);

	enable_i2c();

	state_ = i2c_state_t::READY;
	return Ok();
}

bool Stm32I2C::Write(uint16_t DevAddress, const uint8_t *pData, uint16_t Size, uint32_t Timeout)
{
	return write_7bit_device(DevAddress, pData, Size, Timeout);
}
bool Stm32I2C::Read(uint16_t DevAddress, uint8_t *pData, uint16_t Size, uint32_t Timeout)
{
	return read_7bit_device(DevAddress, pData, Size, Timeout);
}

bool Stm32I2C::write_7bit_device(uint16_t DevAddress, const uint8_t *pData, uint16_t Size,
				 uint32_t Timeout)
{
	if (state_ == i2c_state_t::READY) {
		if (isHardwareBusy(Timeout)) {
			error_ = i2c_error_t::ERR_I2C_BUSY;
			return false;
		}

		RegisterUtils::clearBits(i2c_->CR1, I2C_CR1_POS);

		state_ = i2c_state_t::BUSY_TX;
		mode_ = i2c_mode_t::MASTER;
		error_ = i2c_error_t::NONE;

		// Generate Start
		generate_start();

		// Wait Start Bit
		if (WaitForFlagTimeout(i2c_->SR1, I2C_SR1_SB, Timeout)) {
			state_ = i2c_state_t::READY;
			mode_ = i2c_mode_t::NONE;
			error_ = i2c_error_t::ERR_I2C_TIMEOUT; /* Cannot generate start  */
			return false;
		}

		// Send Slave Address
		i2c_->DR = (DevAddress & ~0x1);

		// Wait Addr Bit
		if (WaitForFlagTimeout(i2c_->SR1, I2C_SR1_ADDR, Timeout)) {
			state_ = i2c_state_t::READY;
			mode_ = i2c_mode_t::NONE;
			error_ = i2c_error_t::ERR_I2C_AF; /* Acknowledgment Failure */
			return false;
		}

		I2C_CLEAR_ADDR_(i2c_); /* Clear Addr Flag*/

		// Transmit Data
		while (Size > 0U) {
			/* Wait TXE Flag Set */
			if (WaitForFlagTimeout(i2c_->SR1, I2C_SR1_TXE, Timeout)) {
				state_ = i2c_state_t::READY;
				mode_ = i2c_mode_t::NONE;
				error_ = i2c_error_t::ERR_I2C_TIMEOUT;
				return false;
			}
			i2c_->DR = *pData;
			pData++;
			Size--;
			if (i2c_->SR1 & I2C_SR1_BTF) {
				i2c_->DR = *pData;
				pData++;
				Size--;
			}
			if (WaitForFlagTimeout(i2c_->SR1, I2C_SR1_BTF, Timeout)) {
				state_ = i2c_state_t::READY;
				mode_ = i2c_mode_t::NONE;
				error_ = i2c_error_t::ERR_I2C_TIMEOUT;
				return false;
			}
		}
		generate_stop();
		state_ = i2c_state_t::READY;
		mode_ = i2c_mode_t::NONE;
		return true;
	} else {
		return false;
	}
}

bool Stm32I2C::read_7bit_device(uint16_t DevAddress, uint8_t *pData, uint16_t Size,
				uint32_t Timeout)
{
	if (state_ == i2c_state_t::READY) {
		if (isHardwareBusy(Timeout)) {
			error_ = i2c_error_t::ERR_I2C_BUSY;
			return false;
		}

		RegisterUtils::clearBits(i2c_->CR1, I2C_CR1_POS);

		state_ = i2c_state_t::BUSY_RX;
		mode_ = i2c_mode_t::MASTER;
		error_ = i2c_error_t::NONE;

		/* Enable Acknowledgement */
		RegisterUtils::setBits(i2c_->CR1, I2C_CR1_ACK);

		/* Enable Start Bit */
		RegisterUtils::setBits(i2c_->CR1, I2C_CR1_START);

		/* Wait Start Bit */
		if (WaitForFlagTimeout(i2c_->SR1, I2C_SR1_SB, Timeout)) {
			state_ = i2c_state_t::READY;
			mode_ = i2c_mode_t::NONE;
			error_ = i2c_error_t::ERR_I2C_TIMEOUT;
			return false;
		}
		/* Send Slave Address */
		i2c_->DR = DevAddress | 0x1 << 0;

		/* Wait Addr Flag */

		if (WaitForFlagTimeout(i2c_->SR1, I2C_SR1_ADDR, Timeout)) {
			state_ = i2c_state_t::READY;
			mode_ = i2c_mode_t::NONE;
			error_ = i2c_error_t::ERR_I2C_AF;
			return false;
		}

		I2C_CLEAR_ADDR_(i2c_); /* Clear Addr Flag*/

		if (Size == 0U) {
			/* Generate Stop */
			generate_stop();
		}

		while (Size > 0U) {
			if (Size <= 3U) {
				if (Size == 1) {
					if (WaitForFlagTimeout(i2c_->SR1, I2C_SR1_RXNE, Timeout)) {
						state_ = i2c_state_t::READY;
						mode_ = i2c_mode_t::NONE;
						error_ = i2c_error_t::ERR_I2C_TIMEOUT;
						return false;
					}
					*pData = i2c_->DR;
					pData++;
					Size--;
				} else if (Size == 2U) {
					RegisterUtils::setBits(i2c_->CR1, I2C_CR1_STOP);
					*pData = i2c_->DR;
					pData++;
					Size--;
					*pData = i2c_->DR;
					pData++;
					Size--;
				} else {
					if (WaitForFlagTimeout(i2c_->SR1, I2C_SR1_BTF, Timeout)) {
						state_ = i2c_state_t::READY;
						mode_ = i2c_mode_t::NONE;
						error_ = i2c_error_t::ERR_I2C_TIMEOUT;
						return false;
					}
					RegisterUtils::clearBits(i2c_->CR1, I2C_CR1_ACK);
					*pData = i2c_->DR;
					pData++;
					Size--;

					if (WaitForFlagTimeout(i2c_->SR1, I2C_SR1_BTF, Timeout)) {
						state_ = i2c_state_t::READY;
						mode_ = i2c_mode_t::NONE;
						error_ = i2c_error_t::ERR_I2C_TIMEOUT;
						return false;
					}

					RegisterUtils::setBits(i2c_->CR1, I2C_CR1_STOP);
					*pData = i2c_->DR;
					pData++;
					Size--;

					*pData = i2c_->DR;
					pData++;
					Size--;
				}
			} else {
				if (WaitForFlagTimeout(i2c_->SR1, I2C_SR1_RXNE, Timeout)) {
					state_ = i2c_state_t::READY;
					mode_ = i2c_mode_t::NONE;
					error_ = i2c_error_t::ERR_I2C_TIMEOUT;
					return false;
				}
				*pData = i2c_->DR;
				pData++;
				Size--;
			}
		}
		state_ = i2c_state_t::READY;
		mode_ = i2c_mode_t::NONE;
		return true;
	} else {
		return false;
	}
}

void Stm32I2C::disable_i2c() const
{
	RegisterUtils::clearBits(i2c_->CR1, I2C_CR1_PE);
}

void Stm32I2C::enable_i2c() const
{
	RegisterUtils::setBits(i2c_->CR1, I2C_CR1_PE);
}

void Stm32I2C::set_addressing_mode(const i2c_addressmode_t &addr) const
{
	switch (addr) {
	case i2c_addressmode_t::AddressMode_7Bit:
		RegisterUtils::clearBits(i2c_->OAR1, I2C_OAR1_ADDMODE);
		break;
	case i2c_addressmode_t::AddressMode_10Bit:
		RegisterUtils::setBits(i2c_->OAR1, I2C_OAR1_ADDMODE);
		break;
	}
}

void Stm32I2C::set_freq() const
{
	i2c_->CR2 = getDrivers().sysclock.getSysClock().apb1 / 1'000'000;
}

void Stm32I2C::set_ccr_and_trise(const i2c_freq_t &cfg) const
{
	/* Configure CCR */
	uint32_t SCL_Freq = 1;
	switch (cfg) {
	case i2c_freq_t::_100KHz:
		SCL_Freq = 100000;
		break;
	case i2c_freq_t::_400Khz:
		SCL_Freq = 400000;
		break;
	}

	uint32_t ccr = getDrivers().sysclock.getSysClock().apb1 / (2 * SCL_Freq);
	RegisterUtils::setBits(i2c_->CCR, ccr);

	/* Configure Trise */
	uint32_t trise = getDrivers().sysclock.getSysClock().apb1 / 1000000 + 1;
	RegisterUtils::setBits(i2c_->TRISE, trise);
}

void Stm32I2C::reset()
{
	// Disable peripheral — clears all internal state machines (RM0368 §18.3.3)
	disable_i2c();

	// Dummy read to flush any pending DR/SR data before re-enable
	volatile uint32_t dummy = i2c_->SR1;
	dummy = i2c_->SR2;
	(void)dummy;

	// Re-enable with same configuration already written to CR1/CR2/CCR/TRISE
	enable_i2c();

	mode_ = i2c_mode_t::NONE;
	error_.store(i2c_error_t::NONE, std::memory_order::relaxed);
	state_.store(i2c_state_t::READY, std::memory_order_relaxed);
}

void Stm32I2C::Error_Handler()
{
	// Release bus before any reset — prevents SDA/SCL from staying asserted
	generate_stop();

	switch (error_.load(std::memory_order::relaxed)) {
	case i2c_error_t::ERR_I2C_AF:
		LOG_ERROR("I2C Acknowledgment Failed");
		break;
	case i2c_error_t::ERR_I2C_ARLO:
		LOG_ERROR("I2C Arbitration Loss");
		break;
	case i2c_error_t::ERR_I2C_BUS:
		LOG_ERROR("I2C Bus Error");
		break;
	case i2c_error_t::ERR_I2C_OVR:
		LOG_ERROR("I2C Overrun / Underrun Error");
		break;
	case i2c_error_t::NONE:
		break;
	case i2c_error_t::ERR_I2C_NULLPTR:
		LOG_ERROR("I2C Handle is nullptr");
		break;
	case i2c_error_t::ERR_I2C_TIMEOUT:
		LOG_ERROR("I2C Wait Flag Timeout");
		break;
	case i2c_error_t::ERR_I2C_BUSY:
		LOG_ERROR("I2C Bus Busy");
		break;
	}
	// Soft-reset the peripheral so the next Write/Read call can retry cleanly.
	// A full re-initialize() is unnecessary — CR1/CR2/CCR/TRISE are still valid.
	reset();
}

void Stm32I2C::scan_address(std::array<uint8_t, 128> &valid_address, uint8_t &len)
{
	len = 0;
	for (uint8_t i = 0x00; i <= 0x7F; ++i) {
		constexpr uint8_t data = 0x00;
		if (Write(i << 1, &data, sizeof(data), 3)) {
			valid_address[len++] = data;
		} else {
			RegisterUtils::clearBits(i2c_->SR1, I2C_SR1_AF);
			RegisterUtils::setBits(I2C1->CR1, I2C_CR1_STOP);
		}
	}
}

bool Stm32I2C::isHardwareBusy(const uint32_t &Timeout) const
{
	uint32_t tickStart = getDrivers().my_systick.get_ticks();
	while ((getDrivers().my_systick.get_ticks() - tickStart) < Timeout) {
		if (!(i2c_->SR2 & I2C_SR2_BUSY)) {
			return false;
		}
	}
	return true;
}

bool Stm32I2C::WaitForFlagTimeout(volatile uint32_t &sr, const uint32_t &mask,
				  const uint32_t &Timeout)
{
	uint32_t tickStart = getDrivers().my_systick.get_ticks();
	while ((getDrivers().my_systick.get_ticks() - tickStart) < Timeout) {
		if (sr & mask) {
			return false;
		}
	}
	return true;
}
void Stm32I2C::generate_start() const
{
	RegisterUtils::setBits(i2c_->CR1, I2C_CR1_START);
}
void Stm32I2C::generate_stop() const
{
	RegisterUtils::setBits(i2c_->CR1, I2C_CR1_STOP);
}
void Stm32I2C::enableAckBit() const
{
	RegisterUtils::setBits(i2c_->CR1, I2C_CR1_ACK);
}
void Stm32I2C::disableAckBit() const
{
	RegisterUtils::clearBits(i2c_->CR1, I2C_CR1_ACK);
}

void Stm32I2C::clear_nack() const
{
	RegisterUtils::clearBits(i2c_->SR1, I2C_SR1_AF);
}
void Stm32I2C::clear_berr() const
{
	RegisterUtils::clearBits(i2c_->SR1, I2C_SR1_BERR);
}
void Stm32I2C::clear_arlo() const
{
	RegisterUtils::clearBits(i2c_->SR1, I2C_SR1_ARLO);
}
void Stm32I2C::clear_ovr() const
{
	RegisterUtils::clearBits(i2c_->SR1, I2C_SR1_OVR);
}
void Stm32I2C::enable_nvic(const i2c_device_t &cfg)
{
	My_NVIC_EnableIRQ(i2c_table[static_cast<int8_t>(cfg)].irqNumEv);
	My_NVIC_EnableIRQ(i2c_table[static_cast<int8_t>(cfg)].irqNumEr);
}
