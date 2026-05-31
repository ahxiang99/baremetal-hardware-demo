#pragma once

#include <cstdint>

#include "low-level/tim_types.h"

enum class TimerState { Reset, Stopped, Running, Expired, Error };

struct TimerConfig {
    TIM_Num_t               Instance;
    TIM_CounterMode_t       CounterMode;
    TIM_ClockDivision_t     ClockDivision;
    TIM_AutoReloadPreload_t AutoReloadPreload;
};

class ITimer {
   public:
    virtual ~ITimer()                    = default;
    virtual void start(uint32_t time_ms) = 0;
    virtual void stop()                  = 0;
    virtual void reset()                 = 0;
    virtual bool isRunning() const       = 0;
    virtual bool isElapsed() const       = 0;
};