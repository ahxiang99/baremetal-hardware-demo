#pragma once

#include <chrono>
#include <ctime>

#include "low-level/pwr_registers.h"
#include "low-level/rcc_registers.h"
#include "low-level/rtc_bitfields.h"
#include "low-level/rtc_registers.h"

struct RTC_DateTypeDef {
    uint8_t WeekDay;
    uint8_t Year;
    uint8_t Month;
    uint8_t Day;
};

struct RTC_TimeTypeDef {
    uint8_t Hours;
    uint8_t Minutes;
    uint8_t Seconds;
    uint8_t TimeFormat;
};

struct BcdDigits_t {
    uint8_t tens;
    uint8_t ones;
};

struct TimeStamp_t {
    RTC_DateTypeDef day;
    RTC_TimeTypeDef time;
};

class Stm32Rtc {
   public:
    Stm32Rtc();
    Result<>        initialize();
    void            setTime(const RTC_TimeTypeDef& t);
    void            setDate(const RTC_DateTypeDef& d);

    RTC_DateTypeDef getDate();
    RTC_TimeTypeDef getTime();
    TimeStamp_t     getTimeStamp();

    bool            IsInit() const;

   protected:
    RTC_TimeTypeDef getTimeStampTime();
    RTC_DateTypeDef getTimeStampDate();

   private:
    RTC_TypeDef*             instance_{RTC};
    RCC_TypeDef*             rcc_instance_{RCC};
    PWR_TypeDef*             pwr_instance_{_PWR};
    RTC_DateTypeDef          date_;
    RTC_TimeTypeDef          time_;
    bool                     init_{false};
    bool                     change_{false};

    BcdDigits_t              decToBcd(uint8_t val);
    uint8_t                  bcdToDec(BcdDigits_t val);
    uint32_t                 timeInBCD(const RTC_TimeTypeDef& time);
    uint32_t                 dateInBCD(const RTC_DateTypeDef& date);

    static constexpr uint8_t WPR_KEY_SEQ1 = 0XCA;
    static constexpr uint8_t WPR_KEY_SEQ2 = 0X53;
    static constexpr uint8_t WPR_KEY_CLR  = 0XFF;
};