#ifndef __MY_STM32F4_UART_LIB_H__
#define __MY_STM32F4_UART_LIB_H__

#include "Inc/drivers/my_stm32f4_gpio_driver.h"
#include "Inc/drivers/my_stm32f4_uart_driver.h"

typedef enum { RX_ONLY = 0, TX_ONLY, RX_TX } UART_ComType;

typedef enum { _115200 = 0, _9600 } UART_BaudRateType;

class UARTComm {
   private:
    UART_ComType      comType;
    UART_BaudRateType baudRate;

   public:
    UARTComm(UART_ComType _comType, UART_BaudRateType _baudRate);
    void              LIB_UART_Init();
    char              LIB_UART_Read() const;
    void              LIB_UART_Write(int ch);
    UART_ComType      LIB_UART_GetComType() const;
    UART_BaudRateType LIB_UART_GetBaudRate() const;
};

#endif /* __MY_STM32F4_UART_LIB_H__ */