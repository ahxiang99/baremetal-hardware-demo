#pragma once

#include <atomic>

#include "cpp/Stm32GpioPin.hpp"
#include "low-level/syscfg_registers.h"

class ExtiInput
{
	using ExtiFn = void (*)(void *);

      public:
	ExtiInput();

	Result<> initialize(const GPIO_Config &config, ExtiFn fn, void *ctx = nullptr);
	void handleInterrupt();
	void processEvent();

      private:
	void setFnCallback(ExtiFn fn, void *ctx);
	void enableClock();
	void configureExtiCr();
	void configureExtiMask();
	void configureGpioPin();
	void configureNvic();
	void clearMask();

	SysCfg_TypeDef *syscfg_instance_;
	Exti_TypeDef *exti_instance_;
	volatile uint32_t *exti_cr;

	Stm32GpioPin gpio_;
	GPIO_Config pin_config_;
	std::atomic_bool flag_{false};
	/* Function Callback For Interrupt */
	ExtiFn fn_ = nullptr;
	void *ctx_ = nullptr;
};
