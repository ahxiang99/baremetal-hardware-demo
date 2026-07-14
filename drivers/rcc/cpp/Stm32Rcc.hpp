#pragma once

#include "Register.hpp"
#include "low-level/flash_bitfields.h"
#include "low-level/flash_registers.h"
#include "low-level/rcc_registers.h"

struct PllConfig {
    uint32_t hsi_hz;  // input clock — 16,000,000
    uint32_t pll_m;   // 2–63
    uint32_t pll_n;   // 50–432
    uint32_t pll_p;   // 2, 4, 6, or 8 only
};

constexpr uint32_t calcSysClock(const PllConfig& cfg) {
    return (cfg.hsi_hz / cfg.pll_m) * cfg.pll_n / cfg.pll_p;
}

constexpr bool isValidPllConfig(const PllConfig& cfg) {
    bool validrange_pllm  = (cfg.pll_m >= 2 && cfg.pll_m <= 63);
    bool validrange_plln  = (cfg.pll_n >= 50 && cfg.pll_n <= 432);
    bool validrange_pllp  = (cfg.pll_p % 2 == 0 && (cfg.pll_p >= 2 && cfg.pll_p <= 8));

    bool valid_VCO_input  = ((cfg.hsi_hz / cfg.pll_m) >= 1'000'000 && (cfg.hsi_hz / cfg.pll_m) <= 2'000'000);
    bool valid_VCO_output = ((cfg.hsi_hz / cfg.pll_m * cfg.pll_n) >= 100'000'000 && (cfg.hsi_hz / cfg.pll_m * cfg.pll_n) <= 432'000'000);

    return validrange_pllm && validrange_plln && validrange_pllp && valid_VCO_input && valid_VCO_output;
}

struct ClockTree {
    uint32_t sysclk;  // 84MHz
    uint32_t ahb;     // sysclk / AHB_prescaler (default 1) = 84MHz
    uint32_t apb1;    // ahb / APB1_prescaler (max 42MHz on F401)
    uint32_t apb2;    // ahb / APB2_prescaler (max 84MHz on F401)
};

constexpr ClockTree calcClockTree(const PllConfig& cfg, uint32_t ahb_div = 1, uint32_t apb1_div = 2, uint32_t apb2_div = 1) {
    uint32_t sysclock = calcSysClock(cfg);
    uint32_t ahbclk   = sysclock / ahb_div;
    return {sysclock, ahbclk, ahbclk / apb1_div, ahbclk / apb2_div};
}

// Valid config — your 84MHz values:
constexpr PllConfig cfg_84mhz = {16'000'000, 8, 84, 2};
static_assert(isValidPllConfig(cfg_84mhz), "84MHz config invalid");
static_assert(calcSysClock(cfg_84mhz) == 84'000'000, "84MHz calculation wrong");

constexpr ClockTree clocks = calcClockTree(cfg_84mhz);

// Validate PLL config:
static_assert(isValidPllConfig(cfg_84mhz), "PLL config invalid");
static_assert(calcSysClock(cfg_84mhz) == 84'000'000, "SYSCLK wrong");

// Validate peripheral clocks:
static_assert(clocks.sysclk == 84'000'000, "SYSCLK must be 84MHz");
static_assert(clocks.apb1 <= 42'000'000, "APB1 overclock!");
static_assert(clocks.apb2 <= 84'000'000, "APB2 overclock!");

// Prove bad config is rejected:
constexpr PllConfig cfg_bad = {16'000'000, 1, 84, 2};
static_assert(!isValidPllConfig(cfg_bad), "bad config should be rejected");

enum class SysClockSource : uint8_t { HSI, HSE, PLL, COUNT };

enum class AHB_ClockDivision : uint8_t { DIV_1, DIV_2 = (1 << 3), DIV_4, DIV_8, DIV_16, DIV_64, DIV_128, DIV_256, DIV_512 };

enum class APB_ClockDivision : uint8_t { DIV_1, DIV_2 = (1 << 2), DIV_4, DIV_8, DIV_16 };

struct SysClockConfig {
    SysClockSource    Src{SysClockSource::HSI};
    PllConfig         PllCfg;
    AHB_ClockDivision ahb_div;
    APB_ClockDivision apb1_div;
    APB_ClockDivision apb2_div;
    uint32_t          flashLatency;
};

constexpr ClockTree calcClockTree_v2(const SysClockConfig& config) {
    uint16_t ahb_div  = 0;
    uint16_t apb1_div = 0;
    uint16_t apb2_div = 0;
    switch (config.ahb_div) {
        case AHB_ClockDivision::DIV_1:
            ahb_div = 1;
            break;
        case AHB_ClockDivision::DIV_2:
            ahb_div = 2;
            break;
        case AHB_ClockDivision::DIV_4:
            ahb_div = 4;
            break;
        case AHB_ClockDivision::DIV_8:
            ahb_div = 8;
            break;
        case AHB_ClockDivision::DIV_16:
            ahb_div = 16;
            break;
        case AHB_ClockDivision::DIV_64:
            ahb_div = 64;
            break;
        case AHB_ClockDivision::DIV_128:
            ahb_div = 128;
            break;
        case AHB_ClockDivision::DIV_256:
            ahb_div = 256;
            break;
        case AHB_ClockDivision::DIV_512:
            ahb_div = 512;
            break;
    }

    switch (config.apb1_div) {
        case APB_ClockDivision::DIV_1:
            apb1_div = 1;
            break;
        case APB_ClockDivision::DIV_2:
            apb1_div = 2;
            break;
        case APB_ClockDivision::DIV_4:
            apb1_div = 4;
            break;
        case APB_ClockDivision::DIV_8:
            apb1_div = 8;
            break;
        case APB_ClockDivision::DIV_16:
            apb1_div = 16;
            break;
    }

    switch (config.apb2_div) {
        case APB_ClockDivision::DIV_1:
            apb2_div = 1;
            break;
        case APB_ClockDivision::DIV_2:
            apb2_div = 2;
            break;
        case APB_ClockDivision::DIV_4:
            apb2_div = 4;
            break;
        case APB_ClockDivision::DIV_8:
            apb2_div = 8;
            break;
        case APB_ClockDivision::DIV_16:
            apb2_div = 16;
            break;
    }

    return calcClockTree(config.PllCfg, ahb_div, apb1_div, apb2_div);
}

class SysClock {
   public:
    SysClock();
    void      initialize(const SysClockConfig& config, const ClockTree& clock);
    void      configurePrescaler();
    ClockTree getSysClock();

   private:
    RCC_TypeDef*   m_Instance;
    Flash_TypeDef* m_Flash;
    SysClockConfig m_Config;
    ClockTree      m_SysClock;
};