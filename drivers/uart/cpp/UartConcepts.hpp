#pragma once

#include <concepts>
#include <cstdint>

#include "DmaUart.hpp"
#include "InterruptUart.hpp"
#include "Stm32Uart.hpp"
#include "low-level/uart_registers.h"

template <typename T>
concept UartDevice = requires(T uart_, std::span<const uint8_t> data, std::span<uint8_t> buf) {
	{ uart_.send(data) } -> std::convertible_to<bool>;
	{ uart_.receive(buf) } -> std::convertible_to<bool>;
	{ uart_.rawInstance() } -> std::convertible_to<USART_TypeDef *>;
};

static_assert(UartDevice<Stm32Uart>, "Must be Uart Class");
static_assert(UartDevice<InterruptUart>, "Must be Uart Class");
static_assert(UartDevice<DmaUart>, "Must be Uart Class");
