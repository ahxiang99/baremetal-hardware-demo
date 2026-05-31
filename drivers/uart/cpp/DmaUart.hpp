#pragma once

#include <array>
#include <cstdint>

#include "RingBuffer.hpp"
#include "Stm32Uart.hpp"

constexpr size_t DMA_CHUNK_SIZE     = 128;
constexpr size_t DMA_MAX_CHUNK_SIZE = 1024;

class DmaUart : public Stm32Uart {
   public:
    using Stm32Uart::Stm32Uart;
    using RxCallback = void (*)(const uint8_t* data, size_t len);
    bool initialize() override;
    bool send(const uint8_t* data, size_t DataLength) override;

    void setRxCallback(RxCallback cb);
    void processRx();

    void handleInterrupt() override;
    void handleTxDmaInterrupt() override;
    void handleRxDmaInterrupt() override;

   protected:
    // Interupt Handler
    void onPostUartInit() override;

    void onTxInterrupt() override;
    void onRxInterrupt() override;
    void onIdleInterrupt() override;

   private:
    RingBuffer<uint8_t, DMA_MAX_CHUNK_SIZE> txBuffer;
    std::array<uint8_t, DMA_CHUNK_SIZE>     dmaTxBuffer;

    RingBuffer<uint8_t, DMA_CHUNK_SIZE>     rxBuffer;
    RxCallback                              rxCallback         = nullptr;
    bool                                    rxEnabled          = false;
    bool                                    keyboardEventReady = false;
    void                                    enableDmaMode();
    void                                    initDmaTx();
    void                                    initDmaRx();
    bool                                    startNextDmaTransfer();
    bool                                    startNextDmaReceive();
};