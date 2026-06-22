#include "InterruptUart.hpp"

#include <atomic>

#include "RegisterUtils.hpp"
#include "Stm32Uart.hpp"
#include "cpp/Stm32Uart.hpp"
#include "drivers.hpp"
#include "pch.hpp"

Result<bool, UartInitError> InterruptUart::initialize() {
    // 1. check uart_ not null
    if (uart_ == nullptr) {
        return Result<bool, UartInitError>::fail(UartInitError::NullPeripheral);
    }
    // 2. check already initialised
    if (m_Init) {
        return Result<bool, UartInitError>::fail(UartInitError::AlreadyInitialised);
    }
    // 3. call Stm32Uart::initialize()
    if (Stm32Uart::initialize()) {
        // 4. call onPostInit()
        if (!onPostUartInit()) {
            return Result<bool, UartInitError>::fail(UartInitError::InvalidPostInit);
        }
        return Result<bool, UartInitError>::success(true);
    }
    // return success or specific error
    return Result<bool, UartInitError>::fail(UartInitError::InvalidConfig);
}

bool InterruptUart::onPostUartInit() {
    enableNVICInterrupt();
    return error_ == UartError::None;
}

bool InterruptUart::send(const uint8_t* data, size_t DataLength) {
    if (DataLength <= 0 || data == nullptr) {
        return false;
    }

    for (size_t i = 0; i < DataLength; ++i) {
        TxBuffer.push(data[i]);
    }
    start_transfer();
    return true;
}

bool InterruptUart::receive(uint8_t* data, size_t DataLength) {
    if (DataLength <= 0 || data == nullptr) {
        return false;
    }
    RxDataPtr    = data;
    XferDataSize = DataLength;
    if (rx_state_ == UartState::Error) {
        // Call Recovery
        RxBuffer  = {};
        rx_state_ = UartState::Ready;
        error_    = UartError::None;
    }
    start_receive();
    return true;
}

void InterruptUart::start_transfer() {
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
    uint8_t byte = static_cast<uint8_t>(uart_->DR);
    if (!RxBuffer.is_full() && rx_state_ == UartState::BusyRx) {
        RxBuffer.push(byte);
    } else {
        rx_state_ = UartState::Error;
        error_    = UartError::Overrun;
    }
}

void InterruptUart::handleInterrupt() {
    const uint32_t sr = uart_->SR;
    if (sr & USART_SR_RXNE) {
        onRxInterrupt();
        XferDataSize--;
        if (XferDataSize == 0) {
            rxEventFlag.store(true, std::memory_order_release);
        }
    }
    if (sr & USART_SR_TXE) {
        onTxInterrupt();
    }
}

void InterruptUart::onDataReceived(rxCallback cb) {
    dataCallback = cb;
}

void InterruptUart::processRx() {
    if (!rxEventFlag.load(std::memory_order_acquire)) {
        return;
    }

    rxEventFlag.store(false, std::memory_order_relaxed);

    if (dataCallback) {
        // Pop data from Rx Buffer and Send to Function.

        size_t transfer_size = RxBuffer.size() > CHUNK_SIZE ? CHUNK_SIZE : RxBuffer.size();

        for (size_t i = 0; i < transfer_size; ++i) {
            auto byte = RxBuffer.pop();
            if (byte.has_value() && RxDataPtr) RxDataPtr[i] = byte.value();
        }
        dataCallback(RxDataPtr, transfer_size);
    }
}
void InterruptUart::recoverTx() {
    if (tx_state_ == UartState::BusyTx && !TxBuffer.empty()) {
        // TXEIE may have been lost — re-enable it
        RegisterUtils::setBits(uart_->CR1, USART_CR1_TXEIE);
    }
}
