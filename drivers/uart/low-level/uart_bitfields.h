#ifndef UART_BITFIELDS_H
#define UART_BITFIELDS_H

// @ Purpose : Masks and positions

/* Bit Mask */
#define USART_SR_PE (1 << 0)
#define USART_SR_FE (1 << 1)
#define USART_SR_NF (1 << 2)
#define USART_SR_ORE (1 << 3)
#define USART_SR_IDLE (1 << 4)
#define USART_SR_RXNE (1 << 5)
#define USART_SR_TC (1 << 6)
#define USART_SR_TXE (1 << 7)
#define USART_SR_LBD (1 << 8)
#define USART_SR_CTS (1 << 9)

#define USART_CR1_SBK (1 << 0)
#define USART_CR1_RWU (1 << 1)
#define USART_CR1_RE (1 << 2)
#define USART_CR1_TE (1 << 3)
#define USART_CR1_IDLEIE (1 << 4)
#define USART_CR1_RXNEIE (1 << 5)
#define USART_CR1_TCIE (1 << 6)
#define USART_CR1_TXEIE (1 << 7)
#define USART_CR1_PEIE (1 << 8)
#define USART_CR1_PS (1 << 9)
#define USART_CR1_PCE (1 << 10)
#define USART_CR1_WAKE (1 << 11)
#define USART_CR1_M (1 << 12)
#define USART_CR1_UE (1 << 13)
#define USART_CR1_OVER8 (1 << 15)

#define USART_CR2_LBDIE (1 << 6)

#define USART_CR3_EIE (1 << 0)
#define USART_CR3_CTSIE (1 << 10)

#define USART1 ((USART_TypeDef*)USART1_BASE)
#define USART2 ((USART_TypeDef*)USART2_BASE)
#define USART6 ((USART_TypeDef*)USART6_BASE)

#define USART1EN (1 << 4)
#define USART2EN (1 << 17)
#define USART6EN (1 << 5)

#endif