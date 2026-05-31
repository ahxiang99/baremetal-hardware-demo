#pragma once
#include "II2C.hpp"
#include "low-level/i2c_registers.h"
#include "status.h"

enum class I2C_State { RESET, READY, BUSY, BUSY_TX, BUSY_RX, ABORT, TIMEOUT, ERROR };

enum class I2C_Error { NONE, ERR_I2C_NULLPTR, ERR_I2C_TIMEOUT, ERR_I2C_AF, ERR_I2C_BUSY, ERR_I2C_SB, ERR_I2C_TXE, ERR_I2C_BTF, ERR_I2C_RXNE, ERR_I2C_BUS, ERR_I2C_ARLO, ERR_I2C_OVR };

enum class I2C_Clk_Freq { _100KHz, _400Khz };

enum class I2C_Addressing_Mode { AddressMode_7Bit, AddressMode_10Bit };

enum class I2C_Mode { NONE, MASTER, SLAVE, MEM };

struct I2C_Config {
    I2C_Clk_Freq        ClockFreq;
    uint32_t            OwnAddress1;
    I2C_Addressing_Mode AddressingMode;
    uint32_t            DualAddressMode;
    uint32_t            OwnAddress2;
};

class Stm32I2C : public II2C {
   protected:
    I2C_TypeDef*       i2c_ = nullptr;
    I2C_Config         config_;
    volatile I2C_State state_ = I2C_State::RESET;

    I2C_Error          error_ = I2C_Error::NONE;
    I2C_Mode           mode_  = I2C_Mode::NONE;

   public:
    Stm32I2C() {}
    Stm32I2C(I2C_TypeDef* pInstance, const I2C_Config& Config) : i2c_(pInstance), config_(Config) {}
    void setVariable(I2C_TypeDef* pInstance, const I2C_Config& Config);
    bool initialize() override;
    bool Write(uint16_t DevAddress, const uint8_t* pData, uint16_t Size, uint32_t Timeout) override;
    bool Read(uint16_t DevAddress, uint8_t* pData, uint16_t Size, uint32_t Timeout) override;
    void handleEVInterrupt() override {};
    void handleERInterrupt() override {};

   protected:
    virtual void onPostI2CInit() {}
    virtual void Error_Handler();
    bool         isHardwareBusy(const uint32_t& Timeout);
    bool         WaitOnFlagUntilTimeout(volatile uint32_t& sr, const uint32_t& mask, const uint32_t& Timeout);

    // Start, Stop Generation
    void generateStartCondition();
    void generateStopCondition();

    void enableAckBit();
    void disableAckBit();

    void enableI2C();
    void disableI2C();

    // Clear Flag
    void clearAddrFlag();
    void clearAFFlag();
    void clearBERRFlag();
    void clearARLOFLag();
    void clearOVRFLag();

   private:
    void enablePeripheralClock();

    void configureI2C();
    void configureGpio();
    void configurePeripheralFreq();
    void configureAddressingMode();
    void configureCCRandTrise();

    bool WriteTo7BitDevice(uint16_t DevAddress, const uint8_t* pData, uint16_t Size, uint32_t Timeout);
    bool ReadFrom7BitDevice(uint16_t DevAddress, uint8_t* pData, uint16_t Size, uint32_t Timeout);
};