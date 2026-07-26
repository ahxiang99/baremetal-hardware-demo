#include "DmaUart.hpp"

#include <atomic>
#include <cstddef>
#include <cstdint>

#include "RegisterUtils.hpp"
#include "Result.hpp"
#include "Stm32Uart.hpp"
#include "cpp/Dma.hpp"
#include "cpp/UartRef.hpp"
#include "low-level/nvic.h"
#include "low-level/rcc.h"
#include "low-level/uart_bitfields.h"
#include "pch.hpp"

DmaUart::DmaUart()
{
}

Result<> DmaUart::initialize(const UartConfig &uart_cfg, const DMA_Config &txdma_cfg,
			     const DMA_Config &rxdma_cfg)
{
	if (m_Init) {
		return Ok();
	}
	/* Setup DMA Config */
	hdmatx.setConfig(txdma_cfg);
	hdmarx.setConfig(rxdma_cfg);

	TRY(Stm32Uart::initialize(uart_cfg));

	if (uart_ == nullptr) {
		return Fail(Err::NullInstance);
	}

	onPostUartInit();
	m_Init = true;
	return Ok();
}

void DmaUart::onPostUartInit()
{
	enable_nvic(uart_);
	initDmaTx();
	initDmaRx();
	startNextDmaReceive();
}

bool DmaUart::send(std::span<const uint8_t> data)
{
	/* To Store all print data in Tx RingBuffer */
	if (data.size() > 0 && data.data() != nullptr) {
		for (size_t i = 0; i < data.size(); ++i) {
			txBuffer.push(data[i]);
		}
		startNextDmaTransfer();
		return true;
	} else {
		return false;
	}
}

void DmaUart::initDmaTx()
{
	hdmatx.initialize();

	/* Setup Transfer Completion Callback */
	hdmatx.setXferCpltCallback(
		[](void *ctx) {
			auto *self = static_cast<DmaUart *>(ctx);
			self->tx_state_ = UartState_t::Ready;
			self->startNextDmaTransfer();
		},
		this);
}

void DmaUart::initDmaRx()
{
	if (!rxEnabled) {
		return;
	}
	hdmarx.initialize();

	/* Setup Transfer Completion Callback */
	hdmarx.setXferCpltCallback(
		[](void *ctx) {
			auto *self = static_cast<DmaUart *>(ctx);
			self->rxBuffer.sync_dma_head(self->hdmarx.getDmaStream()->NDTR);
			self->startNextDmaReceive();
		},
		this);

	hdmarx.StartDataStream((uint32_t)&uart_->DR, rxBuffer.data_ptr(), BUFF_SIZE);
}

void DmaUart::enableTxDmaRequest()
{
	RegisterUtils::setBits(uart_->CR3, USART_CR3_DMAT);
}

void DmaUart::enableRxDmaRequest()
{
	RegisterUtils::setBits(uart_->CR3, USART_CR3_DMAT);
	RegisterUtils::setBits(uart_->CR3, USART_CR3_DMAR);
}

void DmaUart::disableTxDmaRequest()
{
	RegisterUtils::clearBits(uart_->CR3, USART_CR3_DMAT);
}

void DmaUart::disableRxDmaRequest()
{
	RegisterUtils::clearBits(uart_->CR3, USART_CR3_DMAT);
	RegisterUtils::clearBits(uart_->CR3, USART_CR3_DMAR);
}

void DmaUart::handleTxDmaInterrupt()
{
	hdmatx.handleInterrupt();
}

void DmaUart::handleRxDmaInterrupt()
{
	hdmarx.handleInterrupt();
}

bool DmaUart::startNextDmaTransfer()
{
	if (txBuffer.empty()) {
		disableTxDmaRequest();
		return false;
	}

	if (tx_state_ == UartState_t::Ready) {
		tx_state_ = UartState_t::BusyTx;

		uint32_t Size = (txBuffer.size() > CHUNK_SIZE) ? CHUNK_SIZE : txBuffer.size();

		for (size_t i = 0; i < Size; ++i) {
			dmaTxBuffer.at(i) = txBuffer.pop().value();
		}
		enableTxDmaRequest();
		hdmatx.StartDataStream(reinterpret_cast<uint32_t>(dmaTxBuffer.data()),
				       (uint32_t)&uart_->DR, Size);
		return true;
	} else {
		return false;
	}
}

bool DmaUart::startNextDmaReceive()
{
	if (!rxEnabled) {
		return false;
	}

	rx_state_ = UartState_t::BusyRx;
	if (!(uart_->CR1 & USART_CR1_IDLEIE)) {
		RegisterUtils::setBits(uart_->CR1, USART_CR1_IDLEIE);
	}
	if (!hdmarx.is_Enabled()) {
		hdmarx.StartDataStream((uint32_t)&uart_->DR, rxBuffer.data_ptr(), BUFF_SIZE);
	}
	return true;
}

void DmaUart::processRx()
{
	if (!keyboardEventReady.load(std::memory_order_acquire)) {
		return;
	}
	keyboardEventReady.store(false, std::memory_order_relaxed);

	if (rx_fn_) {
		// Pop data from Rx Buffer and Send to Function.
		rxBuffer.sync_dma_head(captured_ndtr_);

		size_t transfer_size = rxBuffer.size();

		if (transfer_size > CHUNK_SIZE) {
			transfer_size = CHUNK_SIZE;
		}

		std::array<uint8_t, CHUNK_SIZE> transfer_buffer;
		for (size_t i = 0; i < transfer_size; ++i) {
			auto temp = rxBuffer.pop();
			if (temp.has_value()) {
				transfer_buffer.at(i) = temp.value();
			}
		}
		rx_fn_(rx_ctx_, transfer_buffer.data(), transfer_size);
	}
}

bool DmaUart::receive(std::span<uint8_t> buf)
{
	return false;
}

void DmaUart::handleInterrupt()
{
	const uint32_t sr = uart_->SR;
	if (sr & USART_SR_IDLE) {
		clearFlag();
		onIdleInterrupt();
	} else if (sr & USART_SR_ORE) {
		clearFlag();
		error_ = UartError_t::Overrun;
		rxBuffer.sync_dma_head(hdmarx.getDmaStream()->NDTR);
	}
}

void DmaUart::onIdleInterrupt()
{
	captured_ndtr_ = static_cast<uint16_t>(hdmarx.getDmaStream()->NDTR);
	keyboardEventReady.store(true, std::memory_order_release);
}
void DmaUart::onDataReceived(RxCallbackFn fn, void *ctx)
{
	rx_fn_ = fn;
	rx_ctx_ = ctx;
}
