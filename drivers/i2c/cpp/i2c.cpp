#include "i2c.hpp"

#include "bit_utils.h"
#include "low-level/gpio_bitfields.h"
#include "low-level/i2c.h"
#include "low-level/i2c_bitfields.h"
#include "low-level/i2c_types.h"
#include "low-level/rcc.h"

i2c_device::i2c_device() : m_pInstance(nullptr), m_pConfig(nullptr), m_Init(false) {}

i2c_device::i2c_device(I2C_InitTypeDef* p_Config) {
    InitDriver(p_Config);
}
bool i2c_device::InitDriver(I2C_InitTypeDef* pConfig) {
    if (pConfig == nullptr) return false;
    m_pInstance = I2C_GetBaseAddress(pConfig->i2cx);
    if (m_pInstance == nullptr) return false;
    m_pConfig = pConfig;

    if (I2C_HardwareInit(m_pConfig) != I2C_OK) {
        m_Init = false;
        return false;
    }
    m_Init = true;
    return true;
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
// Check our software state variable
    if (GetState() != I2C_READY) {
        return false;
    }
    
    // HARDWARE GUARD: If the bus is physically still generating a STOP or is BUSY, 
    // tell the application to wait!
    if (READ_BIT(I2C1->SR2, I2C_SR2_BUSY)) {
        return false;
    }
    
    return true;
}

I2C_State i2c_device::GetState() const {
    return I2C_GetState();
}

void i2c_device::SetState(I2C_State pState) {
    I2C_SetState(pState);
}