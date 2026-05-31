#ifndef UART_TYPES_H
#define UART_TYPES_H

// @ Purpose : Enums and structs

#define UART_BUF_SIZE 64

typedef enum { USART_ERR = 0, USART_OK = 1, USART_BUSY = 2 } USART_Status;
typedef enum { _9600 = 0x683, _115200 = 0x008B } USART_BaudRate;
typedef enum { RX_ONLY = 0, TX_ONLY, RX_TX } USART_CommType;
typedef enum { USART_D1 = 0, USART_D2, USART_D6 } USART_DevNum;
typedef enum { None, Even, Odd } USART_Parity;

typedef struct {
    USART_DevNum   device;
    USART_CommType CommType;
    USART_BaudRate BaudRate;
    USART_Parity   Parity;
} USART_InitTypeDef;

#endif