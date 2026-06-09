#pragma once

#include "IGpio.hpp"

class Stm32GpioPin : public IGpio {
   private:
    GPIO_TypeDef* gpio_;
    GPIO_Config   config_;

   public:
    Stm32GpioPin() {}
    Stm32GpioPin(GPIO_TypeDef* gpio, const GPIO_Config& config) : gpio_(gpio), config_(config) {}
    void       configure(GPIO_TypeDef* gpio, const GPIO_Config& config);
    bool       Init() override;
    void       Write(GPIO_State state) override;
    GPIO_State Read() override;
    void       Toggle() override;

   private:
    void enableGpioClock();
    void configurePin();
};
