#ifndef I2C_REGISTERS_H
#define I2C_REGISTERS_H

#include "low-level/rcc_registers.h"

#define I2C1_BASE (APB1PERIPH_BASE + 0x5400U)
#define I2C2_BASE (APB1PERIPH_BASE + 0x5800U)
#define I2C3_BASE (APB1PERIPH_BASE + 0x5C00U)

#define I2C1 ((I2C_TypeDef*)I2C1_BASE)
#define I2C2 ((I2C_TypeDef*)I2C2_BASE)
#define I2C3 ((I2C_TypeDef*)I2C3_BASE)

#define __IO volatile

typedef struct {
    __IO uint32_t CR1;   /* I2C Control Register 1, Address Offset: 0x00 */
    __IO uint32_t CR2;   /* I2C Control Register 2, Address Offset: 0x04 */
    __IO uint32_t OAR1;  /* I2C OAR1, Address Offset: 0x08 */
    __IO uint32_t OAR2;  /* I2C OAR2, Address Offset: 0x0C */
    __IO uint32_t DR;    /* I2C Data Register, Address Offset: 0x10 */
    __IO uint32_t SR1;   /* I2C Status Register 1, Address Offset: 0x14 */
    __IO uint32_t SR2;   /* I2C Status Register 2, Address Offset: 0x18 */
    __IO uint32_t CCR;   /* I2C Clock Control Register, Address Offset: 0x1C */
    __IO uint32_t TRISE; /* I2C Maximum Time Rise in Fm / Sm Mode (Controller Mode), Address Offset: 0x20 */
    __IO uint32_t FLTR;  /* I2C Noise Filter, Address Offset: 0x24 */
} I2C_TypeDef;

#endif