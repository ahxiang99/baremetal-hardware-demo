#ifndef __MY_STM32F4_I2C_LIB_H__
#define __MY_STM32F4_I2C_LIB_H__

#include <cstdint>

#include "low-level/i2c.h"
#include "low-level/i2c_types.h"

#define SHT40AD1B_I2C_ADDRESS 0x89U

// enum class I2C_State { I2C_READY, I2C_BUSY_TX, I2C_BUSY_RX, I2C_ERROR, I2C_INIT };

class II2CMaster {
   public:
    virtual ~II2CMaster()                                                              = default;
    virtual I2C_Status Write(uint8_t target_addr, const uint8_t* pData, uint16_t size) = 0;
    virtual I2C_Status Read(uint8_t target_addr, uint8_t* pBuffer, uint16_t size)      = 0;
    virtual bool       isReady() const                                                 = 0;
    virtual I2C_State  GetState() const                                                = 0;
    virtual void       SetState(I2C_State pState)                                      = 0;
};

class i2c_device : public II2CMaster {
   private:
    I2C_TypeDef*     m_pInstance;
    I2C_InitTypeDef* m_pConfig;
    bool             m_Init;

   public:
    i2c_device(I2C_InitTypeDef* pConfig);
    I2C_Status Write(uint8_t target_addr, const uint8_t* pData, uint16_t size) override;
    I2C_Status Read(uint8_t target_addr, uint8_t* pBuffer, uint16_t size) override;
    bool       isReady() const override;
    I2C_State  GetState() const;
    void       SetState(I2C_State pState);
};

#endif