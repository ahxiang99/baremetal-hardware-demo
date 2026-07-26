#pragma once

#include "UartConcepts.hpp"
#include "low-level/uart_registers.h"

struct UartRef {
	void *obj{nullptr};
	bool (*send_fn)(void *, std::span<const uint8_t>){nullptr};
	bool (*receive_fn)(void *, std::span<uint8_t>){nullptr};
	USART_TypeDef *(*get_fn)(void *);

	template <UartDevice Uart> static UartRef from(Uart &uart)
	{
		return {&uart,
			[](void *ctx, std::span<const uint8_t> data) {
				return static_cast<Uart *>(ctx)->send(data);
			},
			[](void *ctx, std::span<uint8_t> buf) {
				return static_cast<Uart *>(ctx)->receive(buf);
			},
			[](void *ctx) { return static_cast<Uart *>(ctx)->rawInstance(); }};
	}

	bool send(std::span<const uint8_t> data)
	{
		return send_fn ? send_fn(obj, data) : false;
	}

	bool receive(std::span<uint8_t> buf)
	{
		return receive_fn ? receive_fn(obj, buf) : false;
	}

	USART_TypeDef *rawInstance() const
	{
		return get_fn ? get_fn(obj) : nullptr;
	}
};
