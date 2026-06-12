#pragma once

#include "../Middleware/RingBuffer.hpp"
#include "Stm32Uart.hpp"

constexpr uint32_t BUFF_CHUNK_SIZE = 1024;
constexpr uint32_t TXRX_CHUNK_SIZE = 128;

class InterruptUart : public Stm32Uart {
   public:
    using Stm32Uart::Stm32Uart;
    using rxCallback = void (*)(const uint8_t* pData, size_t len);

    bool initialize();
    bool send(const uint8_t* data, size_t DataLength);
    bool receive(uint8_t* data, size_t DataLength);

    void handleInterrupt();
    void onDataReceived(rxCallback cb);
    void processRx();

   protected:
    bool onPostUartInit();
    void onTxInterrupt();
    void onRxInterrupt();

   private:
    RingBuffer<uint8_t, BUFF_CHUNK_SIZE> TxBuffer;
    RingBuffer<uint8_t, BUFF_CHUNK_SIZE> RxBuffer;
    std::atomic_bool                     rxEventFlag{false};
    rxCallback                           dataCallback = nullptr;

    uint8_t*                             RxDataPtr    = nullptr;
    uint32_t                             XferDataSize = 0;

    void                                 start_transfer();
    void                                 start_receive();
};