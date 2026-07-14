#include "Stm32Timer.hpp"

#include "RegisterUtils.hpp"
#include "Result.hpp"
#include "drivers.hpp"
#include "low-level/nvic.h"
#include "low-level/tim_bitfields.h"
#include "pch.hpp"

typedef struct {
    TIM_TypeDef*       instance;
    volatile uint32_t* RCC_APBx;
    uint32_t           RCC_APBXENR_BIT;
    uint32_t           RCC_APBXRSTR_BIT;
    IRQn_Type          IRQNum;
} tim_info_t;

namespace {
static const tim_info_t tim_table[static_cast<uint8_t>(TimerDevice_t::TIMER_COUNT)] = {
    {TIM1,  &(RCC->APB2ENR), RCC_APB2ENR_TIM1_EN,  RCC_APB2RSTR_TIM1_RST,  TIM1_UP_TIM10_IRQn     },
    {TIM2,  &(RCC->APB1ENR), RCC_APB1ENR_TIM2_EN,  RCC_APB1RSTR_TIM2_RST,  TIM2_IRQn              },
    {TIM3,  &(RCC->APB1ENR), RCC_APB1ENR_TIM3_EN,  RCC_APB1RSTR_TIM3_RST,  TIM3_IRQn              },
    {TIM4,  &(RCC->APB1ENR), RCC_APB1ENR_TIM4_EN,  RCC_APB1RSTR_TIM4_RST,  TIM4_IRQn              },
    {TIM5,  &(RCC->APB1ENR), RCC_APB1ENR_TIM5_EN,  RCC_APB1RSTR_TIM5_RST,  TIM5_IRQn              },
    {TIM9,  &(RCC->APB2ENR), RCC_APB2ENR_TIM9_EN,  RCC_APB2RSTR_TIM9_RST,  TIM1_BRK_TIM9_IRQn     },
    {TIM10, &(RCC->APB2ENR), RCC_APB2ENR_TIM10_EN, RCC_APB2RSTR_TIM10_RST, TIM1_UP_TIM10_IRQn     },
    {TIM11, &(RCC->APB2ENR), RCC_APB2ENR_TIM11_EN, RCC_APB2RSTR_TIM11_RST, TIM1_TRG_COM_TIM11_IRQn},
};

TIM_TypeDef* enableAndGet(TimerDevice_t dev) {
    const auto& entry = tim_table[static_cast<uint8_t>(dev)];
    RegisterUtils::setBits(*entry.RCC_APBx, entry.RCC_APBXENR_BIT);
    My_NVIC_EnableIRQ(entry.IRQNum);
    return entry.instance;
}

}  // namespace

Result<> Stm32Timer::initialize(const TimerConfig& cfg) {
    timer_instance_ = enableAndGet(cfg.Instance);
    if (timer_instance_ == nullptr) return Fail(Err::NullInstance);
    timer_instance_->PSC = getDrivers().sysclock.getSysClock().apb1 / 1000 - 1;
    state_               = TimerState_t::Reset;
    configureTimer(cfg);
    return Ok();
}

void Stm32Timer::start(uint32_t time_ms) {
    if (time_ms == 0) {
        return;
    }

    timer_instance_->ARR = 2 * time_ms - 1;
    reset();
    clearInterruptFlag();
    RegisterUtils::setBits(timer_instance_->DIER, TIM_DIER_UIE);
    RegisterUtils::setBits(timer_instance_->CR1, TIM_CR1_CEN);
    state_ = TimerState_t::Running;
}

void Stm32Timer::stop() {
    RegisterUtils::clearBits(timer_instance_->CR1, TIM_CR1_CEN);
    state_ = TimerState_t::Stopped;
}

void Stm32Timer::reset() {
    RegisterUtils::setBits(timer_instance_->EGR, TIM_EGR_UG);
    state_ = TimerState_t::Reset;
}

bool Stm32Timer::isRunning() const {
    return (state_ == TimerState_t::Running);
}

void Stm32Timer::configureTimer(const TimerConfig& cfg) {
    uint32_t temp = timer_instance_->CR1;

    // CMS
    if (cfg.AlignedMode != TimerCenterAlignedMode_t::EDGE) RegisterUtils::modify(temp, TIM_CR1_CMS_Msk, static_cast<uint8_t>(cfg.AlignedMode) << TIM_CR1_CMS_Pos);

    // Up or Down Counter
    if (cfg.Direction == TimerDirection_t::DOWN) RegisterUtils::setBits(temp, TIM_CR1_DIR);

    // ARR
    if (cfg.AutoReloadPreload == TimerARR_t::ENABLE) RegisterUtils::setBits(temp, TIM_CR1_ARPE);

    // Clock Division
    RegisterUtils::modify(temp, TIM_CR1_CKD_Msk, static_cast<uint8_t>(cfg.ClockDivision) << TIM_CR1_CKD_Pos);

    timer_instance_->CR1 = temp;
}

void Stm32Timer::clearInterruptFlag() {
    if (timer_instance_->SR & TIM_SR_UIF) {
        RegisterUtils::clearBits(timer_instance_->SR, TIM_SR_UIF);
    }
}

void Stm32Timer::handleInterrupt() {
    const uint32_t sr = timer_instance_->SR;
    if (sr & TIM_SR_UIF) {
        RegisterUtils::clearBits(timer_instance_->SR, TIM_SR_UIF);
        state_ = TimerState_t::Expired;
    }
}

bool Stm32Timer::isElapsed() const {
    return state_ == TimerState_t::Expired;
}