#include "DmaUart.hpp"

#include <cstddef>
#include <cstdint>

#include "IUart.hpp"
#include "RegisterUtils.hpp"
#include "Stm32Uart.hpp"
#include "cpp/Dma.hpp"
#include "cpp/IUart.hpp"
#include "low-level/nvic.h"
#include "low-level/rcc.h"
#include "low-level/uart.h"
#include "low-level/uart_bitfields.h"
#include "pch.hpp"

void DmaUart::onPostUartInit() {
    enable_DMA_request();
    initDmaTx();
    initDmaRx();
    My_NVIC_EnableIRQ(DMA1_Stream6_IRQn);
    My_NVIC_EnableIRQ(DMA1_Stream5_IRQn);
    startNextDmaReceive();
}

bool DmaUart::initialize() {
    if (!Stm32Uart::initialize()) return false;
    return true;
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
    /* This DMA Config is for USART2 Tx */
    DMA_Config config;
    config.DMA_BaseAddress = DMA1;
    config.Channel         = DMA_Channel::Channel_4;
    config.Stream          = DMA_Stream::Stream_6;
    config.Direction       = DMA_Direction::DMA_MEMORY_TO_PERIPH;
    config.Mode            = DMA_Mode::Normal;

    hdmatx.setConfig(config);
    hdmatx.initialize();

    /* Setup Transfer Completion Callback */
    hdmatx.setXferCpltCallback([this]() {
        this->tx_state_ = UartState::Ready;
        this->startNextDmaTransfer();
    });
}

void DmaUart::initDmaRx() {
    if (!rxEnabled) return;

    /* This DMA Config is for USART2 Rx */
    DMA_Config config;
    config.DMA_BaseAddress = DMA1;
    config.Channel         = DMA_Channel::Channel_4;
    config.Stream          = DMA_Stream::Stream_5;
    config.Direction       = DMA_Direction::DMA_PERIPH_TO_MEMORY;
    config.Mode            = DMA_Mode::Circular;

    hdmarx.setConfig(config);
    hdmarx.initialize();

    /* Setup Transfer Completion Callback */
    hdmarx.setXferCpltCallback([this]() {
        this->rxBuffer.sync_dma_head(hdmarx.getDmaStream()->NDTR);
        this->startNextDmaReceive();
    });

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
            SetError(UartError::InvalidConfig);
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
            SetError(UartError::InvalidConfig);
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

        hdmatx.StartDataStream(reinterpret_cast<uint32_t>(dmaTxBuffer.data()), (uint32_t)&uart_->DR, Size);
        return true;
    } else {
        return false;
    }
}

bool DmaUart::startNextDmaReceive() {
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
    if (!keyboardEventReady) {
        return;
    }
    keyboardEventReady = false;

    if (rxCallback) {
        // Pop data from Rx Buffer and Send to Function.
        rxBuffer.sync_dma_head(DMA1->SMx[5].NDTR);

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
        rxCallback(transfer_buffer.data(), transfer_size);
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
    keyboardEventReady = true;
}
void DmaUart::onDataReceived(RxCallback cb) {
    rxCallback = cb;
}