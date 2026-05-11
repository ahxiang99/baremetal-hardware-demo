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

USART_Status   USART_SendByte(USART_TypeDef* p_Instance, int ch);
USART_Status   USART_ReceiveByte(USART_TypeDef* p_Instance, char* pData);

USART_Status   USART_WriteRxBuffer();
USART_Status   USART_ReadRxBuffer(char* c);
uint16_t       USART_GetRxBufferSize();

USART_Status   USART_Data_Available();
void           USART2_IRQHandler(void);

#ifdef __cplusplus
}
#endif
#endif /* __MY_STM32F4_UART_DRIVER_H__ */