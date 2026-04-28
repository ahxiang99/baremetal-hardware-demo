#ifndef __MY_STM32F4_SYSTICK_LIB_H__
#define __MY_STM32F4_SYSTICK_LIB_H__

#include <sys/types.h>

#include <cstdint>

#include "Inc/drivers/my_stm32f4_systick_driver.h"

class MySysTick {
   private:
    constexpr static uint32_t SYSTICK_CLK_FREQ = 16000000;  // Assuming 16 MHz clock
    uint32_t                  tickCount;

   public:
    MySysTick(uint32_t seconds);
    void LIB_SET_TICKCOUNT(uint32_t seconds);
};

#endif /* __MY_STM32F4_SYSTICK_LIB_H__ */