#pragma once

#include <cstdint>

#include "RingBuffer.hpp"
#include "cpp/Dma.hpp"
#include "cpp/Stm32I2C.hpp"
#include "pch.hpp"

struct XferParams {
    uint32_t DevAddr;
    uint32_t XferSize;
};

// Tags for the ISR-safe diagnostic log — written in ISR, consumed in processRx()
enum class I2C_IsrTag : uint8_t {
    EV_ENTRY, EV_SB, EV_ADDR_TX, EV_ADDR_RX,
    EV_BTF_STOP, EV_BTF_STALL, EV_TXE,
    ER_AF, ER_BERR, ER_ARLO, ER_OVR,
    TX_DMA_FIRED, TX_DMA_DONE, RX_DMA_DONE
};

class DmaI2C : public Stm32I2C {
   private:
    using rxCallback = std::function<void()>;

   public:
    bool initialize() override;
    bool Write(uint16_t DevAddress, const uint8_t* pData, uint16_t Size, uint32_t Timeout) override;
    bool Read(uint16_t DevAddress, uint8_t* pData, uint16_t Size, uint32_t Timeout) override;
    void handleEVInterrupt() override;
    void handleERInterrupt() override;
    void handleTxDmaInterrupt();
    void handleRxDmaInterrupt();
    void onDataReceived(rxCallback cb);
    void processRx();

   protected:
    void onPostI2CInit() override;
    IDma hdmatx;
    IDma hdmarx;

   private:
    void                                initTxDma();
    void                                initRxDma();

    bool                                isHardwareBusy(const uint32_t& Timeout) override;

    void                                enable_DMA_request_tx();
    void                                enable_DMA_request_rx();
    void                                disable_DMA_request();

    void                                enableInterrupt();
    void                                disableInterrupt();

    std::array<uint8_t, DMA_CHUNK_SIZE> dmaTxBuffer;
    RingBuffer<uint8_t, DMA_CHUNK_SIZE> rxBuffer;

    uint8_t                             DevAddr;
    uint8_t*                            XferPtr;
    uint8_t                             XferSize;
    volatile bool                       RxEventFlag   = false;
    volatile bool                       dma_complete_ = false;
    volatile bool                       rxDmaPending_ = false;
    /* Function Callback */
    rxCallback FuncPtr = nullptr;

    // ISR-safe diagnostic log: push_isr_event() is safe to call from any ISR;
    // drain_isr_log() must only be called from the main loop (processRx).
    void push_isr_event(I2C_IsrTag tag, uint32_t a = 0, uint32_t b = 0);
    void drain_isr_log();
#ifndef NDEBUG
    struct IsrEvent { I2C_IsrTag tag; uint32_t a; uint32_t b; };
    static constexpr size_t           kIsrLogSize   = 16;
    std::array<IsrEvent, kIsrLogSize>  isr_log_      = {};
    volatile uint8_t                   isr_log_head_ = 0;
    uint8_t                            isr_log_tail_ = 0;
#endif
};
