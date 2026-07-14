#pragma once

#include <cstdint>

#include "low-level/gpio_bitfields.h"
#include "low-level/gpio_registers.h"
#include "low-level/gpio_types.h"

enum class GPIO_Port : uint8_t { GPIO_PA, GPIO_PB, GPIO_PC, GPIO_PD, GPIO_PE, GPIO_PH, GPIO_PORT_COUNT };

enum class GPIO_State : uint8_t { LOW, HIGH };

enum class GPIO_Moder : uint8_t { GPIO_MODE_INPUT = 0, GPIO_MODE_OUTPUT, GPIO_MODE_ALTFN, GPIO_MODE_ANALOG };

enum class GPIO_OType : uint8_t { GPIO_OTYPER_PP = 0, GPIO_OTYPER_OD };

enum class GPIO_PUPDR : uint8_t { GPIO_PUPDR_NOPULL = 0, GPIO_PUPDR_PULLUP, GPIO_PUPDR_PULLDOWN };

enum class GPIO_OSPDR : uint8_t { GPIO_OSPEEDR_LS = 0, GPIO_OSPEEDR_MS, GPIO_OSPEEDR_HS, GPIO_OSPEEDR_VHS };

enum class GPIO_AFR : uint8_t {
    GPIO_AF0_SYSTEM    = 0,
    GPIO_AF1_TIM1_2    = 1,
    GPIO_AF2_TIM3_5    = 2,
    GPIO_AF3_TIM9_11   = 3,
    GPIO_AF4_I2C1_3    = 4,
    GPIO_AF5_SPI       = 5,
    GPIO_AF6_SPI3      = 6,
    GPIO_AF7_USART1_2  = 7,
    GPIO_AF8_USART6    = 8,
    GPIO_AF9_I2C2_3    = 9,
    GPIO_AF10_OTG_FS   = 10,
    GPIO_AF12_SDIO     = 12,
    GPIO_AF15_EVENTOUT = 15
};

struct GPIO_Config {
    uint32_t   pin;
    GPIO_Port  port;
    GPIO_Moder mode;
    GPIO_OType otype;
    GPIO_OSPDR ospdr;
    GPIO_PUPDR pupdr;
    GPIO_AFR   afr;
};

class Stm32GpioPin {
   private:
    GPIO_TypeDef* gpio_ = nullptr;
    uint32_t      pin_  = 0;

   public:
    Stm32GpioPin() = default;
    Result<>   initialize(const GPIO_Config& config);
    void       write(GPIO_State state);
    GPIO_State read() const;
    void       toggle();
};

namespace Gpio {
[[nodiscard]] Result<> configureMux(const GPIO_Config& cfg);
}  // namespace Gpio