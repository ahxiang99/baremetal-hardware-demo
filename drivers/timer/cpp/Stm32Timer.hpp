#pragma once

#include "ITimer.hpp"
#include "low-level/tim.h"

class Stm32Timer : public ITimer {
   private:
    TIM_TypeDef* m_pTim;
    TimerConfig  m_Config;
    TimerState   m_State;

   public:
    Stm32Timer() {}
    Stm32Timer(TIM_TypeDef* pTim, const TimerConfig& timer_cfg) : m_pTim(pTim), m_Config(timer_cfg), m_State(TimerState::Reset) {
        enablePeripheralClock();
        configureTimer();
    }
    void start(uint32_t time_ms) override;
    void stop() override;
    void reset() override;
    bool isRunning() const override;
    bool isElapsed() const override;
    void setVariable(TIM_TypeDef* pTim, const TimerConfig& timer_cfg);

    void handleInterrupt();

   private:
    void enablePeripheralClock();
    void configureTimer();
    void clearInterruptFlag();
};