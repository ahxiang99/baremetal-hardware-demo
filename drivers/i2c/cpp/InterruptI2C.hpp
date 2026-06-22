#pragma once

#include <array>
#include <cstdint>
#include <functional>

#include "RingBuffer.hpp"
#include "Stm32I2C.hpp"
#include "pch.hpp"

class InterruptI2C : public Stm32I2C {
   protected:
    uint32_t DevAddr;
    uint8_t* XferPtr;
    uint32_t XferSize;
    bool     isHardwareBusy(const uint32_t& Timeout);
    void     onPostI2CInit();

   public:
    using rxCallback = void (*)(void);
    bool initialize();
    bool Write(uint16_t DevAddress, const uint8_t* pData, uint16_t Size, uint32_t Timeout);
    bool Read(uint16_t DevAddress, uint8_t* pData, uint16_t Size, uint32_t Timeout);
    void handleEVInterrupt();
    void handleERInterrupt();

    void processRx();
    void onDataReceived(rxCallback cb);

   private:
    rxCallback                      dataCallback = nullptr;
    std::atomic_bool                RxEventFlag{false};
    RingBuffer<uint8_t, CHUNK_SIZE> TxBuffer;
    RingBuffer<uint8_t, CHUNK_SIZE> RxBuffer;

    void                            enableInterruptFlag();
    void                            disableInterruptFlag();
};