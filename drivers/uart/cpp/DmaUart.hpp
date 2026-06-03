#pragma once

#include <array>
#include <cstdint>
#include <functional>

#include "RingBuffer.hpp"
#include "Stm32Uart.hpp"
#include "cpp/Dma.hpp"

class DmaUart : public Stm32Uart {
   public:
    using Stm32Uart::Stm32Uart;
    using RxCallback = std::function<void(const uint8_t* data, size_t len)>;
    bool initialize() override;
    bool send(const uint8_t* pData, size_t Size) override;

    void onDataReceived(RxCallback cb);
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
    // DMA Handler
    IDma                                    hdmatx;
    IDma                                    hdmarx;

    RingBuffer<uint8_t, DMA_MAX_CHUNK_SIZE> txBuffer;
    std::array<uint8_t, DMA_CHUNK_SIZE>     dmaTxBuffer;

    RingBuffer<uint8_t, DMA_CHUNK_SIZE>     rxBuffer;
    RxCallback                              rxCallback         = nullptr;
    volatile bool                           rxEnabled          = false;
    volatile bool                           keyboardEventReady = false;
    void                                    enable_DMA_request();
    void                                    disable_DMA_request();
    void                                    initDmaTx();
    void                                    initDmaRx();
    bool                                    startNextDmaTransfer();
    bool                                    startNextDmaReceive();
};