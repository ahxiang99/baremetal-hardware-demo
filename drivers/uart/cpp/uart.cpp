#include "uart.hpp"

#include "drivers/gpio/cpp/gpio.hpp"
#include "low-level/nvic.h"
#include "low-level/rcc.h"
#include "low-level/uart.h"
#include "low-level/uart_types.h"

UARTDevice::UARTDevice(USART_InitTypeDef* p_Config) : m_pInstance(USART_GetBaseAddress(p_Config->device)), m_pConfig(p_Config), m_Init(false), m_LineIdx(0) {
    if (m_pInstance == nullptr || m_pConfig == nullptr) return;
    m_Init = InitDriver(m_pConfig);
}

UARTDevice::UARTDevice() : m_pInstance(nullptr), m_pConfig(nullptr), m_Init(false), m_LineIdx(0) {}

bool UARTDevice::InitDriver(USART_InitTypeDef* p_Config) {
    if (p_Config == nullptr) return false;
    m_pConfig   = p_Config;
    m_pInstance = USART_GetBaseAddress(m_pConfig->device);
    if (SetupPin() == USART_OK && Init_Device() == USART_OK) {
        m_Init = true;
        return true;
    } else {
        m_Init = false;
        return false;
    }
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
        USART_Transmit(m_pInstance, buffer[i]);
    }
    return USART_OK;
}

USART_Status UARTDevice::Get(char* c) {
    if (!m_Init || c == nullptr) return USART_ERR;
    return USART_Receive(m_pInstance, c);
}

const char* UARTDevice::GetLine() {
    return m_Buffer;
}