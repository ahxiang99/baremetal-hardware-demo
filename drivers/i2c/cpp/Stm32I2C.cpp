#include "Stm32I2C.hpp"

#include <string.h>

#include <atomic>
#include <cstddef>

#include "RegisterUtils.hpp"
#include "cpp/II2C.hpp"
#include "cpp/Stm32GpioPin.hpp"
#include "drivers.hpp"
#include "logger.hpp"
#include "low-level/i2c_bitfields.h"
#include "low-level/i2c_registers.h"
#include "low-level/nvic.h"
#include "low-level/rcc.h"
#include "low-level/rcc_bitfields.h"
#include "pch.hpp"

static const peripherals_regs_table<I2C_TypeDef> i2c_table[static_cast<uint8_t>(I2C_DeviceNum::TotalNum)] = {
    {I2C1, RCC_APB1ENR_I2C1_EN, RCC_APB1RSTR_I2C1_RST},
    {I2C2, RCC_APB1ENR_I2C2_EN, RCC_APB1RSTR_I2C2_RST},
    {I2C3, RCC_APB1ENR_I2C3_EN, RCC_APB1RSTR_I2C3_RST}
};

Stm32I2C::Stm32I2C(const I2C_Config& Config) : config_(Config) {
    i2c_ = i2c_table[static_cast<uint8_t>(config_.DevNum)].instance;
}

void Stm32I2C::configure(const I2C_Config& Config) {
    config_ = Config;
    i2c_    = i2c_table[static_cast<uint8_t>(config_.DevNum)].instance;
}

bool Stm32I2C::Write(uint16_t DevAddress, const uint8_t* pData, uint16_t Size, uint32_t Timeout) {
    return WriteTo7BitDevice(DevAddress, pData, Size, Timeout);
}
bool Stm32I2C::Read(uint16_t DevAddress, uint8_t* pData, uint16_t Size, uint32_t Timeout) {
    return ReadFrom7BitDevice(DevAddress, pData, Size, Timeout);
}

bool Stm32I2C::WriteTo7BitDevice(uint16_t DevAddress, const uint8_t* pData, uint16_t Size, uint32_t Timeout) {
    if (state_ == I2C_State::READY) {
        if (isHardwareBusy(Timeout)) {
            error_ = I2C_Error::ERR_I2C_BUSY;
            return false;
        }

        /* Disable Pos but why ? */
        RegisterUtils::clearBits(i2c_->CR1, I2C_CR1_POS);

        state_ = I2C_State::BUSY_TX;
        mode_  = I2C_Mode::MASTER;
        error_ = I2C_Error::NONE;

        // Generate Start Condition
        generateStartCondition();

        // Wait Start Bit
        if (WaitForFlagTimeout(i2c_->SR1, I2C_SR1_SB, Timeout)) {
            state_ = I2C_State::READY;
            mode_  = I2C_Mode::NONE;
            error_ = I2C_Error::ERR_I2C_SB; /* Cannot generate start condition */
            return false;
        }

        // Send Slave Address
        i2c_->DR = (DevAddress & ~0x1);

        // Wait Addr Bit
        if (WaitForFlagTimeout(i2c_->SR1, I2C_SR1_ADDR, Timeout)) {
            state_ = I2C_State::READY;
            mode_  = I2C_Mode::NONE;
            error_ = I2C_Error::ERR_I2C_AF; /* Acknowledgment Failure */
            return false;
        }

        clearAddrFlag(); /* Clear Addr Flag*/

        // Transmit Data
        while (Size > 0U) {
            /* Wait TXE Flag Set */
            if (WaitForFlagTimeout(i2c_->SR1, I2C_SR1_TXE, Timeout)) {
                state_ = I2C_State::READY;
                mode_  = I2C_Mode::NONE;
                error_ = I2C_Error::ERR_I2C_TXE;
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
                state_ = I2C_State::READY;
                mode_  = I2C_Mode::NONE;
                error_ = I2C_Error::ERR_I2C_BTF;
                return false;
            }
        }
        generateStopCondition();
        state_ = I2C_State::READY;
        mode_  = I2C_Mode::NONE;
        return true;
    } else {
        return false;
    }
}

bool Stm32I2C::ReadFrom7BitDevice(uint16_t DevAddress, uint8_t* pData, uint16_t Size, uint32_t Timeout) {
    if (state_ == I2C_State::READY) {
        if (isHardwareBusy(Timeout)) {
            error_ = I2C_Error::ERR_I2C_BUSY;
            return false;
        }

        RegisterUtils::clearBits(i2c_->CR1, I2C_CR1_POS);

        state_ = I2C_State::BUSY_RX;
        mode_  = I2C_Mode::MASTER;
        error_ = I2C_Error::NONE;

        /* Enable Acknowledgement */
        RegisterUtils::setBits(i2c_->CR1, I2C_CR1_ACK);

        /* Enable Start Bit */
        RegisterUtils::setBits(i2c_->CR1, I2C_CR1_START);

        /* Wait Start Bit */

        if (WaitForFlagTimeout(i2c_->SR1, I2C_SR1_SB, Timeout)) {
            state_ = I2C_State::READY;
            mode_  = I2C_Mode::NONE;
            error_ = I2C_Error::ERR_I2C_SB;
            return false;
        }
        /* Send Slave Address */
        i2c_->DR = DevAddress | 0x1 << 0;

        /* Wait Addr Flag */

        if (WaitForFlagTimeout(i2c_->SR1, I2C_SR1_ADDR, Timeout)) {
            state_ = I2C_State::READY;
            mode_  = I2C_Mode::NONE;
            error_ = I2C_Error::ERR_I2C_AF;
            return false;
        }

        clearAddrFlag();

        if (Size == 0U) {
            /* Generate Stop */
            generateStopCondition();
        }

        while (Size > 0U) {
            if (Size <= 3U) {
                if (Size == 1) {
                    if (WaitForFlagTimeout(i2c_->SR1, I2C_SR1_RXNE, Timeout)) {
                        state_ = I2C_State::READY;
                        mode_  = I2C_Mode::NONE;
                        error_ = I2C_Error::ERR_I2C_RXNE;
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
                        state_ = I2C_State::READY;
                        mode_  = I2C_Mode::NONE;
                        error_ = I2C_Error::ERR_I2C_BTF;
                        return false;
                    }
                    RegisterUtils::clearBits(i2c_->CR1, I2C_CR1_ACK);
                    *pData = i2c_->DR;
                    pData++;
                    Size--;

                    if (WaitForFlagTimeout(i2c_->SR1, I2C_SR1_BTF, Timeout)) {
                        state_ = I2C_State::READY;
                        mode_  = I2C_Mode::NONE;
                        error_ = I2C_Error::ERR_I2C_BTF;
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
                    state_ = I2C_State::READY;
                    mode_  = I2C_Mode::NONE;
                    error_ = I2C_Error::ERR_I2C_RXNE;
                    return false;
                }
                *pData = i2c_->DR;
                pData++;
                Size--;
            }
        }
        state_ = I2C_State::READY;
        mode_  = I2C_Mode::NONE;
        return true;
    } else {
        return false;
    }
}

bool Stm32I2C::initialize() {
    if (i2c_ == nullptr) {
        state_ = I2C_State::ERROR;
        error_ = I2C_Error::ERR_I2C_NULLPTR;
        Error_Handler();
        return false;
    }
    enablePeripheralClock();
    configureI2C();
    enableI2C();
    state_ = I2C_State::READY;
    return true;
}

void Stm32I2C::enablePeripheralClock() {
    volatile uint32_t* regs = &RCC->APB1ENR;
    const uint32_t     mask{i2c_table[static_cast<uint8_t>(config_.DevNum)].enableBit};
    RegisterUtils::setBits(*regs, mask);

    // Enable Peripheral Clock
    if (i2c_ == I2C1) {
        RegisterUtils::setBits(RCC->APB1ENR, RCC_APB1ENR_I2C1_EN);
        My_NVIC_EnableIRQ(I2C1_EV_IRQn);
        My_NVIC_EnableIRQ(I2C1_ER_IRQn);
    } else if (i2c_ == I2C2) {
        RegisterUtils::setBits(RCC->APB1ENR, RCC_APB1ENR_I2C2_EN);
        My_NVIC_EnableIRQ(I2C2_EV_IRQn);
        My_NVIC_EnableIRQ(I2C2_ER_IRQn);
    } else if (i2c_ == I2C3) {
        RegisterUtils::setBits(RCC->APB1ENR, RCC_APB1ENR_I2C3_EN);
        My_NVIC_EnableIRQ(I2C3_EV_IRQn);
        My_NVIC_EnableIRQ(I2C3_ER_IRQn);
    } else {
        state_ = I2C_State::ERROR;
        error_ = I2C_Error::ERR_I2C_NULLPTR;
        Error_Handler();
    }
}

void Stm32I2C::configureI2C() {
    disableI2C();
    configurePeripheralFreq();
    configureAddressingMode();
    configureCCRandTrise();
}

void Stm32I2C::disableI2C() {
    RegisterUtils::clearBits(i2c_->CR1, I2C_CR1_PE);
}

void Stm32I2C::enableI2C() {
    RegisterUtils::setBits(i2c_->CR1, I2C_CR1_PE);
}

void Stm32I2C::configurePeripheralFreq() {
    // STM32F4 HSI Frequency is 16MHz
    RegisterUtils::setBits(i2c_->CR2, I2C_CR2_16MHz);
}

void Stm32I2C::configureAddressingMode() {
    switch (config_.AddressingMode) {
        case I2C_Addressing_Mode::AddressMode_7Bit:
            RegisterUtils::clearBits(i2c_->OAR1, I2C_OAR1_ADDMODE);
            break;
        case I2C_Addressing_Mode::AddressMode_10Bit:
            RegisterUtils::setBits(i2c_->OAR1, I2C_OAR1_ADDMODE);
            break;
        default:
            state_ = I2C_State::ERROR;
            Error_Handler();
            return;
    }
}

void Stm32I2C::configureCCRandTrise() {
    /* Configrue CCR */
    uint32_t SCL_Freq = 1;
    switch (config_.ClockFreq) {
        case I2C_FreqHz::_100KHz:
            SCL_Freq = 100000;
            break;
        case I2C_FreqHz::_400Khz:
            SCL_Freq = 400000;
            break;
    }
    uint32_t CCR = HSI_Freq_Hz / (2 * SCL_Freq);
    RegisterUtils::setBits(i2c_->CCR, CCR);

    /* Configure Trise */
    uint32_t Trise = HSI_Freq_Hz / 1000000 + 1;
    RegisterUtils::setBits(i2c_->TRISE, Trise);
}

void Stm32I2C::resetPeripheral() {
    // Disable peripheral — clears all internal state machines (RM0368 §18.3.3)
    disableI2C();

    // Dummy read to flush any pending DR/SR data before re-enable
    volatile uint32_t dummy = i2c_->SR1;
    dummy                   = i2c_->SR2;
    (void)dummy;

    // Re-enable with same configuration already written to CR1/CR2/CCR/TRISE
    enableI2C();

    mode_ = I2C_Mode::NONE;
    error_.store(I2C_Error::NONE, std::memory_order::relaxed);
    state_.store(I2C_State::READY, std::memory_order_relaxed);
}

void Stm32I2C::Error_Handler() {
    // Release bus before any reset — prevents SDA/SCL from staying asserted
    generateStopCondition();

    LOG_ERROR("I2C Error: {}", static_cast<uint16_t>(error_.load(std::memory_order_relaxed)));

    // Soft-reset the peripheral so the next Write/Read call can retry cleanly.
    // A full re-initialize() is unnecessary — CR1/CR2/CCR/TRISE are still valid.
    resetPeripheral();
}

bool Stm32I2C::isHardwareBusy(const uint32_t& Timeout) {
    uint32_t tickStart = getDrivers().my_systick.get_ticks();
    while ((getDrivers().my_systick.get_ticks() - tickStart) < Timeout) {
        if (!(i2c_->SR2 & I2C_SR2_BUSY)) {
            return false;
        }
    }
    return true;
}

bool Stm32I2C::WaitForFlagTimeout(volatile uint32_t& sr, const uint32_t& mask, const uint32_t& Timeout) {
    uint32_t tickStart = getDrivers().my_systick.get_ticks();
    while ((getDrivers().my_systick.get_ticks() - tickStart) < Timeout) {
        if (sr & mask) {
            return false;
        }
    }
    return true;
}
void Stm32I2C::generateStartCondition() {
    RegisterUtils::setBits(i2c_->CR1, I2C_CR1_START);
}
void Stm32I2C::generateStopCondition() {
    RegisterUtils::setBits(i2c_->CR1, I2C_CR1_STOP);
}
void Stm32I2C::enableAckBit() {
    RegisterUtils::setBits(i2c_->CR1, I2C_CR1_ACK);
}
void Stm32I2C::disableAckBit() {
    RegisterUtils::clearBits(i2c_->CR1, I2C_CR1_ACK);
}
void Stm32I2C::clearAddrFlag() {
    uint32_t temp;
    temp = i2c_->SR1;
    temp = i2c_->SR2;
    (void)(temp);
}
void Stm32I2C::clearAFFlag() {
    RegisterUtils::clearBits(i2c_->SR1, I2C_SR1_AF);
}
void Stm32I2C::clearBERRFlag() {
    RegisterUtils::clearBits(i2c_->SR1, I2C_SR1_BERR);
}
void Stm32I2C::clearARLOFLag() {
    RegisterUtils::clearBits(i2c_->SR1, I2C_SR1_ARLO);
}
void Stm32I2C::clearOVRFLag() {
    RegisterUtils::clearBits(i2c_->SR1, I2C_SR1_OVR);
}
void Stm32I2C::enableNVICInterrupt() {
    switch (config_.DevNum) {
        case I2C_DeviceNum::I2C_D1:
            My_NVIC_EnableIRQ(I2C1_EV_IRQn);
            My_NVIC_EnableIRQ(I2C1_ER_IRQn);
            break;
        case I2C_DeviceNum::I2C_D2:
            My_NVIC_EnableIRQ(I2C2_EV_IRQn);
            My_NVIC_EnableIRQ(I2C2_ER_IRQn);
            break;
        case I2C_DeviceNum::I2C_D3:
            My_NVIC_EnableIRQ(I2C3_EV_IRQn);
            My_NVIC_EnableIRQ(I2C3_ER_IRQn);
            break;
        case I2C_DeviceNum::TotalNum:
            break;
    }
}
