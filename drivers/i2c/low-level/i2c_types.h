#ifndef I2C_TYPES_H
#define I2C_TYPES_H

#include <stdint.h>

#include "low-level/i2c_registers.h"

#define I2C_CR2_16MHz (1 << 4)

typedef enum { I2C_ERR = -1, I2C_OK = 1, I2C_BUSY } I2C_Status;
typedef enum {
    I2C_NOT_READY,
    I2C_READY,
    I2C_BUSY_TX,
    I2C_BUSY_RX,
    I2C_ERROR_NACK,  // Sensor ignored us
    I2C_ERROR_BUS,   // Physical signal issue
    I2C_ERROR_ARLO,  // Multi-master collision
    I2C_ERROR_OVR    // Timing issue
} I2C_State;
typedef enum { I2C_1, I2C_2, I2C_3 } I2C_Num_t;
typedef enum { I2C_SPEED_STANDARD = 100000, I2C_SPEED_FAST = 400000 } I2C_Freq_t;

typedef struct {
    I2C_TypeDef*       pInstance;
    uint8_t*           pBuffer;
    uint8_t            cmdBuffer;
    uint16_t           Size;
    uint16_t           Count;
    volatile I2C_State State;
} I2C_Handle_t;

typedef struct {
    I2C_Num_t  i2cx;
    I2C_Freq_t I2C_FREQ;
    uint32_t   OwnAddress1;
    uint32_t   AddressingMode;
    uint32_t   DualAddressMode;
    uint32_t   OwnAddress2;
} I2C_InitTypeDef;

#endif