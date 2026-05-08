#include "Inc/libs/my_stm32f4_uart_lib.h"

#define BRR_CNF1_9600 0x683      // Baud rate configuration for 9600 baud (assuming 16 MHz clock)
#define BRR_CNF1_115200 0x008B   // Baud rate configuration for 115200 baud (assuming 16 MHz clock)
#define CR1_CNF1_TX (1 << 3)     // Enable transmitter
#define CR1_CNF1_RX (1 << 2)     // Enable receiver
#define CR2_CNF1 0x0000          // 1 stop bit
#define CR3_CNF1 0x0000          // No flow control
#define UART2_CR1_EN1 (1 << 13)  // Enable USART
#define UART2_CR1_DIS 0x0000     // Disable USART

#define USART2_BAUDRATE_9600 0x683

UARTComm::UARTComm(UART_ComType _comType, UART_BaudRateType _baudRate) : comType(_comType), baudRate(_baudRate) {}

void UARTComm::LIB_UART_Init() {
    RCC->AHB1ENR |= (1 << 0);   // Enable GPIOA clock
    RCC->APB1ENR |= (1 << 17);  // Enable USART2 clock

    USART2->CR1 = UART2_CR1_DIS;  // Disable USART before configuration

    switch (comType) {
        case TX_ONLY:
            // PIN configuration for TX (PA2)
            GPIOA->MODER &= ~(0x3 << (2 * 2));   // Clear mode for PA2
            GPIOA->MODER |= (0x2 << (2 * 2));    // Set mode to alternate function for PA2
            GPIOA->AFR[0] &= ~(0xF << (2 * 4));  // Clear alternate function for PA2
            GPIOA->AFR[0] |= (0x7 << (2 * 4));   // Set alternate function 7 (USART2) for PA2
            USART2->CR1 = CR1_CNF1_TX;           // Enable transmitter, 8 bits data
            break;
        case RX_ONLY:
            // PIN configuration for RX (PA3)
            GPIOA->MODER &= ~(0x3 << (3 * 2));   // Clear mode for PA3
            GPIOA->MODER |= (0x2 << (3 * 2));    // Set mode to alternate function for PA3
            GPIOA->AFR[0] &= ~(0xF << (3 * 4));  // Clear alternate function for PA3
            GPIOA->AFR[0] |= (0x7 << (3 * 4));   // Set alternate function 7 (USART2) for PA3
            USART2->CR1 = CR1_CNF1_RX;           // Enable receiver, 8 bits data
            break;
        case RX_TX:
            // PIN configuration for TX (PA2)
            GPIOA->MODER &= ~(0x3 << (2 * 2));   // Clear mode for PA2
            GPIOA->MODER |= (0x2 << (2 * 2));    // Set mode to alternate function for PA2
            GPIOA->AFR[0] &= ~(0xF << (2 * 4));  // Clear alternate function for PA2
            GPIOA->AFR[0] |= (0x7 << (2 * 4));   // Set alternate function 7 (USART2) for PA2
            // PIN configuration for RX (PA3)
            GPIOA->MODER &= ~(0x3 << (3 * 2));        // Clear mode for PA3
            GPIOA->MODER |= (0x2 << (3 * 2));         // Set mode to alternate function for PA3
            GPIOA->AFR[0] &= ~(0xF << (3 * 4));       // Clear alternate function for PA3
            GPIOA->AFR[0] |= (0x7 << (3 * 4));        // Set alternate function 7 (USART2) for PA3
            USART2->CR1 = CR1_CNF1_TX | CR1_CNF1_RX;  // Enable both transmitter and receiver
            break;
        default:
            break;
    }

    switch (baudRate) {
        case _115200:
            USART2->BRR = BRR_CNF1_115200;  // Baud rate configuration for 115200 baud (assuming 16 MHz clock)
            break;
        case _9600:
            USART2->BRR = BRR_CNF1_9600;  // Baud rate configuration for 9600 baud
            break;
        default:
            break;
    }

    USART2->CR2 = CR2_CNF1;        // 1 stop bit
    USART2->CR3 = CR3_CNF1;        // No flow control
    USART2->CR1 |= UART2_CR1_EN1;  // Enable USART
}

char UARTComm::LIB_UART_Read() const {
    while (!(USART2->SR & (1 << 5)));  // Wait for RXNE flag
    return USART2->DR;                 // Read the received data
}

void UARTComm::LIB_UART_WriteChar(int ch) {
    while (!(USART2->SR & (1 << 7)));  // Wait until TXE (Transmit Data Register Empty) is set
    USART2->DR = (ch & 0xFF);          // Write the character to the data register
}

void UARTComm::LIB_UART_Write(const char* string, uint16_t size) {
    for (uint16_t i = 0; i < size; ++i) {
        LIB_UART_WriteChar((int)string[i]);
    }
}

UART_ComType UARTComm::LIB_UART_GetComType() const {
    return comType;
}
UART_BaudRateType UARTComm::LIB_UART_GetBaudRate() const {
    return baudRate;
}