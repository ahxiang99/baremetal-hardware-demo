#pragma once
#include "low-level/wwdg_bitfields.h"
#include "low-level/wwdg_registers.h"

class WindowWatchDog {
   public:
    WindowWatchDog() : m_Instance(WWDG_BASE) {}

    void initialize();
    void resetCounter();

   private:
    static constexpr uint8_t CRITICAL_VALUE = 0X40U;
    WWDG_TypeDef*            m_Instance;
    float_t                  t_max;
};