#ifndef HW_TESTS_HPP
#define HW_TESTS_HPP

#include <assert.h>

#include <cstdint>
#include <type_traits>

#include "dma/cpp/Dma.hpp"
#include "Register.hpp"
#include "Sht40ad1b.hpp"
#include "cpp/Dma.hpp"

// === Register access permissions ===
static_assert(sizeof(Register<0x40020000, uint32_t>) == 1, "Register should have no data members");

// === DMA config validation ===
constexpr DMA_Config dma_config_test{
    .Peripheral = DMA_Peripheral::I2C1_TX, .Stream = DMA_Stream::Stream_7, .Channel = DMA_Channel::Channel_1, .Direction = DMA_Direction::DMA_MEMORY_TO_PERIPH, .Mode = DMA_Mode::Normal};

static_assert(isValidDmaConfig(dma_config_test), "Invalid Config");

// === PLL config validation ===
constexpr PllConfig cfg_84MHz{16'000'000, 8, 84, 2};  // verified in Session 26
static_assert(isValidPllConfig(cfg_84MHz), "84MHz PLL config invalid");
static_assert(calcSysClock(cfg_84MHz) == 84'000'000, "SYSCLK must be 84MHz");

// === GpioPin concept enforcement ===
struct MockBadGpio {};  // no methods — should NOT satisfy GpioPin
static_assert(!GpioPin<MockBadGpio>, "MockBadGpio should fail GpioPin");

struct MockGoodGpio {
    bool Init(const GPIO_Config&) {
        return true;
    }
    void       Write(GPIO_State) {}
    GPIO_State Read() {
        return {};
    }
    void Toggle() {}
};
static_assert(GpioPin<MockGoodGpio>, "MockGoodGpio should satisfy GpioPin");

// === GPIO_Config size ===
static_assert(sizeof(GPIO_Config) == 12, "GPIO_Config bloated — check enum types");

// === SensorPacket layout ===
static_assert(sizeof(Sht40ad1b::SensorData) == sizeof(float) + sizeof(float), "SensorPacket has padding — binary layout broken");
#endif