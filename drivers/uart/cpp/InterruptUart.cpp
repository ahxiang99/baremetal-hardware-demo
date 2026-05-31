#include "InterruptUart.hpp"

#include "IUart.hpp"
#include "RegisterUtils.hpp"
#include "low-level/uart_bitfields.h"

bool InterruptUart::initialize() {
    if (!Stm32Uart::initialize()) {
        return false;
    }

    return true;
}

void InterruptUart::onPostUartInit() {
    if (config_.comm != UartComm::TX_ONLY) {
        start_receive();
    }
}

bool InterruptUart::send(const uint8_t* data, size_t DataLength) {
    if (DataLength <= 0 || data == nullptr) {
        return false;
    }

    for (size_t i = 0; i < DataLength; ++i) {
        TxBuffer.push(data[i]);
    }
    send_IT();
    return true;
}

void InterruptUart::send_IT() {
    if (tx_state_ != UartState::Ready) {
        return;
    }

    tx_state_ = UartState::BusyTx;

    // Enable Tx Interrupt
    RegisterUtils::setBits(uart_->CR1, USART_CR1_TXEIE);
}

void InterruptUart::start_receive() {
    if (rx_state_ != UartState::Ready) {
        return;
    }
    rx_state_ = UartState::BusyRx;

    // Enable Rx Interrupt
    RegisterUtils::setBits(uart_->CR1, USART_CR1_RXNEIE);
}

void InterruptUart::onTxInterrupt() {
    if (!TxBuffer.empty()) {
        uart_->DR = TxBuffer.pop().value();
    } else {
        RegisterUtils::clearBits(uart_->CR1, USART_CR1_TXEIE);
        tx_state_ = UartState::Ready;
    }
}

void InterruptUart::onRxInterrupt() {
    if (!RxBuffer.is_full() && rx_state_ == UartState::BusyRx) {
        RxBuffer.push(uart_->DR);
        rxEventFlag = true;
    } else {
        rx_state_ = UartState::Error;
        error_    = UartError::Overrun;
    }
}

void InterruptUart::handleInterrupt() {
    const uint32_t sr = uart_->SR;
    if (sr & USART_SR_RXNE) {
        onRxInterrupt();
    }
    if (sr & USART_SR_TXE) {
        onTxInterrupt();
    }
}

void InterruptUart::onDataReceived(rxCallback cb) {
    dataCallback = cb;
}

void InterruptUart::processRx() {
    if (!rxEventFlag) {
        return;
    }
    rxEventFlag = false;

    if (dataCallback) {
        // Pop data from Rx Buffer and Send to Function.
        size_t transfer_size = RxBuffer.size();

        if (transfer_size > TXRX_CHUNK_SIZE) {
            transfer_size = TXRX_CHUNK_SIZE;
        }

        std::array<uint8_t, TXRX_CHUNK_SIZE> transfer_buffer;
        for (size_t i = 0; i < transfer_size; ++i) {
            auto temp = RxBuffer.pop();
            if (temp.has_value()) {
                transfer_buffer.at(i) = temp.value();
            }
        }
        dataCallback(transfer_buffer.data(), transfer_size);
    }
}
