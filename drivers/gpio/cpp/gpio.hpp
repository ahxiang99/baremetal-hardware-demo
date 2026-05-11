#ifndef __MY_STM32F4_GPIO_LIB_H__
#define __MY_STM32F4_GPIO_LIB_H__

#include "low-level/gpio.h"
#include "low-level/gpio_types.h"

using GPIO_Config = GPIO_InitTypeDef;
using GPIO_STATUS = gpio_status_t;

GPIO_InitTypeDef gpio_create_config(gpio_port_t, uint32_t, gpio_mode_t, gpio_otyper_t, gpio_ospeedr_t, gpio_pupdr_t, uint32_t);

class GPIO {
   private:
    GPIO_TypeDef* m_pInstance;
    GPIO_Config*  m_pConfig;
    bool          m_Init;

   public:
    GPIO(GPIO_InitTypeDef* _cfg);
    GPIO_STATUS InitDriver(GPIO_Config* p_Config);
    GPIO_STATUS ResetDriver();

    GPIO_STATUS SetPinConfig(GPIO_Config* p_Config);  // Set Pin Configuration

    GPIO_STATUS TogglePin(const uint16_t PIN);

    bool        IsInit();
};

#endif /* __MY_STM32F4_GPIO_LIB_H__ */