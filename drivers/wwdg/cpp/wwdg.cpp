#include "cpp/wwdg.hpp"

#include <cmath>

#include "RegisterUtils.hpp"
#include "drivers.hpp"
#include "low-level/rcc_bitfields.h"
#include "low-level/wwdg_bitfields.h"
#include "pch.hpp"

void WindowWatchDog::initialize() {
    /*Enable WatchDog Timer at RCC*/
    RegisterUtils::setBits(RCC->APB1ENR, RCC_APB1ENR_WWDG_EN);

    /*Set Timer Base and Windows Value */
    uint8_t prescaler = 3U;
    m_Instance->CFR   = (prescaler << WWDG_CFR_WDGTB_Pos) | (0x7FU << WWDG_CFR_W_Pos);

    /*Enable WatchDog*/
    m_Instance->CR = WWDG_CR_WDGA | 0x7FU;
}
void WindowWatchDog::resetCounter() {
    /* To avoid reset, reload counter register when its value is lower than the windows register value and greater than 0x3F */
    m_Instance->CR = WWDG_CR_WDGA | 0x7FU;
}
