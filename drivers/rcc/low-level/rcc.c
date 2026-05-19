#include "rcc.h"

#include "bit_utils.h"
#include "low-level/gpio_types.h"
#include "low-level/rcc_bitfields.h"
#include "stdint.h"

status_t Init48MHzForUSB(void) {
    // 1. Enable HSE and PLL
    uint32_t temp = RCC->CR;
    CLEAR_BIT(temp, RCC_CR_HSEON);
    SET_BIT(temp, RCC_CR_HSEON);
    RCC->CR = temp;

    while (!(RCC->CR & RCC_CR_HSERDY));  // Wait until HSE stabilize

    temp = RCC->CR;
    CLEAR_BIT(temp, RCC_CR_PLLON);
    SET_BIT(temp, RCC_CR_PLLON);
    RCC->CR = temp;
    while (!(RCC->CR & RCC_CR_PLLRDY));  // Wait until PLL stabilize

    // PLLM = 15 , PLLN = 144, PLLQ = 5

    temp = RCC->PLLCFGR;
    temp |= (15 << RCC_CFGR_PLLM_Pos);
    temp |= (144 << RCC_CFGR_PLLN_Pos);
    temp |= (5 << RCC_CFGR_PLLQ_Pos);
    RCC->PLLCFGR = temp;

    return STATUS_OK;
}