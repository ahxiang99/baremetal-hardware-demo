#pragma once

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <functional>

#include "Result.hpp"
#include "RingBuffer.hpp"
#include "Stm32Uart.hpp"
#include "cpp/Dma.hpp"

class DmaUart : public Stm32Uart {
    using RxCallbackFn = void (*)(void*, const uint8_t*, size_t);

   public:
    DmaUart();
    void                        configure(const UartConfig& uart_cfg, const DMA_Config& txdma_cfg, const DMA_Config& rxdma_cfg);
    Result<bool, UartInitError> initialize();

    bool                        send(const uint8_t* pData, size_t Size);
    bool                        receive(uint8_t* pData, size_t Size);

    void                        onDataReceived(RxCallbackFn fn, void* ctx);
    void                        processRx();

    void                        handleInterrupt();
    void                        handleTxDmaInterrupt();
    void                        handleRxDmaInterrupt();

   protected:
    // Interupt Handler
    void onPostUartInit();
    void onIdleInterrupt();

   private:
    // DMA Handler
    IDma                           hdmatx;
    IDma                           hdmarx;

    RingBuffer<uint8_t, BUFF_SIZE> txBuffer;
    std::array<uint8_t, BUFF_SIZE> dmaTxBuffer;
    RingBuffer<uint8_t, BUFF_SIZE> rxBuffer;

    RxCallbackFn                   rx_fn_    = nullptr;
    void*                          rx_ctx_   = nullptr;

    bool                           rxEnabled = false;
    std::atomic_bool               keyboardEventReady{false};
    volatile uint16_t              captured_ndtr_{0};
    void                           enable_DMA_request();
    void                           disable_DMA_request();
    void                           initDmaTx();
    void                           initDmaRx();
    bool                           startNextDmaTransfer();
    bool                           startNextDmaReceive();
};