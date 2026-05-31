#ifndef I2C_BITFIELDS_H
#define I2C_BITFIELDS_H

#define I2C_ENABLE 0X1U

#define I2C_SR1_SB (1 << 0)
#define I2C_SR1_ADDR (1 << 1)
#define I2C_SR1_BTF (1 << 2)
#define I2C_SR1_ADD10 (1 << 3)
#define I2C_SR1_STOPF (1 << 4)
#define I2C_SR1_RXNE (1 << 6)
#define I2C_SR1_TXE (1 << 7)
#define I2C_SR1_BERR (1 << 8)
#define I2C_SR1_ARLO (1 << 9)
#define I2C_SR1_AF (1 << 10)
#define I2C_SR1_OVR (1 << 11)
#define I2C_SR1_PECERR (1 << 12)
#define I2C_SR1_TIMEOUT (1 << 14)
#define I2C_SR1_SMBALERT (1 << 15)

#define I2C_SR2_MSL (1 << 0)
#define I2C_SR2_BUSY (1 << 1)
#define I2C_SR2_TRA (1 << 2)
#define I2C_SR2_GENCALL (1 << 4)
#define I2C_SR2_SMBDEFAULT (1 << 5)
#define I2C_SR2_SMBHOST (1 << 6)
#define I2C_SR2_DUALF (1 << 7)

#define I2C_CR1_PE (1 << 0)
#define I2C_CR1_SMBUS (1 << 1)
#define I2C_CR1_SMBTYPE (1 << 3)
#define I2C_CR1_ENARP (1 << 4)
#define I2C_CR1_ENPEC (1 << 5)
#define I2C_CR1_ENGC (1 << 6)
#define I2C_CR1_NOSTRETCH (1 << 7)
#define I2C_CR1_START (1 << 8)
#define I2C_CR1_STOP (1 << 9)
#define I2C_CR1_ACK (1 << 10)
#define I2C_CR1_POS (1 << 11)
#define I2C_CR1_PEC (1 << 12)
#define I2C_CR1_ALERT (1 << 13)
#define I2C_CR1_SWRST (1 << 15)

#define I2C_CR2_ITERREN (1 << 8)
#define I2C_CR2_ITEVTEN (1 << 9)
#define I2C_CR2_ITBUFEN (1 << 10)
#define I2C_CR2_DMAEN (1 << 11)
#define I2C_CR2_LAST (1 << 12)

#define I2C_OAR1_ADDMODE (1 << 15)

#endif