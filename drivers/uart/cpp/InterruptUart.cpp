#include "InterruptUart.hpp"

#include <atomic>

#include "RegisterUtils.hpp"
#include "Result.hpp"
#include "Stm32Uart.hpp"
#include "drivers.hpp"
#include "pch.hpp"

Result<> InterruptUart::initialize(const UartConfig &cfg)
{
	if (m_Init) {
		return Ok();
	}

	// call Stm32Uart::initialize()
	TRY(Stm32Uart::initialize(cfg));

	// call onPostInit()
	TRY(onPostUartInit());
	return Ok();
}

Result<> InterruptUart::onPostUartInit()
{
	TRY(enable_nvic(uart_));
	start_receive();
	return Ok();
}

bool InterruptUart::send(std::span<const uint8_t> data)
{
	if (data.empty()) {
		return false;
	}

	for (auto byte : data) {
		TxBuffer.push(byte);
	}
	
	start_transfer();
	return true;
}

bool InterruptUart::receive(std::span<uint8_t> buf)
{
	if (buf.data() == nullptr) {
		return false;
	}

	RxDataPtr = buf.data();
	XferDataSize = buf.size();
	if (rx_state_ == UartState_t::Error) {
		// Call Recovery
		RxBuffer = {};
		rx_state_ = UartState_t::Ready;
		error_ = UartError_t::None;
	}
	start_receive();
	return true;
}

void InterruptUart::start_transfer()
{
	if (tx_state_ != UartState_t::Ready) {
		return;
	}

	tx_state_ = UartState_t::BusyTx;

	// Enable Tx Interrupt
	RegisterUtils::setBits(uart_->CR1, USART_CR1_TXEIE);
}

void InterruptUart::start_receive()
{
	if (rx_state_ != UartState_t::Ready) {
		return;
	}
	rx_state_ = UartState_t::BusyRx;

	// Enable Rx Interrupt
	RegisterUtils::setBits(uart_->CR1, USART_CR1_RXNEIE);
}

void InterruptUart::onTxInterrupt()
{
	if (!TxBuffer.empty()) {
		uart_->DR = TxBuffer.pop().value();
	} else {
		RegisterUtils::clearBits(uart_->CR1, USART_CR1_TXEIE);
		tx_state_ = UartState_t::Ready;
	}
}

void InterruptUart::onRxInterrupt()
{
	auto byte = static_cast<uint8_t>(uart_->DR);
	if (!RxBuffer.is_full() && rx_state_ == UartState_t::BusyRx) {
		RxBuffer.push(byte);
	} else {
		rx_state_ = UartState_t::Error;
		error_ = UartError_t::Overrun;
	}
}

void InterruptUart::handleInterrupt()
{
	const uint32_t sr = uart_->SR;
	if (sr & USART_SR_RXNE) {
		onRxInterrupt();
		rxEventFlag.store(true, std::memory_order_release);
	}
	if (sr & USART_SR_TXE) {
		onTxInterrupt();
	}
}

void InterruptUart::onDataReceived(rxCallback fn, void *ctx)
{
	fn_ = fn;
	ctx_ = ctx;
}

void InterruptUart::processRx()
{
	if (!rxEventFlag.load(std::memory_order_acquire)) {
		return;
	}

	rxEventFlag.store(false, std::memory_order_relaxed);

	if (fn_) {
		// Pop data from Rx Buffer and Send to Function.
		size_t len = RxBuffer.size();
		std::array<uint8_t, 4> buf;
		for (size_t i = 0; i < len; ++i) {
			auto byte = RxBuffer.pop();
			if (byte.has_value()) {
				buf[i] = byte.value();
			}
		}
		fn_(ctx_, buf.data(), len);
	}
}
void InterruptUart::recoverTx() const
{
	if (tx_state_ == UartState_t::BusyTx && !TxBuffer.empty()) {
		// TXEIE may have been lost — re-enable it
		RegisterUtils::setBits(uart_->CR1, USART_CR1_TXEIE);
	}
}
