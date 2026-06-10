#pragma once

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>

#include "RingBuffer.hpp"
#include "Stm32Uart.hpp"
#include "cpp/Dma.hpp"

class DmaUart : public Stm32Uart {
   public:
    DmaUart() {}

    using RxCallbackFn = void (*)(void*, const uint8_t*, size_t);

    void configure(const UartConfig& uart_cfg, const DMA_Config& txhdma_cfg,
                   const DMA_Config& rxhdma_cfg);
    bool send(const uint8_t* pData, size_t Size) override;

    void onDataReceived(RxCallbackFn fn, void* ctx);
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

    RxCallbackFn                            rx_fn_    = nullptr;
    void*                                   rx_ctx_   = nullptr;

    bool                                    rxEnabled = false;
    std::atomic_bool                        keyboardEventReady{false};
    void                                    enable_DMA_request();
    void                                    disable_DMA_request();
    void                                    initDmaTx();
    void                                    initDmaRx();
    bool                                    startNextDmaTransfer();
    bool                                    startNextDmaReceive();
};