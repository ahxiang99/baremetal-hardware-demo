#pragma once

#include <concepts>
#include <cstdint>

#include "Stm32GpioPin.hpp"

template <typename T>
concept GpioPin = requires(T p) {
    { p.Init(std::declval<const GPIO_Config&>()) } -> std::convertible_to<bool>;
    p.Write(std::declval<GPIO_State>());
    { p.Read() } -> std::convertible_to<GPIO_State>;
    p.Toggle();
};

template <typename T>
concept IsGpioConfig = std::same_as<T, GPIO_Config>;

template <GpioPin T, IsGpioConfig... Config>
bool Startup(T& gpio, const Config&... cfg) {
    bool result = true;
    ((result && (result = gpio.Init(cfg))), ...);
    return result;
}
