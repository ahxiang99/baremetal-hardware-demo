#pragma once

#include "Register.hpp"

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

class RCC_Regs {
    static constexpr uint32_t AHB_Addr = PERIPH_BASE + 0x00020000U;
    using AHB_Control                  = Register<AHB_Addr, uint32_t>;
};