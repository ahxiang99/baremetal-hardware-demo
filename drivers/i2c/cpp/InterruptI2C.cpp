#include "InterruptI2C.hpp"

#include "Stm32I2C.hpp"
#include "low-level/i2c_bitfields.h"
#include "pch.hpp"

bool InterruptI2C::initialize() {
    if (!Stm32I2C::initialize()) {
        return false;
    }
    return true;
}

bool InterruptI2C::Write(uint16_t DevAddress, const uint8_t* pData, uint16_t Size, uint32_t Timeout) {
    if (state_ == I2C_State::READY) {
        if (isHardwareBusy(Timeout)) {
            error_ = I2C_Error::ERR_I2C_BUSY;
            return false;
        }

        state_   = I2C_State::BUSY_TX;
        mode_    = I2C_Mode::MASTER;
        error_   = I2C_Error::NONE;

        DevAddr  = std::move(DevAddress);
        XferSize = std::move(Size);

        for (size_t i = 0; i < Size; ++i) {
            TxBuffer.push(std::move(pData[i]));
        }

        generateStartCondition();
        enableInterruptFlag();

        return true;
    } else {
        return false;
    }
}

bool InterruptI2C::Read(uint16_t DevAddress, uint8_t* pData, uint16_t Size, uint32_t Timeout) {
    if (state_ == I2C_State::READY) {
        if (isHardwareBusy(Timeout)) {
            error_ = I2C_Error::ERR_I2C_BUSY;
            return false;
        }

        state_   = I2C_State::BUSY_RX;
        mode_    = I2C_Mode::MASTER;
        error_   = I2C_Error::NONE;

        DevAddr  = std::move(DevAddress);
        XferPtr  = pData;
        XferSize = std::move(Size);

        enableAckBit();

        generateStartCondition();

        enableInterruptFlag();

        return true;

    } else {
        return false;
    }
}

void InterruptI2C::processRx() {
    if (RxEventFlag) {
        RxEventFlag = false;

        while (RxBuffer.size() > 0) {
            *XferPtr++ = RxBuffer.pop().value();
        }

        if (dataCallback) {
            dataCallback();
        }
    }
}

void InterruptI2C::handleEVInterrupt() {
    const uint32_t sr1 = i2c_->SR1;
    // SB Flag: Start condition generated
    if (sr1 & I2C_SR1_SB) {
        if (state_ == I2C_State::BUSY_TX) {
            i2c_->DR = DevAddr & ~(1 << 0);
        } else if (state_ == I2C_State::BUSY_RX) {
            i2c_->DR = DevAddr | (1 << 0);
        } else {
            state_ = I2C_State::ERROR;
            error_ = I2C_Error::ERR_I2C_SB;
            generateStopCondition();
        }
    }
    // ADDR Flag: Address sent and ACK received
    else if (sr1 & I2C_SR1_ADDR) {
        // Clear ADDR Bit
        clearAddrFlag();
    }
    // TXE Flag: Data register empty, ready for next byte
    else if ((sr1 & I2C_SR1_TXE) && !(sr1 & I2C_SR1_BTF) && (state_ == I2C_State::BUSY_TX)) {
        if (TxBuffer.size() > 0) {
            i2c_->DR = TxBuffer.pop().value();
        }
    }
    // Wait for BTF (Byte Transfer Finished) to ensure last byte is gone
    else if ((sr1 & I2C_SR1_BTF)) {
        if (state_ == I2C_State::BUSY_TX) {
            // Tx State
            if (TxBuffer.size() > 0) {
                i2c_->DR = TxBuffer.pop().value();
            } else if (TxBuffer.empty()) {
                // Nothing to send, Send Stop Condition.
                generateStopCondition();
                disableInterruptFlag();
                state_ = I2C_State::READY;
                mode_  = I2C_Mode::NONE;
            }
        }
    }
    // RNXE Flag: Data register is full, receive the byte
    else if ((sr1 & I2C_SR1_RXNE)) {
        if (state_ == I2C_State::BUSY_RX) {
            if (XferSize > 1) {
                if (XferSize == 2) {
                    disableAckBit();
                    generateStopCondition();
                }
                RxBuffer.push(i2c_->DR);
                XferSize--;
            } else {
                RxBuffer.push(i2c_->DR);
                XferSize--;
                RxEventFlag = true;
                state_      = I2C_State::READY;
                mode_       = I2C_Mode::NONE;
                disableInterruptFlag();
            }
        }
    }
}

void InterruptI2C::handleERInterrupt() {
    const uint32_t sr1 = i2c_->SR1;
    // 1. Acknowledge Failure (AF) - The Sensor didn't respond
    if (sr1 & I2C_SR1_AF) {
        // Clear flag: Write 0 to the bit
        clearAFFlag();

        // Action: Generate STOP to release the bus
        generateStopCondition();

        state_ = I2C_State::ERROR;
        mode_  = I2C_Mode::NONE;
        error_ = I2C_Error::ERR_I2C_AF;
    }

    // 2. Bus Error (BERR) - Misplaced Start/Stop condition
    else if (sr1 & I2C_SR1_BERR) {
        clearBERRFlag();
        state_ = I2C_State::ERROR;
        mode_  = I2C_Mode::NONE;
        error_ = I2C_Error::ERR_I2C_BUS;
    }

    // 3. Arbitration Lost (ARLO) - Another Master took the bus
    else if (sr1 & I2C_SR1_ARLO) {
        clearARLOFLag();
        state_ = I2C_State::ERROR;
        mode_  = I2C_Mode::NONE;
        error_ = I2C_Error::ERR_I2C_ARLO;
    }

    // 4. Overrun/Underrun (OVR) - CPU too slow for the clock speed
    else if (sr1 & I2C_SR1_OVR) {
        clearOVRFLag();
        state_ = I2C_State::ERROR;
        mode_  = I2C_Mode::NONE;
        error_ = I2C_Error::ERR_I2C_OVR;
    }
    Error_Handler();
    // CRITICAL: Disable interrupts so we don't loop forever in an error state
    disableInterruptFlag();
}

void InterruptI2C::enableInterruptFlag() {
    constexpr uint32_t mask = I2C_CR2_ITEVTEN | I2C_CR2_ITBUFEN | I2C_CR2_ITERREN;
    RegisterUtils::setBits(i2c_->CR2, mask);
}

void InterruptI2C::disableInterruptFlag() {
    constexpr uint32_t mask = I2C_CR2_ITEVTEN | I2C_CR2_ITBUFEN | I2C_CR2_ITERREN;
    RegisterUtils::clearBits(i2c_->CR2, mask);
}

void InterruptI2C::onDataReceived(rxCallback cb) {
    dataCallback = cb;
}
bool InterruptI2C::isHardwareBusy(const uint32_t& Timeout) {
    volatile uint32_t count = Timeout * 15999;
    do {
        count = count - 1;
        if (count == 0U) {
            return false;
        }
    } while (i2c_->SR2 & I2C_SR2_BUSY);
    return true;
}