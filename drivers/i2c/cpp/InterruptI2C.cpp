#include "InterruptI2C.hpp"

#include <atomic>

#include "Stm32I2C.hpp"
#include "low-level/i2c_bitfields.h"
#include "pch.hpp"

Result<> InterruptI2C::initialize(const i2c_config_t &cfg) {
  TRY(Stm32I2C::initialize(cfg));
  enable_nvic(cfg.DevNum);

  if (i2c_ == nullptr)
    return Fail(Err::NullInstance);

  return Ok();
}

bool InterruptI2C::Write(uint16_t DevAddress, uint8_t *pData, uint16_t Size,
                         uint32_t Timeout) {
  if (state_ == i2c_state_t::READY) {
    if (isHardwareBusy(Timeout)) {
      error_ = i2c_error_t::ERR_I2C_BUSY;
      return false;
    }

    state_ = i2c_state_t::BUSY_TX;
    mode_ = i2c_mode_t::MASTER;
    error_ = i2c_error_t::NONE;

    DevAddr = DevAddress;
    XferPtr = pData;
    XferSize = Size;

    generate_start();
    enable_interrupt();
    return true;
  } else {
    return false;
  }
}

bool InterruptI2C::Read(uint16_t DevAddress, uint8_t *pData, uint16_t Size,
                        uint32_t Timeout) {
  if (state_ == i2c_state_t::READY) {
    if (isHardwareBusy(Timeout)) {
      error_ = i2c_error_t::ERR_I2C_BUSY;
      return false;
    }

    state_ = i2c_state_t::BUSY_RX;
    mode_ = i2c_mode_t::MASTER;
    error_ = i2c_error_t::NONE;

    DevAddr = DevAddress;
    XferPtr = pData;
    XferSize = Size;

    enableAckBit();

    generate_start();

    enable_interrupt();

    return true;
  } else {
    return false;
  }
}

void InterruptI2C::processRx() {
  if (RxEventFlag.load(std::memory_order_acquire)) {
    RxEventFlag.store(false, std::memory_order_relaxed);

    while (RxBuffer.size() > 0) {
      *XferPtr++ = RxBuffer.pop().value();
    }
    onDataReceived();
  }
}

void InterruptI2C::handleEVInterrupt() {
  const uint32_t sr1 = i2c_->SR1;
  // SB Flag: Start  generated
  if (sr1 & I2C_SR1_SB) {
    if (state_ == i2c_state_t::BUSY_TX) {
      i2c_->DR = DevAddr & ~(1 << 0);
    } else if (state_ == i2c_state_t::BUSY_RX) {
      i2c_->DR = DevAddr | (1 << 0);
    } else {
      state_ = i2c_state_t::ERROR;
      error_ = i2c_error_t::ERR_I2C_SB;
      Error_Handler();
    }
  }
  // ADDR Flag: Address sent and ACK received
  else if (sr1 & I2C_SR1_ADDR) {
    if (state_.load(std::memory_order_relaxed) == i2c_state_t::BUSY_RX) {
      if (XferSize == 1) {
        disableAckBit();
        generate_stop();
      }
    }
    // Clear ADDR Bit
    clear_addr();
  }
  // TXE Flag: Data register empty, ready for next byte
  else if ((sr1 & I2C_SR1_TXE) && !(sr1 & I2C_SR1_BTF) &&
           (state_ == i2c_state_t::BUSY_TX)) {
    if (XferSize > 0) {
      i2c_->DR = *XferPtr++;
      XferSize--;
    }
  }
  // Wait for BTF (Byte Transfer Finished) to ensure last byte is gone
  else if ((sr1 & I2C_SR1_BTF)) {
    if (state_ == i2c_state_t::BUSY_TX) {
      // Tx State
      if (XferSize > 0) {
        i2c_->DR = *XferPtr++;
        XferSize--;
      } else {
        // Nothing to send, Send Stop .
        generate_stop();
        disable_interrupt();
        state_ = i2c_state_t::READY;
        mode_ = i2c_mode_t::NONE;
      }
    }
  }
  // RNXE Flag: Data register is full, receive the byte
  else if ((sr1 & I2C_SR1_RXNE)) {
    if (state_ == i2c_state_t::BUSY_RX) {
      if (XferSize > 1) {
        if (XferSize == 2) {
          disableAckBit();
          generate_stop();
        }
        RxBuffer.push(i2c_->DR);
        XferSize--;
      } else {
        RxBuffer.push(i2c_->DR);
        XferSize--;
        state_ = i2c_state_t::READY;
        mode_ = i2c_mode_t::NONE;
        disable_interrupt();
        RxEventFlag.store(true, std::memory_order_release);
      }
    }
  }
}

void InterruptI2C::handleERInterrupt() {
  const uint32_t sr1 = i2c_->SR1;
  // 1. Acknowledge Failure (AF) - The Sensor didn't respond
  if (sr1 & I2C_SR1_AF) {
    // Clear flag: Write 0 to the bit
    clear_nack();

    // Action: Generate STOP to release the bus
    generate_stop();

    state_ = i2c_state_t::ERROR;
    mode_ = i2c_mode_t::NONE;
    error_ = i2c_error_t::ERR_I2C_AF;
  }

  // 2. Bus Error (BERR) - Misplaced Start/Stop
  else if (sr1 & I2C_SR1_BERR) {
    clear_berr();
    state_ = i2c_state_t::ERROR;
    mode_ = i2c_mode_t::NONE;
    error_ = i2c_error_t::ERR_I2C_BUS;
  }

  // 3. Arbitration Lost (ARLO) - Another Master took the bus
  else if (sr1 & I2C_SR1_ARLO) {
    clear_arlo();
    state_ = i2c_state_t::ERROR;
    mode_ = i2c_mode_t::NONE;
    error_ = i2c_error_t::ERR_I2C_ARLO;
  }

  // 4. Overrun/Underrun (OVR) - CPU too slow for the clock speed
  else if (sr1 & I2C_SR1_OVR) {
    clear_ovr();
    state_ = i2c_state_t::ERROR;
    mode_ = i2c_mode_t::NONE;
    error_ = i2c_error_t::ERR_I2C_OVR;
  }
  Error_Handler();
  // CRITICAL: Disable interrupts so we don't loop forever in an error state
  disable_interrupt();
}

void InterruptI2C::enable_interrupt() {
  RegisterUtils::setBits(i2c_->CR2,
                         I2C_CR2_ITEVTEN | I2C_CR2_ITBUFEN | I2C_CR2_ITERREN);
}

void InterruptI2C::disable_interrupt() {
  RegisterUtils::clearBits(i2c_->CR2,
                           I2C_CR2_ITEVTEN | I2C_CR2_ITBUFEN | I2C_CR2_ITERREN);
}

void InterruptI2C::onDataReceived() {
  for (auto &e : receivers_) {
    e.notify();
  }
}
bool InterruptI2C::isHardwareBusy(const uint32_t &Timeout) {
  volatile uint32_t count = Timeout * 15999;
  do {
    count = count - 1;
    if (count == 0U) {
      return false;
    }
  } while (i2c_->SR2 & I2C_SR2_BUSY);
  return true;
}
