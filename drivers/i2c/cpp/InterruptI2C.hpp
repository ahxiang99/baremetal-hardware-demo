#pragma once

#include <array>
#include <cstdint>
#include <functional>

#include "RingBuffer.hpp"
#include "Stm32I2C.hpp"

class InterruptI2C : public Stm32I2C {
   protected:
    uint32_t DevAddr;
    uint8_t* XferPtr;
    uint32_t XferSize;

    bool     isHardwareBusy(const uint32_t& Timeout) override;

   public:
    using rxCallback = std::function<void()>;
    bool initialize() override;
    bool Write(uint16_t DevAddress, const uint8_t* pData, uint16_t Size, uint32_t Timeout) override;
    bool Read(uint16_t DevAddress, uint8_t* pData, uint16_t Size, uint32_t Timeout) override;
    void handleEVInterrupt() override;
    void handleERInterrupt() override;

    void processRx();
    void onDataReceived(rxCallback cb);

   private:
    rxCallback               dataCallback = nullptr;
    volatile bool            RxEventFlag  = false;
    RingBuffer<uint8_t, 2>   TxBuffer;
    RingBuffer<uint8_t, 128> RxBuffer;

    void                     enableInterruptFlag();
    void                     disableInterruptFlag();
};