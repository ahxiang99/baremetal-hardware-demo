#pragma once

#include "cpp/Stm32GpioPin.hpp"
#include "cpp/Stm32I2C.hpp"
#include "cpp/Stm32Timer.hpp"
#include "cpp/Stm32Uart.hpp"
#include "drivers.hpp"
#include "pch.hpp"
namespace board
{
inline constexpr SysClockConfig sys_cfg_84{SysClockSource::PLL,      {HSI_Freq_Hz, 8, 84, 2},
					   AHB_ClockDivision::DIV_1, APB_ClockDivision::DIV_2,
					   APB_ClockDivision::DIV_1, 3};

static_assert(isValidPllConfig(sys_cfg_84.PllCfg), "PLL config invalid");
inline constexpr ClockTree clock_tree = calcClockTree_v2(sys_cfg_84);

// Validate peripheral clocks:
static_assert(clock_tree.sysclk == 84'000'000, "SYSCLK must be 84MHz");
static_assert(clock_tree.apb1 <= 42'000'000, "APB1 overclock!");
static_assert(clock_tree.apb2 <= 84'000'000, "APB2 overclock!");

namespace uart2
{
/* Configure Uart2 Pin */
inline constexpr GPIO_Config gpio_cfg{.pin = GPIO_PIN_2 | GPIO_PIN_3,
				      .port = GPIO_Port::GPIO_PA,
				      .mode = GPIO_Moder::GPIO_MODE_ALTFN,
				      .otype = GPIO_OType::GPIO_OTYPER_PP,
				      .ospdr = GPIO_OSPDR::GPIO_OSPEEDR_VHS,
				      .pupdr = GPIO_PUPDR::GPIO_PUPDR_NOPULL,
				      .afr = GPIO_AFR::GPIO_AF7_USART1_2};
inline constexpr UartConfig cfg{.dev_num = UartDevice_t::USART_D2,
				.baudRate = UartBaudRate_t::BR_9600,
				.comm = UartComm_t::RX_TX,
				.parity = UartParity_t::NONE,
				.stopbits = UartStopBit_t::USART_CR2_STOP_1};

/* This DMA Config is for USART2 Tx */
inline constexpr DMA_Config hdmatx_cfg{.Peripheral = DMA_Peripheral::USART2_TX,
				       .Device = DMA_Device::DMA_D1,
				       .Stream = DMA_Stream::Stream_6,
				       .Channel = DMA_Channel::Channel_4,
				       .Direction = DMA_Direction::DMA_MEMORY_TO_PERIPH,
				       .Mode = DMA_Mode::Normal};

/* This DMA Config is for USART2 Rx */
inline constexpr DMA_Config hdmarx_cfg{.Peripheral = DMA_Peripheral::USART2_RX,
				       .Device = DMA_Device::DMA_D1,
				       .Stream = DMA_Stream::Stream_5,
				       .Channel = DMA_Channel::Channel_4,
				       .Direction = DMA_Direction::DMA_PERIPH_TO_MEMORY,
				       .Mode = DMA_Mode::Circular};

} // namespace uart2

namespace uart1
{

inline constexpr GPIO_Config gpio_cfg{.pin = GPIO_PIN_9 | GPIO_PIN_10,
				      .port = GPIO_Port::GPIO_PA,
				      .mode = GPIO_Moder::GPIO_MODE_ALTFN,
				      .otype = GPIO_OType::GPIO_OTYPER_PP,
				      .ospdr = GPIO_OSPDR::GPIO_OSPEEDR_VHS,
				      .pupdr = GPIO_PUPDR::GPIO_PUPDR_NOPULL,
				      .afr = GPIO_AFR::GPIO_AF7_USART1_2};

inline constexpr UartConfig cfg{.dev_num = UartDevice_t::USART_D1,
				.baudRate = UartBaudRate_t::BR_460800,
				.comm = UartComm_t::RX_TX,
				.parity = UartParity_t::NONE,
				.stopbits = UartStopBit_t::USART_CR2_STOP_1};

} // namespace uart1

namespace i2c1
{
/* Configure i2c1 Pin */
inline constexpr GPIO_Config gpio_cfg{.pin = GPIO_PIN_8 | GPIO_PIN_9,
				      .port = GPIO_Port::GPIO_PB,
				      .mode = GPIO_Moder::GPIO_MODE_ALTFN,
				      .otype = GPIO_OType::GPIO_OTYPER_OD,
				      .ospdr = GPIO_OSPDR::GPIO_OSPEEDR_VHS,
				      .pupdr = GPIO_PUPDR::GPIO_PUPDR_PULLUP,
				      .afr = GPIO_AFR::GPIO_AF4_I2C1_3

};
inline constexpr i2c_config_t cfg{
	.DevNum = i2c_device_t::I2C_D1,
	.ClockFreq = i2c_freq_t::_100KHz,
	.OwnAddress1 = 0,
	.AddressingMode = i2c_addressmode_t::AddressMode_7Bit,
	.DualAddressMode = 0,
	.OwnAddress2 = 0,
};

inline constexpr DMA_Config config_tx{
	.Device = DMA_Device::DMA_D1,
	.Stream = DMA_Stream::Stream_7,
	.Channel = DMA_Channel::Channel_1,
	.Direction = DMA_Direction::DMA_MEMORY_TO_PERIPH,
	.Mode = DMA_Mode::Normal,
};

inline constexpr DMA_Config config_rx{
	.Device = DMA_Device::DMA_D1,
	.Stream = DMA_Stream::Stream_0,
	.Channel = DMA_Channel::Channel_1,
	.Direction = DMA_Direction::DMA_PERIPH_TO_MEMORY,
	.Mode = DMA_Mode::Normal,
};

} // namespace i2c1

namespace spi1
{
/* Configure spi1 Pin */
inline constexpr GPIO_Config gpio_cfg{.pin = GPIO_PIN_3 | GPIO_PIN_5,
				      .port = GPIO_Port::GPIO_PB,
				      .mode = GPIO_Moder::GPIO_MODE_ALTFN,
				      .otype = GPIO_OType::GPIO_OTYPER_PP,
				      .ospdr = GPIO_OSPDR::GPIO_OSPEEDR_VHS,
				      .pupdr = GPIO_PUPDR::GPIO_PUPDR_NOPULL,
				      .afr = GPIO_AFR::GPIO_AF5_SPI};
/* SPI */
inline constexpr Stm32Spi::Config cfg{.dev = Stm32Spi::SpiDev::SPI_D1,
				      .mode = Stm32Spi::Mode::Master,
				      .cpol = Stm32Spi::ClockPolarity::IdleLow,
				      .cpha = Stm32Spi::ClockPhase::FirstEdge,
				      .dataSize = Stm32Spi::DataSize::Bits8,
				      .bitOrder = Stm32Spi::BitOrder::MsbFirst,
				      .baudRatePrescalar = 5,
				      .nssSoftware = true};
} // namespace spi1

namespace led
{
/* Blinking LED */
inline constexpr GPIO_Config gpio_cfg{.pin = GPIO_PIN_5,
				      .port = GPIO_Port::GPIO_PA,
				      .mode = GPIO_Moder::GPIO_MODE_OUTPUT,
				      .otype = GPIO_OType::GPIO_OTYPER_PP,
				      .ospdr = GPIO_OSPDR::GPIO_OSPEEDR_LS,
				      .pupdr = GPIO_PUPDR::GPIO_PUPDR_NOPULL,
				      .afr = GPIO_AFR::GPIO_AF0_SYSTEM};
} // namespace led

namespace exti
{
/* Configure for Button Input */
inline constexpr GPIO_Config pc13_gpio_cfg{.pin = GPIO_PIN_13,
					   .port = GPIO_Port::GPIO_PC,
					   .mode = GPIO_Moder::GPIO_MODE_INPUT,
					   .otype = GPIO_OType::GPIO_OTYPER_PP,
					   .ospdr = GPIO_OSPDR::GPIO_OSPEEDR_LS,
					   .pupdr = GPIO_PUPDR::GPIO_PUPDR_PULLUP,
					   .afr = GPIO_AFR::GPIO_AF0_SYSTEM};
} // namespace exti

namespace tim3
{
inline constexpr TimerConfig cfg{.Instance = TimerDevice_t::TIMER_3,
				 .AlignedMode = TimerCenterAlignedMode_t::EDGE,
				 .Direction = TimerDirection_t::UP,
				 .ClockDivision = TimerClockDivision_t::TIM_CLOCKDIVISION_DIV1,
				 .AutoReloadPreload = TimerARR_t::ENABLE};
}

} // namespace board
