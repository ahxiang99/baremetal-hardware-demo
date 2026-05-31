#include "DmaUart.hpp"

#include <cstddef>
#include <cstdint>

#include "IUart.hpp"
#include "RegisterUtils.hpp"
#include "Stm32Uart.hpp"
#include "low-level/DMA.h"
#include "low-level/nvic.h"
#include "low-level/rcc.h"
#include "low-level/uart.h"

void DmaUart::onPostUartInit() {
    enableDmaMode();
    My_NVIC_EnableIRQ(DMA1_Stream6_IRQn);
    initDmaTx();
    initDmaRx();
    startNextDmaReceive();
}

bool DmaUart::initialize() {
    if (!Stm32Uart::initialize()) return false;

    return true;
}

bool DmaUart::send(const uint8_t* data, size_t DataLength) {
    // Push data into buffer.
    if (DataLength == 0 || data == nullptr) {
        return false;
    }

    for (size_t i = 0; i < DataLength; ++i) {
        txBuffer.push(data[i]);
    }
    startNextDmaTransfer();
    return true;
}

void DmaUart::initDmaTx() {
    // USART2 TX -> Channel 4 Stream 6
    volatile uint32_t* Reg  = &DMA1->SMx[6].CR;
    uint32_t           mask = DMA_SCR_EN | (0x3 << DMA_SCR_DIR_Pos);

    // Disable Stream 6
    RegisterUtils::clearBits(*Reg, mask);

    // Make Configuration
    mask = (4 << DMA_SCR_CHSEL_Pos) | (0x1 << DMA_SCR_DIR_Pos) | DMA_SCR_MINC | DMA_SCR_TCIE;
    RegisterUtils::setBits(*Reg, mask);

    DMA1->SMx[6].PAR = (uint32_t)&uart_->DR;
}

void DmaUart::initDmaRx() {
    if (!rxEnabled) return;

    // USART2 RX -> Channel 4 Stream 5
    volatile uint32_t* Reg  = &DMA1->SMx[5].CR;
    uint32_t           mask = DMA_SCR_EN | (0x3 << DMA_SCR_DIR_Pos);

    // Disable Stream 5
    RegisterUtils::clearBits(*Reg, mask);

    mask = (4 << DMA_SCR_CHSEL_Pos) | DMA_SCR_MINC | DMA_SCR_TCIE | DMA_SCR_CIRC;
    RegisterUtils::setBits(*Reg, mask);

    DMA1->SMx[5].PAR  = (uint32_t)&uart_->DR;
    DMA1->SMx[5].M0AR = rxBuffer.data_ptr();
    DMA1->SMx[5].NDTR = DMA_CHUNK_SIZE;
}

void DmaUart::enableDmaMode() {
    constexpr uint32_t mask   = RCC_AHB1ENR_DMA1_EN;
    volatile uint32_t* enrReg = &RCC->AHB1ENR;

    RegisterUtils::setBits(*enrReg, mask);

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

void DmaUart::handleTxDmaInterrupt() {
    if (DMA1->HISR & DMA_HISR_TCIF6) {
        DMA1->HIFCR |= DMA_HIFCR_CTCIF6;
        tx_state_ = UartState::Ready;
        DMA1->SMx[6].CR &= ~DMA_SCR_EN;
        while (DMA1->SMx[6].CR & DMA_SCR_EN);
        startNextDmaTransfer();
    }
}

void DmaUart::handleRxDmaInterrupt() {
    if (DMA1->HISR & DMA_HISR_TCIF5) {
        DMA1->HIFCR |= DMA_HIFCR_CTCIF5;
        rxBuffer.sync_dma_head(DMA1->SMx[5].NDTR);
    }
}

bool DmaUart::startNextDmaTransfer() {
    if (txBuffer.empty()) {
        return false;
    }

    if (tx_state_ != UartState::Ready) {
        return false;
    }

    tx_state_              = UartState::BusyTx;

    uint32_t transfer_size = txBuffer.size();

    if (transfer_size > DMA_CHUNK_SIZE) {
        transfer_size = DMA_CHUNK_SIZE;
    }

    for (size_t i = 0; i < transfer_size; ++i) {
        auto temp = txBuffer.pop();
        if (temp.has_value()) {
            dmaTxBuffer.at(i) = temp.value();
        }
    }

    DMA1->SMx[6].M0AR = reinterpret_cast<uint32_t>(dmaTxBuffer.data());
    DMA1->SMx[6].NDTR = transfer_size;
    DMA1->SMx[6].CR |= DMA_SCR_EN;
    return true;
}

bool DmaUart::startNextDmaReceive() {
    if (rx_state_ != UartState::Ready) {
        return false;
    }
    rx_state_ = UartState::BusyRx;
    uart_->CR1 |= USART_CR1_IDLEIE;
    DMA1->SMx[5].CR |= DMA_SCR_EN;
    return true;
}

void DmaUart::setRxCallback(RxCallback cb) {
    rxCallback = cb;
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
