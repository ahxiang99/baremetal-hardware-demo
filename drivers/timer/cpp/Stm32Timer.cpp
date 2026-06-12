#include "Stm32Timer.hpp"

#include "pch.hpp"

void Stm32Timer::start(uint32_t time_ms) {
    m_pTim->ARR = time_ms - 1;
    m_pTim->PSC = 15999;

    reset();
    clearInterruptFlag();
    RegisterUtils::setBits(m_pTim->DIER, TIM_DIER_UIE);
    RegisterUtils::setBits(m_pTim->CR1, TIM_CR1_CEN);
    m_State = TimerState::Running;
}

void Stm32Timer::stop() {
    RegisterUtils::clearBits(m_pTim->CR1, TIM_CR1_CEN);
    m_State = TimerState::Stopped;
}

void Stm32Timer::reset() {
    RegisterUtils::setBits(m_pTim->EGR, TIM_EGR_UG);
    m_State = TimerState::Reset;
}

bool Stm32Timer::isRunning() const {
    return (m_State == TimerState::Running);
}

void Stm32Timer::enablePeripheralClock() {
    if (m_pTim == nullptr) return;
    volatile uint32_t* enrReg = nullptr;
    uint32_t           mask   = 0;
    enrReg                    = tim_table[m_Config.Instance].RCC_APBx;
    mask                      = tim_table[m_Config.Instance].RCC_APBXENR_BIT;
    My_NVIC_EnableIRQ(tim_table[m_Config.Instance].IRQNum);
    RegisterUtils::setBits(*enrReg, mask);
}
void Stm32Timer::configureTimer() {
    uint32_t temp = m_pTim->CR1;
    uint32_t mask = (TIM_CR1_DIR | TIM_CR1_CMS | TIM_CR1_CKD | TIM_CR1_ARPE);
    RegisterUtils::clearBits(temp, mask);

    RegisterUtils::setBits(temp, m_Config.CounterMode);
    RegisterUtils::setBits(temp, m_Config.ClockDivision);
    RegisterUtils::setBits(temp, m_Config.AutoReloadPreload);
    m_pTim->CR1 = temp;
}
void Stm32Timer::clearInterruptFlag() {
    if (m_pTim->SR & TIM_SR_UIF) {
        RegisterUtils::clearBits(m_pTim->SR, TIM_SR_UIF);
    }
}
void Stm32Timer::handleInterrupt() {
    const uint32_t sr = m_pTim->SR;
    if (sr & TIM_SR_UIF) {
        RegisterUtils::clearBits(m_pTim->SR, TIM_SR_UIF);
        m_State = TimerState::Expired;
    }
}
void Stm32Timer::setVariable(TIM_TypeDef* pTim, const TimerConfig& timer_cfg) {
    m_pTim   = pTim;
    m_Config = timer_cfg;
    enablePeripheralClock();
    configureTimer();
}
bool Stm32Timer::isElapsed() const {
    return m_State == TimerState::Expired;
}
