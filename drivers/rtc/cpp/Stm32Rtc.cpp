#include "Stm32RTC.hpp"

#include "RegisterUtils.hpp"
#include "low-level/pwr_bitfields.h"
#include "low-level/rcc_bitfields.h"

Stm32Rtc::Stm32Rtc() {}

Result<> Stm32Rtc::initialize() {
    /* Enable Power Interface Clock */
    RegisterUtils::setBits(rcc_instance_->APB1ENR, RCC_APB1ENR_PWR_EN);

    /* Enable DBP Bit to modify BDCR */
    RegisterUtils::setBits(pwr_instance_->CR, PWR_CR_DBP);

    /* Check if the RTC already initialize */
    if (IsInit() && !change_) {
        init_   = true;
        change_ = false;
        return Ok();

    } else {
        /* Enable LSE Crystal in Peripheral */
        RegisterUtils::setBits(rcc_instance_->BDCR, RCC_BDCR_LSEON);

        /* Wait until LSE is stable */
        while (!(rcc_instance_->BDCR & RCC_BDCR_LSERDY_Msk));

        /* Select RTC to use LSE oscillator clock */
        RegisterUtils::setBits(rcc_instance_->BDCR, RCC_BDCR_RTCSEL_LSE);

        /* Enable RTC Clock Enable */
        RegisterUtils::setBits(rcc_instance_->BDCR, RCC_BDCR_RTCEN);

        /* Write Protection Register First */
        instance_->WPR = WPR_KEY_SEQ1;
        instance_->WPR = WPR_KEY_SEQ2;

        /* 1. Set INIT Bit in ISR to enter initialization mode. */
        RegisterUtils::setBits(instance_->ISR, RTC_ISR_INIT);

        /* 2. Wait until INITF to set before configure */
        while (!(instance_->ISR & RTC_ISR_INITF));

        /* 3. Program  Sync = 256, Async = 128 in RTC_PRER */
        instance_->PRER = (127U << RTC_PREDIV_A_Pos) | (255U << RTC_PREDIV_S_Pos);

        /* 4. Load Time Register, Date Register */
        instance_->TR = timeInBCD(time_);
        instance_->DR = dateInBCD(date_);

        /* 5. Exit Initialization Mode */
        RegisterUtils::clearBits(instance_->ISR, RTC_ISR_INIT);

        /* Enable Write Protection */
        instance_->WPR = WPR_KEY_CLR;

        init_          = true;
        change_        = false;
        return Ok();
    }
}
uint32_t Stm32Rtc::timeInBCD(const RTC_TimeTypeDef& time) {
    BcdDigits_t sec = decToBcd(time.Seconds);
    BcdDigits_t min = decToBcd(time.Minutes);
    BcdDigits_t hrs = decToBcd(time.Hours);

    return (time.TimeFormat << RTC_TR_PM_Pos) | (hrs.tens << RTC_TR_HT_Pos) | (hrs.ones << RTC_TR_HU_Pos) | (min.tens << RTC_TR_MNT_Pos) | (min.ones << RTC_TR_MNU_Pos) | (sec.tens << RTC_TR_ST_Pos) |
           (sec.ones << RTC_TR_SU_Pos);
}
uint32_t Stm32Rtc::dateInBCD(const RTC_DateTypeDef& d) {
    BcdDigits_t date  = decToBcd(d.Day);
    BcdDigits_t month = decToBcd(d.Month);
    BcdDigits_t year  = decToBcd(d.Year);

    return (d.WeekDay << RTC_DR_WDU_Pos) | (year.tens << RTC_DR_YT_Pos) | (year.ones << RTC_DR_YU_Pos) | (month.tens << RTC_DR_MT_Pos) | (month.ones << RTC_DR_MU_Pos) | (date.tens << RTC_DR_DT_Pos) |
           (date.ones << RTC_DR_DU_Pos);
}
RTC_TimeTypeDef Stm32Rtc::getTimeStampTime() {
    const uint32_t time = instance_->TSTR;

    BcdDigits_t    sec{static_cast<uint8_t>((time & RTC_TSTR_ST_Msk) >> RTC_TSTR_ST_Pos), static_cast<uint8_t>((time & RTC_TSTR_SU_Msk) >> RTC_TSTR_SU_Pos)};
    BcdDigits_t    min{static_cast<uint8_t>((time & RTC_TSTR_MNT_Msk) >> RTC_TSTR_MNT_Pos), static_cast<uint8_t>((time & RTC_TSTR_MNU_Msk) >> RTC_TSTR_MNU_Pos)};
    BcdDigits_t    hrs{static_cast<uint8_t>((time & RTC_TSTR_HT_Msk) >> RTC_TSTR_HT_Pos), static_cast<uint8_t>((time & RTC_TSTR_HU_Msk) >> RTC_TSTR_HU_Pos)};

    return RTC_TimeTypeDef{.Hours = bcdToDec(hrs), .Minutes = bcdToDec(min), .Seconds = bcdToDec(sec), .TimeFormat = static_cast<uint8_t>((time & RTC_TSTR_PM_Msk) >> RTC_TSTR_PM_Pos)};
}
RTC_DateTypeDef Stm32Rtc::getTimeStampDate() {
    const uint32_t dateTsReg = instance_->TSDR;
    const uint32_t dateReg   = instance_->DR;

    BcdDigits_t    date{static_cast<uint8_t>((dateTsReg & RTC_TSDR_DT_Msk) >> RTC_TSDR_DT_Pos), static_cast<uint8_t>((dateTsReg & RTC_TSDR_DU_Msk) >> RTC_TSDR_DU_Pos)};
    BcdDigits_t    month{static_cast<uint8_t>((dateTsReg & RTC_TSDR_MT_Msk) >> RTC_TSDR_MT_Pos), static_cast<uint8_t>((dateTsReg & RTC_TSDR_MU_Msk) >> RTC_TSDR_MU_Pos)};
    BcdDigits_t    year{static_cast<uint8_t>((dateReg & RTC_DR_YT_Msk) >> RTC_DR_YT_Pos), static_cast<uint8_t>((dateReg & RTC_DR_YU_Msk) >> RTC_DR_YU_Pos)};

    return RTC_DateTypeDef{.WeekDay = static_cast<uint8_t>((dateTsReg & RTC_TSDR_WDU_Msk) >> RTC_TSDR_WDU_Pos), .Year = bcdToDec(year), .Month = bcdToDec(month), .Day = bcdToDec(date)};
}
BcdDigits_t Stm32Rtc::decToBcd(uint8_t val) {
    return {uint8_t(val / 10), uint8_t(val % 10)};
}
uint8_t Stm32Rtc::bcdToDec(BcdDigits_t val) {
    return (val.tens * 10) + val.ones;
}
TimeStamp_t Stm32Rtc::getTimeStamp() {
    return TimeStamp_t{.day = getTimeStampDate(), .time = getTimeStampTime()};
}
RTC_DateTypeDef Stm32Rtc::getDate() {
    const uint32_t dateReg = instance_->DR;

    BcdDigits_t    date{static_cast<uint8_t>((dateReg & RTC_DR_DT_Msk) >> RTC_DR_DT_Pos), static_cast<uint8_t>((dateReg & RTC_DR_DU_Msk) >> RTC_DR_DU_Pos)};
    BcdDigits_t    month{static_cast<uint8_t>((dateReg & RTC_DR_MT_Msk) >> RTC_DR_MT_Pos), static_cast<uint8_t>((dateReg & RTC_DR_MU_Msk) >> RTC_DR_MU_Pos)};
    BcdDigits_t    year{static_cast<uint8_t>((dateReg & RTC_DR_YT_Msk) >> RTC_DR_YT_Pos), static_cast<uint8_t>((dateReg & RTC_DR_YU_Msk) >> RTC_DR_YU_Pos)};

    return RTC_DateTypeDef{.WeekDay = static_cast<uint8_t>((dateReg & RTC_DR_WDU_Msk) >> RTC_DR_WDU_Pos), .Year = bcdToDec(year), .Month = bcdToDec(month), .Day = bcdToDec(date)};
}
RTC_TimeTypeDef Stm32Rtc::getTime() {
    const uint32_t time = instance_->TR;

    BcdDigits_t    sec{static_cast<uint8_t>((time & RTC_TR_ST_Msk) >> RTC_TR_ST_Pos), static_cast<uint8_t>((time & RTC_TR_SU_Msk) >> RTC_TR_SU_Pos)};
    BcdDigits_t    min{static_cast<uint8_t>((time & RTC_TR_MNT_Msk) >> RTC_TR_MNT_Pos), static_cast<uint8_t>((time & RTC_TR_MNU_Msk) >> RTC_TR_MNU_Pos)};
    BcdDigits_t    hrs{static_cast<uint8_t>((time & RTC_TR_HT_Msk) >> RTC_TR_HT_Pos), static_cast<uint8_t>((time & RTC_TR_HU_Msk) >> RTC_TR_HU_Pos)};

    return RTC_TimeTypeDef{.Hours = bcdToDec(hrs), .Minutes = bcdToDec(min), .Seconds = bcdToDec(sec), .TimeFormat = static_cast<uint8_t>((time & RTC_TR_PM_Msk) >> RTC_TR_PM_Pos)};
}
void Stm32Rtc::setDate(const RTC_DateTypeDef& d) {
    date_   = d;
    change_ = true;
}
void Stm32Rtc::setTime(const RTC_TimeTypeDef& t) {
    time_   = t;
    change_ = true;
}
bool Stm32Rtc::IsInit() const {
    return (instance_->ISR & RTC_ISR_INITS) != 0;
}
