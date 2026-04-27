#ifndef __MY_STM32F4_UART_DRIVER_H__
#define __MY_STM32F4_UART_DRIVER_H__

#include "Inc/drivers/my_stm32f4_gpio_driver.h"

#define ABP1PERIPH_BASE (PERIPH_BASE + 0x00000000U)
#define ABP2PERIPH_BASE (PERIPH_BASE + 0x00010000U)
#define USART1_BASE (ABP2PERIPH_BASE + 0x1000U)
#define USART6_BASE (ABP2PERIPH_BASE + 0x1400U)
#define USART2_BASE (ABP1PERIPH_BASE + 0x4400U)

typedef struct {
    __IO uint32_t SR;   /*!< USART Status register,                   Address offset: 0x00 */
    __IO uint32_t DR;   /*!< USART Data register,                     Address offset: 0x04 */
    __IO uint32_t BRR;  /*!< USART Baud rate register,                Address offset: 0x08 */
    __IO uint32_t CR1;  /*!< USART Control register 1,                Address offset: 0x0C */
    __IO uint32_t CR2;  /*!< USART Control register 2,                Address offset: 0x10 */
    __IO uint32_t CR3;  /*!< USART Control register 3,                Address offset: 0x14 */
    __IO uint32_t GTPR; /*!< USART Guard time and prescaler register, Address offset: 0x18 */
} USART_TypeDef;

#define USART1 ((USART_TypeDef*)USART1_BASE)
#define USART2 ((USART_TypeDef*)USART2_BASE)
#define USART6 ((USART_TypeDef*)USART6_BASE)

#endif /* __MY_STM32F4_UART_DRIVER_H__ */