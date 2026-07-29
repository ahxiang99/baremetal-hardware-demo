#pragma once

#include "RingBuffer.hpp"
#include "Result.hpp"
#include "Stm32Uart.hpp"
#include "low-level/uart_registers.h"

class InterruptUart: public Stm32Uart
{
      public:
	using Stm32Uart::Stm32Uart;
	using rxCallback = void (*)(void *ctx, const uint8_t *pData, size_t len);

	Result<> initialize(const UartConfig &cfg);
	bool send(std::span<const uint8_t> data);
	bool receive(std::span<uint8_t> buf);

	void handleInterrupt();
	void onDataReceived(rxCallback cb, void *ctx);
	void processRx();

      protected:
	Result<> onPostUartInit();
	void onTxInterrupt();
	void onRxInterrupt();

	void recoverTx() const;

      private:
	RingBuffer<uint8_t, BUFF_SIZE> TxBuffer;
	RingBuffer<uint8_t, BUFF_SIZE> RxBuffer;
	std::atomic_bool rxEventFlag{false};
	rxCallback fn_ = nullptr;
	void *ctx_;

	uint8_t *RxDataPtr = nullptr;
	uint32_t XferDataSize = 0;

	void start_transfer();
	void start_receive();
};
