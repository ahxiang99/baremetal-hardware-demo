#ifndef SYSTICK_H
#define SYSTICK_H

#include <stdint.h>

#define SCB_BASE 0xE000E000U
#define SYSTICK_OFFSET 0x0010U
#define SYSTICK_BASE (SCB_BASE + SYSTICK_OFFSET)

#define __IOM volatile  // Read Write Structure member permissions
#define __OM volatile   // Write Only Structure member permissions
#define __IM volatile   // Read Only Structure member permissions

typedef struct {
    __IOM uint32_t CTRL;  /*!< SysTick Control and Status Register, Address offset: 0x00 */
    __IOM uint32_t LOAD;  /*!< SysTick Reload Value Register,       Address offset: 0x04 */
    __IOM uint32_t VAL;   /*!< SysTick Current Value Register,      Address offset: 0x08 */
    __IOM uint32_t CALIB; /*!< SysTick Calibration Value Register,  Address offset: 0x0C */
} SysTick_TypeDef;

#define SysTickPtr ((SysTick_TypeDef*)SYSTICK_BASE)

#endif /* SYSTICK_H */