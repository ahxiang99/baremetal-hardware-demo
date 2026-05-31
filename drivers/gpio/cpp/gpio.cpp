#include "gpio.hpp"

#include <cstddef>

#include "low-level/gpio_types.h"


GPIO::GPIO(GPIO_InitTypeDef* p_Config) : m_pInstance{nullptr}, m_pConfig{p_Config}, m_Init{false} {
    if (InitDriver(m_pConfig) == STATUS_OK) {
        m_Init = true;
    }
}
GPIO::GPIO() : m_pInstance{nullptr}, m_pConfig{nullptr}, m_Init{false} {}

Result GPIO::InitDriver(GPIO_InitTypeDef* const p_Config) {
    if (p_Config == nullptr) {
        return ERR_NULLPTR;
    } else {
        if (!m_Init) {
            m_pConfig = p_Config;
            if ((GPIO_HardwareInit(m_pConfig->PORT, &m_pInstance) == STATUS_OK)) {
                if (GPIO_PinSetConfig(m_pInstance, m_pConfig) == STATUS_OK) {
                    m_Init = true;
                }
            } else {
                m_Init = false;
                return ERR_INIT;
            }
        }
        return STATUS_OK;
    }
}

Result GPIO::ResetDriver() {
    m_Init = false;
    return GPIO_HardwareReset(m_pConfig->PORT);
}

Result GPIO::SetPinConfig(GPIO_InitTypeDef* p_Config) {
    if (p_Config == nullptr) return ERR_NULLPTR;

    if (m_Init) {
        // if (GPIO_HardwareInit(p_Config->PORT) != m_pInstance) return GPIO_ERR;
        return GPIO_PinSetConfig(m_pInstance, p_Config);
    } else {
        return InitDriver(p_Config);
    }
}

Result GPIO::TogglePin(const uint16_t PIN) {
    if (!m_Init) return ERR_NULLPTR;
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