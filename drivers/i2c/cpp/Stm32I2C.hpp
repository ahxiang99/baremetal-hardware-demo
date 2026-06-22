#pragma once
#include "II2C.hpp"
#include "low-level/i2c_registers.h"

enum class I2C_State : uint8_t { RESET, READY, BUSY, BUSY_TX, BUSY_RX, ABORT, TIMEOUT, ERROR };

enum class I2C_Error : uint8_t {
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

enum class I2C_DeviceNum { I2C_D1, I2C_D2, I2C_D3, TotalNum };

enum class I2C_FreqHz : uint8_t { _100KHz, _400Khz };

enum class I2C_Addressing_Mode : uint8_t { AddressMode_7Bit, AddressMode_10Bit };

enum class I2C_Mode : uint8_t { NONE, MASTER, SLAVE, MEM_READ, MEM_WRITE };

struct I2C_Config {
    I2C_DeviceNum       DevNum;
    I2C_FreqHz          ClockFreq;
    uint32_t            OwnAddress1;
    I2C_Addressing_Mode AddressingMode;
    uint32_t            DualAddressMode;
    uint32_t            OwnAddress2;
};

class Stm32I2C {
   protected:
    I2C_TypeDef*           i2c_ = nullptr;
    I2C_Config             config_;
    std::atomic<I2C_State> state_{I2C_State::RESET};
    std::atomic<I2C_Error> error_{I2C_Error::NONE};
    I2C_Mode               mode_ = I2C_Mode::NONE;

   public:
    Stm32I2C() {}
    Stm32I2C(const I2C_Config& Config);
    void configure(const I2C_Config& Config);
    bool initialize();
    bool Write(uint16_t DevAddress, const uint8_t* pData, uint16_t Size, uint32_t Timeout);
    bool Read(uint16_t DevAddress, uint8_t* pData, uint16_t Size, uint32_t Timeout);
    void handleEVInterrupt();
    void handleERInterrupt();

   protected:
    bool isHardwareBusy(const uint32_t& Timeout);
    bool WaitForFlagTimeout(volatile uint32_t& sr, const uint32_t& mask, const uint32_t& Timeout);
    void Error_Handler();
    void resetPeripheral();

    // Start, Stop Generation
    void generateStartCondition();
    void generateStopCondition();

    void enableAckBit();
    void disableAckBit();

    void enableI2C();
    void disableI2C();

    void enableNVICInterrupt();

    // Clear Flag
    void clearAddrFlag();
    void clearAFFlag();
    void clearBERRFlag();
    void clearARLOFLag();
    void clearOVRFLag();

   private:
    void enablePeripheralClock();
    void configureI2C();
    void configurePeripheralFreq();
    void configureAddressingMode();
    void configureCCRandTrise();

    bool WriteTo7BitDevice(uint16_t DevAddress, const uint8_t* pData, uint16_t Size, uint32_t Timeout);
    bool ReadFrom7BitDevice(uint16_t DevAddress, uint8_t* pData, uint16_t Size, uint32_t Timeout);
};