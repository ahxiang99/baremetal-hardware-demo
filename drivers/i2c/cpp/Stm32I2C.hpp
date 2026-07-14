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
  ERR_I2C_TIMEOUT,
  ERR_I2C_AF,
  ERR_I2C_BUSY,
  ERR_I2C_SB,
  ERR_I2C_TXE,
  ERR_I2C_BTF,
  ERR_I2C_RXNE,
  ERR_I2C_BUS,
  ERR_I2C_ARLO,
  ERR_I2C_OVR,
  ERR_I2C_DATA_EMPTY
};

enum class i2c_device_t { I2C_D1, I2C_D2, I2C_D3, TotalNum };

enum class i2c_freq_t : uint8_t { _100KHz, _400Khz };

enum class i2c_addressmode_t : uint8_t { AddressMode_7Bit, AddressMode_10Bit };

enum class i2c_mode_t : uint8_t { NONE, MASTER, SLAVE, MEM_READ, MEM_WRITE };

struct i2c_config_t {
  i2c_device_t DevNum;
  i2c_freq_t ClockFreq;
  uint32_t OwnAddress1;
  i2c_addressmode_t AddressingMode;
  uint32_t DualAddressMode;
  uint32_t OwnAddress2;
};

class Stm32I2C {
protected:
  I2C_TypeDef *i2c_ = nullptr;
  std::atomic<i2c_state_t> state_{i2c_state_t::RESET};
  std::atomic<i2c_error_t> error_{i2c_error_t::NONE};
  i2c_mode_t mode_{i2c_mode_t::NONE};

public:
  Stm32I2C();
  Result<> initialize(const i2c_config_t &Config);
  bool Write(uint16_t DevAddress, const uint8_t *pData, uint16_t Size,
             uint32_t Timeout);
  bool Read(uint16_t DevAddress, uint8_t *pData, uint16_t Size,
            uint32_t Timeout);
  void handleEVInterrupt();
  void handleERInterrupt();

protected:
  bool isHardwareBusy(const uint32_t &Timeout);
  bool WaitForFlagTimeout(volatile uint32_t &sr, const uint32_t &mask,
                          const uint32_t &Timeout);
  void Error_Handler();
  void resetPeripheral();

  // Start, Stop Generation
  void generate_start();
  void generate_stop();

  void enableAckBit();
  void disableAckBit();

  void enable_i2c();
  void disable_i2c();

  void enable_nvic(const i2c_device_t &cfg);

  // Clear Flag
  void clear_addr();
  void clear_nack();
  void clear_berr();
  void clear_arlo();
  void clear_ovr();

private:
  void set_freq();
  void set_ccr_and_trise(const i2c_freq_t &cfg);
  void set_addressing_mode(const i2c_addressmode_t &cfg);

  bool WriteTo7BitDevice(uint16_t DevAddress, const uint8_t *pData,
                         uint16_t Size, uint32_t Timeout);
  bool ReadFrom7BitDevice(uint16_t DevAddress, uint8_t *pData, uint16_t Size,
                          uint32_t Timeout);
};