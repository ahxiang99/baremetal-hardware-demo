#include "i2c.hpp"

#include "bit_utils.h"
#include "low-level/gpio_bitfields.h"
#include "low-level/i2c.h"
#include "low-level/i2c_bitfields.h"
#include "low-level/i2c_types.h"
#include "low-level/rcc.h"

i2c_device::i2c_device() : m_pInstance(nullptr), m_pConfig(nullptr), m_Init(false) {}

i2c_device::i2c_device(I2C_InitTypeDef* p_Config) : m_pInstance(I2C_GetBaseAddress(p_Config->i2cx)), m_pConfig(p_Config) {
    if (I2C_HardwareInit(p_Config) != I2C_OK) {
        m_Init = false;
    }
    m_Init = true;
}

I2C_Status i2c_device::Write(uint8_t target_addr, const uint8_t* pCmd, uint16_t size) {
    if (!m_Init) return I2C_ERR;
    return I2C_SendCommand(m_pInstance, target_addr, const_cast<uint8_t*>(pCmd), size);
}

I2C_Status i2c_device::Read(uint8_t target_addr, uint8_t* pBuffer, uint16_t size) {
    if (!m_Init) return I2C_ERR;
    return I2C_ReceiveByte(m_pInstance, target_addr, pBuffer, size);
}

bool i2c_device::isReady() const {
    bool HardwareIdle = !(READ_BIT(m_pInstance->SR2, I2C_SR2_BUSY));
    bool SoftwareIdle = (I2C_GetState() == I2C_READY);
    return (HardwareIdle && SoftwareIdle);
}

I2C_State i2c_device::GetState() const {
    return I2C_GetState();
}

void i2c_device::SetState(I2C_State pState) {
    I2C_SetState(pState);
}