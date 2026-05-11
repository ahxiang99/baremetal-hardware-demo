#ifndef BIT_UTILS_H
#define BIT_UTILS_H

#define SET_BIT(REG, BIT) ((REG) |= (BIT))

#define CLEAR_BIT(REG, BIT) ((REG) &= ~(BIT))

#define TOGGLE_BIT(REG, BIT) ((REG) ^= (BIT))

#define READ_BIT(REG, BIT) ((REG) & (BIT))

#define MODIFY_REG(REG, CLEARMASK, SETMASK) ((REG) = (((REG) & (~(CLEARMASK))) | (SETMASK)))

#endif