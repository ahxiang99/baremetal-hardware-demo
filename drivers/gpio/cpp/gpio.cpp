#include "gpio.hpp"

#include <cstddef>

#include "low-level/gpio_types.h"

GPIO::GPIO(GPIO_InitTypeDef* p_Config) : m_pInstance{nullptr}, m_pConfig{p_Config}, m_Init{false} {
    if (InitDriver(m_pConfig) == GPIO_OK) {
        m_Init = true;
    }
}

GPIO_STATUS GPIO::InitDriver(GPIO_InitTypeDef* const p_Config) {
    if (m_pConfig == nullptr) {
        return GPIO_ERR;
    } else {
        if (!m_Init) {
            m_pConfig   = p_Config;
            m_pInstance = GPIO_HardwareInit(m_pConfig->PORT);
            if (GPIO_PinSetConfig(m_pInstance, m_pConfig) == GPIO_ERR) {
                m_Init = false;
                return GPIO_ERR;
            }
        }
        return GPIO_OK;
    }
}

GPIO_STATUS GPIO::ResetDriver() {
    m_Init = false;
    return GPIO_HardwareReset(m_pConfig->PORT);
}

GPIO_STATUS GPIO::SetPinConfig(GPIO_Config* p_Config) {
    if (p_Config == nullptr) return GPIO_ERR;

    if (m_Init) {
        if (GPIO_GetBaseAddress(p_Config->PORT) != m_pInstance) return GPIO_ERR;
        return GPIO_PinSetConfig(m_pInstance, p_Config);
    } else {
        return InitDriver(p_Config);
    }
}

GPIO_STATUS GPIO::TogglePin(const uint16_t PIN) {
    if (!m_Init) return GPIO_ERR;
    return GPIO_ToggleOutputPin(m_pInstance, PIN);
}

bool GPIO::IsInit() {
    return m_Init;
}

GPIO_InitTypeDef gpio_create_config(gpio_port_t _PORT, uint32_t _PIN, gpio_mode_t _MODE, gpio_otyper_t _OTYPE, gpio_ospeedr_t _OSPD, gpio_pupdr_t _PUPD, uint32_t _ALT) {
    GPIO_InitTypeDef temp;
    temp.PORT      = _PORT;
    temp.PIN       = _PIN;
    temp.MODE      = _MODE;
    temp.OTYPE     = _OTYPE;
    temp.SPD       = _OSPD;
    temp.PUPD      = _PUPD;
    temp.Alternate = _ALT;
    return temp;
}