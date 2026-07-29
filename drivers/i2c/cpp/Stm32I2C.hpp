#pragma once
#include "II2C.hpp"
#include "low-level/i2c_registers.h"

enum class i2c_state_t : uint8_t {
	RESET,
	READY,
	BUSY,
	BUSY_TX,
	BUSY_RX,
	ABORT,
	TIMEOUT,
	ERROR
};

enum class i2c_error_t : uint8_t {
	NONE,
	ERR_I2C_NULLPTR,
	ERR_I2C_DATA_EMPTY,
	ERR_I2C_TIMEOUT,
	ERR_I2C_BUSY,
	ERR_I2C_AF,
	ERR_I2C_BUS,
	ERR_I2C_ARLO,
	ERR_I2C_OVR
};

enum class i2c_device_t {
	I2C_D1,
	I2C_D2,
	I2C_D3,
	COUNT
};

enum class i2c_freq_t : uint8_t {
	_100KHz,
	_400Khz
};

enum class i2c_addressmode_t : uint8_t {
	AddressMode_7Bit,
	AddressMode_10Bit
};

enum class i2c_mode_t : uint8_t {
	NONE,
	MASTER,
	SLAVE,
	MEM_READ,
	MEM_WRITE
};

struct i2c_config_t {
	i2c_device_t DevNum;
	i2c_freq_t ClockFreq;
	uint32_t OwnAddress1;
	i2c_addressmode_t AddressingMode;
	uint32_t DualAddressMode;
	uint32_t OwnAddress2;
};

#define I2C_CLEAR_ADDR_(handle)                                                                    \
	do {                                                                                       \
		uint32_t temp = handle->SR1;                                                       \
		temp = handle->SR2;                                                                \
		(void)temp;                                                                        \
	} while (0);

class Stm32I2C
{
      protected:
	I2C_TypeDef *i2c_ = nullptr;
	std::atomic<i2c_state_t> state_{i2c_state_t::RESET};
	std::atomic<i2c_error_t> error_{i2c_error_t::NONE};
	i2c_mode_t mode_{i2c_mode_t::NONE};

      public:
	Stm32I2C() = default;
	Result<> initialize(const i2c_config_t &Config);
	bool Write(uint16_t DevAddress, const uint8_t *pData, uint16_t Size, uint32_t Timeout);
	bool Read(uint16_t DevAddress, uint8_t *pData, uint16_t Size, uint32_t Timeout);

	void scan_address(std::array<uint8_t, 128> &valid_address, uint8_t &len);

      protected:
	bool isHardwareBusy(const uint32_t &Timeout) const;
	static bool WaitForFlagTimeout(volatile uint32_t &sr, const uint32_t &mask,
				       const uint32_t &Timeout);
	void Error_Handler();
	void reset();

	// Start, Stop Generation
	void generate_start() const;
	void generate_stop() const;

	void enableAckBit() const;
	void disableAckBit() const;

	void enable_i2c() const;
	void disable_i2c() const;

	static void enable_nvic(const i2c_device_t &cfg);
	// Clear Flag
	void clear_nack() const;
	void clear_berr() const;
	void clear_arlo() const;
	void clear_ovr() const;

      private:
	void set_freq() const;
	void set_ccr_and_trise(const i2c_freq_t &cfg) const;
	void set_addressing_mode(const i2c_addressmode_t &cfg) const;

	bool write_7bit_device(uint16_t DevAddress, const uint8_t *pData, uint16_t Size,
			       uint32_t Timeout);
	bool read_7bit_device(uint16_t DevAddress, uint8_t *pData, uint16_t Size, uint32_t Timeout);
};
