#ifndef GPIO_H
#define GPIO_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include "gpio_bitfields.h"
#include "gpio_registers.h"
#include "gpio_types.h"

/* Driver Prototypes */
GPIO_TypeDef* GPIO_HardwareInit(gpio_port_t _port);
GPIO_TypeDef* GPIO_GetBaseAddress(gpio_port_t _port);
gpio_status_t GPIO_HardwareReset(gpio_port_t _port);
gpio_status_t GPIO_PinSetConfig(GPIO_TypeDef* GPIOx, const GPIO_InitTypeDef* IO_cfg);
gpio_status_t GPIO_WriteOutputPin(GPIO_TypeDef* GPIOx, const uint16_t PIN, const gpio_pin_state_t state);
gpio_status_t GPIO_ReadInputPin(GPIO_TypeDef* GPIOx, const uint16_t GPIO_PIN, gpio_pin_state_t* state);
gpio_status_t GPIO_ToggleOutputPin(GPIO_TypeDef* GPIOx, const uint16_t GPIO_PIN);

#ifdef __cplusplus
}
#endif

#endif /* __MY_STM32F4_GPIO_DRIVER_H */