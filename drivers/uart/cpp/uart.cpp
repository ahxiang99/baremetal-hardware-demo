#include "uart.hpp"

#include "cpp/gpio.hpp"
#include "low-level/nvic.h"
#include "low-level/rcc.h"
#include "low-level/uart.h"
#include "low-level/uart_types.h"

UARTDevice::UARTDevice(USART_InitTypeDef* p_Config) : m_pInstance(USART_GetBaseAddress(p_Config->device)), m_pConfig(p_Config), m_Init(false), m_LineIdx(0) {
    if (m_pInstance == nullptr || m_pConfig == nullptr) return;

    if (SetupPin() != USART_OK) return;

    if (Init_Device() != USART_OK) return;

    m_Init = true;
}

USART_Status UARTDevice::SetupPin() {
    // 1. Initialize the GPIO Pin for respective USARTx.
    GPIO_InitTypeDef gpio_cfg;
    switch (m_pConfig->device) {
        case USART_D1:
            gpio_cfg = gpio_create_config(GPIO_PA, GPIO_PIN_9 | GPIO_PIN_10, GPIO_MODE_ALTFN, GPIO_OTYPER_PP, GPIO_OSPEEDR_LS, GPIO_PUPDR_NOPULL, 0x07);
            break;
        case USART_D2:
            gpio_cfg = gpio_create_config(GPIO_PA, GPIO_PIN_2 | GPIO_PIN_3, GPIO_MODE_ALTFN, GPIO_OTYPER_PP, GPIO_OSPEEDR_LS, GPIO_PUPDR_PULLUP, 0x07);
            break;
        case USART_D6:
            gpio_cfg = gpio_create_config(GPIO_PA, GPIO_PIN_11 | GPIO_PIN_12, GPIO_MODE_ALTFN, GPIO_OTYPER_PP, GPIO_OSPEEDR_LS, GPIO_PUPDR_NOPULL, 0x07);
            break;
    }

    // 2. Use GPIO class to setup the USART Pin.
    GPIO USART_GPIO(&gpio_cfg);
    if (USART_GPIO.IsInit())
        return USART_OK;
    else
        return USART_ERR;
}

USART_Status UARTDevice::Init_Device() {
    return USART_HardwareInit(m_pConfig);
}

USART_Status UARTDevice::Print(const char* buffer, uint16_t max_size) {
    if (!m_Init || buffer == nullptr || max_size <= 0) return USART_ERR;

    for (uint16_t i = 0; i < max_size; ++i) {
        USART_SendByte(m_pInstance, buffer[i]);
    }
    return USART_OK;
}

USART_Status UARTDevice::Get(char* c) {
    if (!m_Init || c == nullptr) return USART_ERR;
    return USART_ReceiveByte(m_pInstance, c);
}

const char* UARTDevice::GetLine() {
    return m_Buffer;
}

bool UARTDevice::HandleInput() {
    char c;
    while (DataAvailable()) {
        // --- START CRITICAL SECTION ---
        My_NVIC_DisableIRQ(38);
        bool dataReceived = (USART_ReadRxBuffer(&c) == USART_OK);
        My_NVIC_EnableIRQ(38);
        // --- END CRITICAL SECTION ---

        if (!dataReceived) return false;

        switch (c) {
            case '\r':  // Carriage Return (Enter)
            case '\n':
                if (m_LineIdx > 0) {             // Only process if something was typed
                    m_Buffer[m_LineIdx] = '\0';  // Null terminate
                    m_LineIdx           = 0;     // Reset for next time
                    Print("\r\n", 2);            // Echo newline to user
                    return true;
                }
                Print("\r\nNode > ", 9);  // Just a fresh line if empty
                break;
            case '\b':  // Backspace (ASCII 8)
            case 0x7F:  // Delete (often sent as backspace by some terminals)
                if (m_LineIdx > 0) {
                    m_LineIdx--;
                    Print("\b \b", 3);  // Move back, overwrite with space, move back again
                }
                break;

            default:
                // Only add printable characters and check for buffer overflow
                if (c >= 32 && c <= 126) {
                    if (m_LineIdx < (UART_BUF_SIZE - 1)) {
                        m_Buffer[m_LineIdx++] = (char)c;
                        Print(&c, 1);  // Echo character so the user sees what they type
                    }
                }
                break;
        }
    }
    return false;
}

USART_Status UARTDevice::DataAvailable() {
    return USART_Data_Available();
}
