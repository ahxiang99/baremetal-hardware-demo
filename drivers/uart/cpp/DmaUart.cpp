#include "DmaUart.hpp"

#include <atomic>
#include <cstddef>
#include <cstdint>

#include "IUart.hpp"
#include "RegisterUtils.hpp"
#include "Stm32Uart.hpp"
#include "cpp/Dma.hpp"
#include "cpp/IUart.hpp"
#include "low-level/nvic.h"
#include "low-level/rcc.h"
#include "low-level/uart_bitfields.h"
#include "pch.hpp"

void DmaUart::onPostUartInit() {
    enable_DMA_request();
    initDmaTx();
    initDmaRx();
    startNextDmaReceive();
}

void DmaUart::configure(const UartConfig& uart_cfg, const DMA_Config& txhdma_cfg,
                        const DMA_Config& rxhdma_cfg) {
    Stm32Uart::configure(uart_cfg);
    hdmatx.setConfig(txhdma_cfg);
    hdmarx.setConfig(rxhdma_cfg);
}

bool DmaUart::send(const uint8_t* pData, size_t Size) {
    /* To Store all print data in Tx RingBuffer */
    if (Size > 0 && pData != nullptr) {
        for (size_t i = 0; i < Size; ++i) {
            txBuffer.push(pData[i]);
        }
        startNextDmaTransfer();
        return true;
    } else {
        return false;
    }
}

void DmaUart::initDmaTx() {
    hdmatx.initialize();
    /* Setup Transfer Completion Callback */
    hdmatx.setXferCpltCallback(
        [](void* ctx) {
            auto* self      = static_cast<DmaUart*>(ctx);
            self->tx_state_ = UartState::Ready;
            self->startNextDmaTransfer();
        },
        this);
}

void DmaUart::initDmaRx() {
    if (!rxEnabled) return;

    hdmarx.initialize();
    /* Setup Transfer Completion Callback */
    hdmarx.setXferCpltCallback(
        [](void* ctx) {
            auto* self = static_cast<DmaUart*>(ctx);
            self->rxBuffer.sync_dma_head(self->hdmarx.getDmaStream()->NDTR);
            self->startNextDmaReceive();
        },
        this);

    hdmarx.StartDataStream((uint32_t)&uart_->DR, rxBuffer.data_ptr(), DMA_CHUNK_SIZE);
}

void DmaUart::enable_DMA_request() {
    switch (config_.comm) {
        case UartComm::RX_ONLY:
            RegisterUtils::setBits(uart_->CR3, USART_CR3_DMAR);
            rxEnabled = true;
            break;
        case UartComm::TX_ONLY:
            RegisterUtils::setBits(uart_->CR3, USART_CR3_DMAT);
            break;
        case UartComm::RX_TX:
            RegisterUtils::setBits(uart_->CR3, USART_CR3_DMAT);
            RegisterUtils::setBits(uart_->CR3, USART_CR3_DMAR);
            rxEnabled = true;
            break;
        default:
            error_ = UartError::InvalidConfig;
            return;
    }
}

void DmaUart::disable_DMA_request() {
    switch (config_.comm) {
        case UartComm::RX_ONLY:
            RegisterUtils::clearBits(uart_->CR3, USART_CR3_DMAR);
            break;
        case UartComm::TX_ONLY:
            RegisterUtils::clearBits(uart_->CR3, USART_CR3_DMAT);
            break;
        case UartComm::RX_TX:
            RegisterUtils::clearBits(uart_->CR3, USART_CR3_DMAT);
            RegisterUtils::clearBits(uart_->CR3, USART_CR3_DMAR);
            break;
        default:
            error_ = UartError::InvalidConfig;
            return;
    }
}

void DmaUart::handleTxDmaInterrupt() {
    hdmatx.handleInterrupt();
}

void DmaUart::handleRxDmaInterrupt() {
    hdmarx.handleInterrupt();
}

bool DmaUart::startNextDmaTransfer() {
    if (txBuffer.empty()) {
        return false;
    }

    if (tx_state_ == UartState::Ready) {
        tx_state_     = UartState::BusyTx;

        uint32_t Size = (txBuffer.size() > DMA_CHUNK_SIZE) ? DMA_CHUNK_SIZE : txBuffer.size();

        for (size_t i = 0; i < Size; ++i) {
            dmaTxBuffer.at(i) = txBuffer.pop().value();
        }

        hdmatx.StartDataStream(reinterpret_cast<uint32_t>(dmaTxBuffer.data()), (uint32_t)&uart_->DR,
                               Size);
        return true;
    } else {
        return false;
    }
}

bool DmaUart::startNextDmaReceive() {
    if (!rxEnabled) return false;

    rx_state_ = UartState::BusyRx;
    if (!(uart_->CR1 & USART_CR1_IDLEIE)) {
        RegisterUtils::setBits(uart_->CR1, USART_CR1_IDLEIE);
    }
    if (!hdmarx.is_Enabled()) {
        hdmarx.StartDataStream((uint32_t)&uart_->DR, rxBuffer.data_ptr(), DMA_CHUNK_SIZE);
    }
    return true;
}

void DmaUart::processRx() {
    if (!keyboardEventReady.load(std::memory_order_acquire)) {
        return;
    }
    keyboardEventReady.store(false, std::memory_order_relaxed);

    if (rx_fn_) {
        // Pop data from Rx Buffer and Send to Function.
        rxBuffer.sync_dma_head(hdmarx.getDmaStream()->NDTR);

        size_t transfer_size = rxBuffer.size();

        if (transfer_size > DMA_CHUNK_SIZE) {
            transfer_size = DMA_CHUNK_SIZE;
        }

        std::array<uint8_t, DMA_CHUNK_SIZE> transfer_buffer;
        for (size_t i = 0; i < transfer_size; ++i) {
            auto temp = rxBuffer.pop();
            if (temp.has_value()) {
                transfer_buffer.at(i) = temp.value();
            }
        }
        rx_fn_(rx_ctx_, transfer_buffer.data(), transfer_size);
    }
}

void DmaUart::handleInterrupt() {
    const uint32_t sr = uart_->SR;
    if (sr & USART_SR_IDLE) {
        clearFlag();
        onIdleInterrupt();
    } else if (sr & USART_SR_ORE) {
        clearFlag();
    }
}

void DmaUart::onTxInterrupt() {}

void DmaUart::onRxInterrupt() {}

void DmaUart::onIdleInterrupt() {
    keyboardEventReady.store(true, std::memory_order_release);
}
void DmaUart::onDataReceived(RxCallbackFn fn, void* ctx) {
    rx_fn_  = fn;
    rx_ctx_ = ctx;
}
