#ifndef __MY_STM32F4_I2C_LIB_H__
#define __MY_STM32F4_I2C_LIB_H__

#include <cstdint>

#include "Inc/drivers/my_stm32f4_i2c_driver.h"

#define SHT40AD1B_I2C_ADDRESS 0x89U

#define I2C_DUTYCYCLE_2 0U
#define I2C_ENABLE 0X1U

#define I2C_SR1_SB (1 << 0)
#define I2C_SR1_ADDR (1 << 1)
#define I2C_SR1_BTF (1 << 2)
#define I2C_SR1_RXNE (1 << 6)
#define I2C_SR1_TXE (1 << 7)
#define I2C_SR1_AF (1 << 10)

#define I2C_SR2_BUSY (1 << 1)

#define I2C_CR1_START (1 << 8)
#define I2C_CR1_STOP (1 << 9)
#define I2C_CR1_ACK (1 << 10)

#define RCC_APB1ENR_I2C1EN (1 << 21)

#define I2C_OK 1
#define I2C_ERROR -1

typedef struct {
    uint32_t SCL_ClkFreq;
    uint32_t DutyCycle;
    uint32_t OwnAddress1;
    uint32_t AddressingMode;
    uint32_t DualAddressMode;
    uint32_t OwnAddress2;

} I2C_InitTypeDef;

class I2C {
   private:
    I2C_TypeDef*     I2Cx;
    I2C_InitTypeDef* I2C_initStruct;

    uint32_t         GetSysClockFreq();
    uint32_t         GetCCR();

   public:
    I2C(I2C_TypeDef* _i2c, I2C_InitTypeDef* _init);
    int32_t LIB_I2C_TRANSMIT(uint32_t dev_addr, uint8_t* cmd, uint16_t cmd_size);
    int32_t LIB_I2C_READ_REGISTER(uint32_t dev_addr, uint8_t* pData, uint16_t size);
};

#endif