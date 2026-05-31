#pragma once

#include <functional>

#include "../Middleware/RingBuffer.hpp"
#include "Stm32Uart.hpp"

constexpr uint32_t BUFF_CHUNK_SIZE = 1024;
constexpr uint32_t TXRX_CHUNK_SIZE = 128;

class InterruptUart : public Stm32Uart {
   public:
    using Stm32Uart::Stm32Uart;
    bool initialize() override;
    bool send(const uint8_t* data, size_t DataLength) override;
    void send_IT();
    void start_receive();

    using rxCallback = std::function<void(const uint8_t* pData, size_t len)>;
    void handleInterrupt() override;

    void onDataReceived(rxCallback cb);
    void processRx();

   protected:
    void onPostUartInit() override;
    void onTxInterrupt() override;
    void onRxInterrupt() override;

   private:
    RingBuffer<uint8_t, BUFF_CHUNK_SIZE> TxBuffer;
    RingBuffer<uint8_t, BUFF_CHUNK_SIZE> RxBuffer;
    bool                                 rxEventFlag  = false;
    rxCallback                           dataCallback = nullptr;
};