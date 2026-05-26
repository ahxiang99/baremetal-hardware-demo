#include "uart.h"

#include "bit_utils.h"
#include "low-level/nvic.h"
#include "low-level/rcc_bitfields.h"
#include "uart_bitfields.h"
#include "uart_types.h"

volatile char     rx_buffer[UART_BUF_SIZE];
volatile uint16_t rx_head = 0;
volatile uint16_t rx_tail = 0;

void              USART2_IRQHandler(void) {
    if (USART2->SR & USART_SR_RXNE) {
        USART_WriteRxBuffer();
    }
}

USART_Status USART_HardwareInit(USART_InitTypeDef* p_Config) {
    if (!p_Config) return USART_ERR;

    USART_TypeDef* p_UART = USART_GetBaseAddress(p_Config->device);
    if (!p_UART) return USART_ERR;

    // 1. Enable USART Clock
    switch (p_Config->device) {
        case USART_D1:
            SET_BIT(RCC->APB2ENR, RCC_APB2ENR_USART1_EN);
            break;
        case USART_D2:
            SET_BIT(RCC->APB1ENR, RCC_APB1ENR_USART2_EN);
            break;
        case USART_D6:
            SET_BIT(RCC->APB2ENR, RCC_APB2ENR_USART6_EN);
            break;
    }
    // Disable USART Device during configuration
    CLEAR_BIT(p_UART->CR1, USART_CR1_UE);

    // 2. Configure Baud Rate
    SET_BIT(p_UART->BRR, p_Config->BaudRate);

    // 3. Configure Comm Type
    switch (p_Config->CommType) {
        case RX_ONLY:
            SET_BIT(p_UART->CR1, USART_CR1_RE);
            CLEAR_BIT(p_UART->CR1, USART_CR1_TE);
            break;
        case TX_ONLY:
            SET_BIT(p_UART->CR1, USART_CR1_TE);
            CLEAR_BIT(p_UART->CR1, USART_CR1_RE);
            break;
        case RX_TX:
            SET_BIT(p_UART->CR1, USART_CR1_RE);
            SET_BIT(p_UART->CR1, USART_CR1_TE);
            break;
    }

    // 4. Enable Interrupt
    My_NVIC_EnableIRQ(38);

    // 5. Enable USART Device
    SET_BIT(p_UART->CR1, USART_CR1_UE);
    return USART_OK;
}

USART_TypeDef* USART_GetBaseAddress(USART_DevNum dev_num) {
    switch (dev_num) {
        case USART_D1:
            return USART1;
        case USART_D2:
            return USART2;
        case USART_D6:
            return USART6;
        default:
            return 0x0U;
    }
}

USART_Status USART_SendByte(USART_TypeDef* p_Instance, int ch) {
    // Polling Method
    if (!p_Instance) return USART_ERR;

    while (!(p_Instance->SR & USART_SR_TXE));
    p_Instance->DR = (ch & 0xFF);
    return USART_OK;
}

USART_Status USART_ReceiveByte(USART_TypeDef* p_Instance, char* pData) {
    // Polling Method
    if (!p_Instance || !pData) return USART_ERR;

    while (!(p_Instance->SR & USART_SR_RXNE));
    *pData = p_Instance->DR;
    return USART_OK;
}

USART_Status USART_WriteRxBuffer(void) {
    char data = (char)USART2->DR;

    // Push data into buffer
    uint16_t next = (rx_head + 1) & (64 - 1);

    if (next != rx_tail) {
        rx_buffer[rx_head] = data;
        rx_head            = next;
    }
    return USART_OK;
}

USART_Status USART_ReadRxBuffer(char* c) {
    if (rx_head == rx_tail) return USART_ERR;
    *c      = rx_buffer[rx_tail];
    rx_tail = (rx_tail + 1) % 256;
    return USART_OK;
}

uint16_t USART_GetRxBufferSize(void) {
    return rx_head - rx_tail;
}