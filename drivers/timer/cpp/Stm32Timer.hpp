#pragma once

#include "low-level/tim_bitfields.h"
#include "low-level/tim_registers.h"
#include "low-level/tim_types.h"

enum class TimerState : uint8_t { Reset, Stopped, Running, Expired, Error };

struct TimerConfig {
    TIM_Num_t               Instance;
    TIM_CounterMode_t       CounterMode;
    TIM_ClockDivision_t     ClockDivision;
    TIM_AutoReloadPreload_t AutoReloadPreload;
};

class Stm32Timer {
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
    void start(uint32_t time_ms);
    void stop();
    void reset();
    bool isRunning() const;
    bool isElapsed() const;
    void setVariable(TIM_TypeDef* pTim, const TimerConfig& timer_cfg);

    void handleInterrupt();

   private:
    void enablePeripheralClock();
    void configureTimer();
    void clearInterruptFlag();
};