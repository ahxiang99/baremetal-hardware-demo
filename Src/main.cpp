#include "Inc/libs/my_stm32f4_uart_lib.h"

int main() {
    UARTComm uart(TX_ONLY, _115200);
    uart.LIB_UART_Init();

    while (1) {
        uart.LIB_UART_Write('H');
        uart.LIB_UART_Write('e');
        uart.LIB_UART_Write('l');
        uart.LIB_UART_Write('l');
        uart.LIB_UART_Write('o');
        uart.LIB_UART_Write('\n');
        for (volatile int i = 0; i < 1000000; ++i) {
            // Simple delay loop
        }
    }

    return 0;
}