#ifndef GPIO_H
#define GPIO_H

#ifdef __cplusplus
extern "C" {
#endif

#include <assert.h>
#include <stddef.h>
#include <stdint.h>

#include "gpio_bitfields.h"
#include "gpio_registers.h"
#include "gpio_types.h"

/* Driver Prototypes */
status_t GPIO_HardwareInit(gpio_port_t port, GPIO_TypeDef** p_Instance);
status_t GPIO_HardwareReset(gpio_port_t port);
status_t GPIO_PinSetConfig(GPIO_TypeDef* p_Instance, const GPIO_InitTypeDef* p_Config);
status_t GPIO_WriteOutputPin(GPIO_TypeDef* p_Instance, const uint16_t PIN, const gpio_pin_state_t state);
status_t GPIO_ReadInputPin(GPIO_TypeDef* p_Instance, const uint16_t GPIO_PIN, gpio_pin_state_t* state);
status_t GPIO_ToggleOutputPin(GPIO_TypeDef* p_Instance, const uint16_t GPIO_PIN);

#ifdef __cplusplus
}
#endif

#endif