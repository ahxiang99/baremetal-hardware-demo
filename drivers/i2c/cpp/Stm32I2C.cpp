#include "Stm32I2C.hpp"

#include <string.h>

#include <cstddef>

#include "RegisterUtils.hpp"
#include "cpp/IGpio.hpp"
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

static const peripherals_regs_table<I2C_TypeDef>
    i2c_table[static_cast<uint8_t>(I2C_Dev::I2C_Total)] = {
        {I2C1, RCC_APB1ENR_I2C1_EN, RCC_APB1RSTR_I2C1_RST},
        {I2C2, RCC_APB1ENR_I2C2_EN, RCC_APB1RSTR_I2C2_RST},
        {I2C3, RCC_APB1ENR_I2C3_EN, RCC_APB1RSTR_I2C3_RST}
};

Stm32I2C::Stm32I2C(const I2C_Config& config) : config_(config) {
    i2c_ = i2c_table[static_cast<uint8_t>(config_.Dev_Num)].instance;
    initialize();
}

void Stm32I2C::configure(const I2C_Config& Config) {
    config_ = Config;
    i2c_    = i2c_table[static_cast<uint8_t>(config_.Dev_Num)].instance;
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
    onPostI2CInit();
    state_ = I2C_State::READY;
    return true;
}

bool Stm32I2C::Write(uint16_t DevAddress, const uint8_t* pData, uint16_t Size, uint32_t Timeout) {
    return WriteTo7BitDevice(DevAddress, pData, Size, Timeout);
}
bool Stm32I2C::Read(uint16_t DevAddress, uint8_t* pData, uint16_t Size, uint32_t Timeout) {
    return ReadFrom7BitDevice(DevAddress, pData, Size, Timeout);
}

bool Stm32I2C::WriteTo7BitDevice(uint16_t DevAddress, const uint8_t* pData, uint16_t Size,
                                 uint32_t Timeout) {
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
            Error_Handler();
            return false;
        }

        // Send Slave Address
        i2c_->DR = (DevAddress & ~0x1);

        // Wait Addr Bit
        if (WaitForFlagTimeout(i2c_->SR1, I2C_SR1_ADDR, Timeout)) {
            state_ = I2C_State::READY;
            mode_  = I2C_Mode::NONE;
            error_ = I2C_Error::ERR_I2C_AF; /* Acknowledgment Failure */
            Error_Handler();
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
                Error_Handler();
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
                Error_Handler();
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

bool Stm32I2C::ReadFrom7BitDevice(uint16_t DevAddress, uint8_t* pData, uint16_t Size,
                                  uint32_t Timeout) {
    if (state_ == I2C_State::READY) {
        if (isHardwareBusy(Timeout)) {
            error_ = I2C_Error::ERR_I2C_BUSY;
            return false;
        }

        // For N=2: set POS so ACK controls the *next* byte, not the current one.
        // For all other sizes: clear POS (normal ACK behaviour).
        if (Size == 2U) {
            RegisterUtils::setBits(i2c_->CR1, I2C_CR1_POS);
        } else {
            RegisterUtils::clearBits(i2c_->CR1, I2C_CR1_POS);
        }

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
            Error_Handler();
            return false;
        }
        /* Send Slave Address */
        i2c_->DR = DevAddress | 0x1 << 0;

        /* Wait Addr Flag */

        if (WaitForFlagTimeout(i2c_->SR1, I2C_SR1_ADDR, Timeout)) {
            state_ = I2C_State::READY;
            mode_  = I2C_Mode::NONE;
            error_ = I2C_Error::ERR_I2C_AF;
            Error_Handler();
            return false;
        }

        clearAddrFlag();

        // For N=2 the RM0368 sequence requires ACK to be cleared immediately
        // after ADDR so that NACK is sent when the second byte is clocked in.
        if (Size == 2U) {
            RegisterUtils::clearBits(i2c_->CR1, I2C_CR1_ACK);
        }

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
                        Error_Handler();
                        return false;
                    }
                    *pData = i2c_->DR;
                    pData++;
                    Size--;
                } else if (Size == 2U) {
                    // RM0368 §18.3.3 N=2 sequence:
                    // Wait BTF — at this point DR holds byte 1 and the shift
                    // register holds byte 2. Only then set STOP so the bus is
                    // released after the shift register empties.
                    if (WaitForFlagTimeout(i2c_->SR1, I2C_SR1_BTF, Timeout)) {
                        state_ = I2C_State::READY;
                        mode_  = I2C_Mode::NONE;
                        error_ = I2C_Error::ERR_I2C_BTF;
                        Error_Handler();
                        return false;
                    }
                    generateStopCondition();
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
                        Error_Handler();
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
                        Error_Handler();
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
                    Error_Handler();
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

void Stm32I2C::enablePeripheralClock() {
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
    RegisterUtils::setBits(i2c_->CR2, kI2C_CR2_FREQ_16MHz);
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
        case I2C_Clk_Freq::_100KHz:
            SCL_Freq = 100000;
            break;
        case I2C_Clk_Freq::_400Khz:
            SCL_Freq = 400000;
            break;
    }
    uint32_t CCR = HSI_Freq_Hz / (2 * SCL_Freq);
    i2c_->CCR    = CCR;

    /* Configure Trise */
    uint32_t Trise = HSI_Freq_Hz / 1000000 + 1;
    RegisterUtils::setBits(i2c_->TRISE, Trise);
}

void Stm32I2C::Error_Handler() {
    generateStopCondition();
    LOG_ERROR("I2C Error: {}", static_cast<uint16_t>(error_));
    while (1);
}

bool Stm32I2C::isHardwareBusy(const uint32_t& Timeout) {
    uint32_t tickStart = getDriver().my_systick.get_ticks();
    while ((getDriver().my_systick.get_ticks() - tickStart) < Timeout) {
        if (!(i2c_->SR2 & I2C_SR2_BUSY)) {
            return false;
        }
    }
    // Bus still busy after timeout — slave is holding SDA low mid-transaction.
    // Bit-bang 9 SCL pulses to clock it out, then recover the peripheral.
    // I2C1: SCL=PB6, SDA=PB7 (AF4 on STM32F401)
    recoverBus(GPIOB, (1U << 8), GPIOB, (1U << 9));
    return (i2c_->SR2 & I2C_SR2_BUSY) != 0U;
}

void Stm32I2C::recoverBus(GPIO_TypeDef* scl_port, uint32_t scl_pin, GPIO_TypeDef* sda_port,
                          uint32_t sda_pin) {
    // 1. Reconfigure SCL and SDA as open-drain GPIO outputs
    auto configure_od_out = [](GPIO_TypeDef* port, uint32_t pin) {
        uint32_t pos = 0;
        uint32_t p   = pin;
        while (p >>= 1U) {
            ++pos;
        }
        // MODER: output (01)
        port->MODER &= ~(0x3U << (pos * 2U));
        port->MODER |= (0x1U << (pos * 2U));
        // OTYPER: open-drain (1)
        port->OTYPER |= pin;
        // OSPEEDR: high speed
        port->OSPEEDR |= (0x3U << (pos * 2U));
        // PUPDR: pull-up (01)
        port->PUPDR &= ~(0x3U << (pos * 2U));
        port->PUPDR |= (0x1U << (pos * 2U));
        // Set high
        port->BSRR = pin;
    };
    configure_od_out(scl_port, scl_pin);
    configure_od_out(sda_port, sda_pin);

    // 2. Clock 9 SCL pulses — enough to flush any in-progress slave byte
    for (uint8_t i = 0; i < 9U; ++i) {
        scl_port->BSRR = (scl_pin << 16U);  // SCL low
        for (volatile uint32_t d = 0; d < 20U; ++d) {
        }
        scl_port->BSRR = scl_pin;  // SCL high
        for (volatile uint32_t d = 0; d < 20U; ++d) {
        }
        // Stop as soon as SDA is released by the slave
        if (sda_port->IDR & sda_pin) {
            break;
        }
    }

    // 3. Generate a STOP condition on the GPIO: SDA low → SCL high → SDA high
    sda_port->BSRR = (sda_pin << 16U);  // SDA low
    for (volatile uint32_t d = 0; d < 20U; ++d) {
    }
    scl_port->BSRR = scl_pin;  // SCL high
    for (volatile uint32_t d = 0; d < 20U; ++d) {
    }
    sda_port->BSRR = sda_pin;  // SDA high

    // 4. Re-configure pins back to AF4 (I2C open-drain)
    auto restore_af = [](GPIO_TypeDef* port, uint32_t pin, uint8_t af) {
        uint32_t pos = 0;
        uint32_t p   = pin;
        while (p >>= 1U) {
            ++pos;
        }
        port->MODER &= ~(0x3U << (pos * 2U));
        port->MODER |= (0x2U << (pos * 2U));  // ALTFN
        port->OTYPER |= pin;                  // open-drain
        if (pos < 8U) {
            port->AFR[0] &= ~(0xFU << (pos * 4U));
            port->AFR[0] |= ((uint32_t)af << (pos * 4U));
        } else {
            port->AFR[1] &= ~(0xFU << ((pos - 8U) * 4U));
            port->AFR[1] |= ((uint32_t)af << ((pos - 8U) * 4U));
        }
    };
    constexpr uint8_t kAF4_I2C = 4U;
    restore_af(scl_port, scl_pin, kAF4_I2C);
    restore_af(sda_port, sda_pin, kAF4_I2C);

    // 5. SWRST the peripheral to clear its internal state, then re-enable
    RegisterUtils::setBits(i2c_->CR1, I2C_CR1_SWRST);
    for (volatile uint32_t d = 0; d < 10U; ++d) {
    }
    RegisterUtils::clearBits(i2c_->CR1, I2C_CR1_SWRST);
    configureI2C();
    enableI2C();
}

bool Stm32I2C::WaitForFlagTimeout(volatile uint32_t& sr, const uint32_t& mask,
                                  const uint32_t& Timeout) {
    uint32_t tickStart = getDriver().my_systick.get_ticks();
    while ((getDriver().my_systick.get_ticks() - tickStart) < Timeout) {
        if (sr & mask) {
            return false;
        }
    }
    return true;
}
void Stm32I2C::clearAddrFlag() {
    uint32_t temp;
    temp = i2c_->SR1;
    temp = i2c_->SR2;
    (void)(temp);
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