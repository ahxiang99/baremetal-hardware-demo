#ifndef UART_H
#define UART_H

#include "uart_bitfields.h"
#include "uart_registers.h"
#include "uart_types.h"

#ifdef __cplusplus
extern "C" {
#endif

USART_Status   USART_HardwareInit(USART_InitTypeDef* p_Config);
USART_TypeDef* USART_GetBaseAddress(USART_DevNum dev);

USART_Status   USART_Transmit(USART_TypeDef* p_Instance, uint8_t ch);
USART_Status   USART_Receive(USART_TypeDef* p_Instance, char* pData);

void           USART2_IRQHandler(void);

#ifdef __cplusplus
}
#endif
#endif /* __MY_STM32F4_UART_DRIVER_H__ */