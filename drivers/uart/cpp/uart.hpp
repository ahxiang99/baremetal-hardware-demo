#ifndef UART_HPP
#define UART_HPP

#include "low-level/uart.h"

class UARTDevice {
   private:
    USART_TypeDef*     m_pInstance;
    USART_InitTypeDef* m_pConfig;

    bool               m_Init;
    char               m_Buffer[UART_BUF_SIZE];
    uint16_t           m_LineIdx;

    USART_Status       SetupPin();
    USART_Status       Init_Device();

   public:
    UARTDevice();
    UARTDevice(USART_InitTypeDef* cfg);
    bool InitDriver(USART_InitTypeDef* cfg);

    /* USART Data Transmission Function */
    USART_Status DataAvailable();
    bool         HandleInput();  // For Command Line Input...

    USART_Status Print(const char* buffer, uint16_t size);  // Print to Console
    USART_Status Get(char* c);

    const char*  GetLine();
};

#endif