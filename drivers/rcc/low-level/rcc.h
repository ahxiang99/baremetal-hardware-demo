#ifndef RCC_H
#define RCC_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include "low-level/gpio_types.h"
#include "rcc_bitfields.h"
#include "rcc_registers.h"

void Init48MHzForUSB(void);
#ifdef __cplusplus
}
#endif
#endif