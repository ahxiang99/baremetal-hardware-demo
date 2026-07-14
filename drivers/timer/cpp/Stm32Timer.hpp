#pragma once

#include "low-level/tim_bitfields.h"
#include "low-level/tim_registers.h"

enum class TimerState_t : uint8_t { Reset, Stopped, Running, Expired, Error };
enum class TimerDevice_t : uint8_t { TIMER_1, IMER_2, TIMER_3, TIMER_4, TIMER_5, TIMER_9, TIMER_10, TIMER_11, TIMER_COUNT };
enum class TimerCenterAlignedMode_t : uint8_t { EDGE, CENTER_MODE_1, CENTER_MODE_2, CENTER_MODE_3 };
enum class TimerDirection_t : uint8_t { UP, DOWN };
enum class TimerClockDivision_t : uint16_t { TIM_CLOCKDIVISION_DIV1 = (0 << 8), TIM_CLOCKDIVISION_DIV2 = (1 << 8), TIM_CLOCKDIVISION_DIV4 = (2 << 8) };
enum class TimerARR_t : uint8_t { DISABLE, ENABLE };

struct TimerConfig {
    TimerDevice_t            Instance;
    TimerCenterAlignedMode_t AlignedMode;
    TimerDirection_t         Direction;
    TimerClockDivision_t     ClockDivision;
    TimerARR_t               AutoReloadPreload;
};

class Stm32Timer {
   private:
    TIM_TypeDef* timer_instance_;
    TimerState_t state_;

   public:
    Stm32Timer() = default;
    Result<> initialize(const TimerConfig& cfg);
    void     start(uint32_t time_ms);
    void     stop();
    void     reset();
    bool     isRunning() const;
    bool     isElapsed() const;
    void     handleInterrupt();

   private:
    void configureTimer(const TimerConfig& cfg);
    void clearInterruptFlag();
};