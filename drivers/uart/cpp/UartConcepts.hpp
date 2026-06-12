#pragma once

#include <concepts>
#include <cstdint>

#include "DmaUart.hpp"
#include "InterruptUart.hpp"
#include "Stm32Uart.hpp"

template <typename T>
concept UartDevice = requires(T uart_, const uint8_t* data, uint8_t* buf, size_t len) {
    { uart_.send(data, len) } -> std::convertible_to<bool>;
    { uart_.receive(buf, len) } -> std::convertible_to<bool>;
};

static_assert(UartDevice<Stm32Uart>, "Must be Uart Class");
static_assert(UartDevice<InterruptUart>, "Must be Uart Class");
static_assert(UartDevice<DmaUart>, "Must be Uart Class");