#ifndef UART_REGISTERS_H
#define UART_REGISTERS_H

// @ Purpose : Register layouts

#include <stdint.h>

#include "low-level/rcc_registers.h"

#define __IO volatile

#define USART1_BASE (APB2PERIPH_BASE + 0x1000U)
#define USART6_BASE (APB2PERIPH_BASE + 0x1400U)
#define USART2_BASE (APB1PERIPH_BASE + 0x4400U)

typedef struct {
    __IO uint32_t SR;   /*!< USART Status register,                   Address offset: 0x00 */
    __IO uint32_t DR;   /*!< USART Data register,                     Address offset: 0x04 */
    __IO uint32_t BRR;  /*!< USART Baud rate register,                Address offset: 0x08 */
    __IO uint32_t CR1;  /*!< USART Control register 1,                Address offset: 0x0C */
    __IO uint32_t CR2;  /*!< USART Control register 2,                Address offset: 0x10 */
    __IO uint32_t CR3;  /*!< USART Control register 3,                Address offset: 0x14 */
    __IO uint32_t GTPR; /*!< USART Guard time and prescaler register, Address offset: 0x18 */
} USART_TypeDef;

#endif