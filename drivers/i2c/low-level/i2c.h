#ifndef I2C_DRIVER_H
#define I2C_DRIVER_H

#include "i2c_bitfields.h"
#include "i2c_registers.h"
#include "i2c_types.h"
#include "low-level/i2c.h"
#include "low-level/i2c_types.h"
#include "low-level/uart.h"
#include "low-level/uart_registers.h"

#ifdef __cplusplus
extern "C" {
#endif

I2C_TypeDef* I2C_GetBaseAddress(I2C_Num_t p_I2Cx);
I2C_Status   I2C_HardwareInit(I2C_InitTypeDef* p_Config);
I2C_Status   I2C_SendCommand(I2C_TypeDef* pInstance, uint8_t target_addr, uint8_t* pCmd, uint16_t pCmdSize);
I2C_Status   I2C_ReceiveByte(I2C_TypeDef* pInstance, uint8_t target_addr, uint8_t* pData, uint16_t pDataSize);
I2C_Status   I2C_Print(USART_TypeDef* pInstance, const char* buffer, uint16_t max_size);
I2C_State    I2C_GetState();
I2C_Status   I2C_SetState(I2C_State pState);

#ifdef __cplusplus
}
#endif

#endif