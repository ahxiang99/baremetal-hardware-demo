#pragma once

/* Time Registers */
#define RTC_TR_SU_Pos 0
#define RTC_TR_SU_Msk (0xFU << RTC_TR_SU_Pos)
#define RTC_TR_ST_Pos 4
#define RTC_TR_ST_Msk (0x7U << RTC_TR_ST_Pos)
#define RTC_TR_MNU_Pos 8
#define RTC_TR_MNU_Msk (0xFU << RTC_TR_MNU_Pos)
#define RTC_TR_MNT_Pos 12
#define RTC_TR_MNT_Msk (0x7U << RTC_TR_MNT_Pos)
#define RTC_TR_HU_Pos 16
#define RTC_TR_HU_Msk (0xFU << RTC_TR_HU_Pos)
#define RTC_TR_HT_Pos 20
#define RTC_TR_HT_Msk (0x3U << RTC_TR_HT_Pos)
#define RTC_TR_PM_Pos 22
#define RTC_TR_PM_Msk (0x1U << RTC_TR_PM_Pos)

/* Date Registers */
#define RTC_DR_DU_Pos 0
#define RTC_DR_DU_Msk (0xFU << RTC_DR_DU_Pos)
#define RTC_DR_DT_Pos 4
#define RTC_DR_DT_Msk (0x3U << RTC_DR_DT_Pos)
#define RTC_DR_MU_Pos 8
#define RTC_DR_MU_Msk (0xFU << RTC_DR_MU_Pos)
#define RTC_DR_MT_Pos 12
#define RTC_DR_MT_Msk (0x1U << RTC_DR_MT_Pos)
#define RTC_DR_WDU_Pos 13
#define RTC_DR_WDU_Msk (0x7U << RTC_DR_WDU_Pos)
#define RTC_DR_YU_Pos 16
#define RTC_DR_YU_Msk (0xFU << RTC_DR_YU_Pos)
#define RTC_DR_YT_Pos 20
#define RTC_DR_YT_Msk (0xFU << RTC_DR_YT_Pos)

/* Control Registers */
#define RTC_CR_WUCKSEL_Pos 0
#define RTC_CR_WUCKSEL_Msk (0x7F << RTC_CR_WUCKSEL_Pos)
#define RTC_CR_TSEDGE_Pos 3
#define RTC_CR_REFCKON_Pos 4
#define RTC_CR_BYPSHAD_Pos 5
#define RTC_CR_FMT_Pos 6
#define RTC_CR_DCE_Pos 7
#define RTC_CR_ALRAE_Pos 8
#define RTC_CR_ALRBE_Pos 9
#define RTC_CR_WUTE_Pos 10
#define RTC_CR_TSE_Pos 11
#define RTC_CR_ALRAIE_Pos 12
#define RTC_CR_ALRBIE_Pos 13
#define RTC_CR_WUTIE_Pos 14
#define RTC_CR_TSIE_Pos 15
#define RTC_CR_ADD1H_Pos 16
#define RTC_CR_SUB1H_Pos 17
#define RTC_CR_BKP_Pos 18
#define RTC_CR_COSEL_Pos 19
#define RTC_CR_POL_Pos 20
#define RTC_CR_OSEL_Pos 21
#define RTC_CR_OSEL_Msk (0x3U << RTC_CR_OSEL_Pos)
#define RTC_CR_COE_Pos 23

/* Initialization and Status Registers */
#define RTC_ISR_ALRAWF (1 << 0)
#define RTC_ISR_ALRBWF (1 << 1)
#define RTC_ISR_WUTWF (1 << 2)
#define RTC_ISR_SHPF (1 << 3)
#define RTC_ISR_INITS (1 << 4)
#define RTC_ISR_RSF (1 << 5)
#define RTC_ISR_INITF (1 << 6)
#define RTC_ISR_INIT (1 << 7)
#define RTC_ISR_ALRAF (1 << 8)
#define RTC_ISR_ALRBF (1 << 9)
#define RTC_ISR_WUTF (1 << 10)
#define RTC_ISR_TSF (1 << 11)
#define RTC_ISR_TSOVF (1 << 12)
#define RTC_ISR_TAMP1F (1 << 13)
#define RTC_ISR_RECALPF (1 << 16)

/* Prescalar Registers */
#define RTC_PREDIV_S_Pos 0
#define RTC_PREDIV_A_Pos 16

/* Calibration Register */
#define RTC_CALIBR_DC_Pos 0
#define RTC_CALIBR_DC_Mask (0x1FU << RTC_CALIBR_DC_Pos)
#define RTC_CALIBR_DCS (1 << 7)

/* RTC Alarm A Register */
#define RTC_ALRMAR_SU_Pos 0
#define RTC_ALRMAR_SU_Msk (0xFU << RTC_ALRMAR_SU_Pos)
#define RTC_ALRMAR_ST_Pos 4
#define RTC_ALRMAR_ST_Msk (0x7U << RTC_ALRMAR_ST_Pos)
#define RTC_ALRMAR_MSK1_Pos 7
#define RTC_ALRMAR_MNU_Pos 8
#define RTC_ALRMAR_MNU_Msk (0xFU << RTC_ALRMAR_MNU_Pos)
#define RTC_ALRMAR_MNT_Pos 12
#define RTC_ALRMAR_MNT_Msk (0x7U << RTC_ALRMAR_MNT_Pos)
#define RTC_ALRMAR_MSK2_Pos 15
#define RTC_ALRMAR_HU_Pos 16
#define RTC_ALRMAR_HU_Msk (0xFU << RTC_ALRMAR_HU_Pos)
#define RTC_ALRMAR_HT_Pos 20
#define RTC_ALRMAR_HT_Msk (0x3U << RTC_ALRMAR_HT_Pos)
#define RTC_ALRMAR_PM_Pos 22
#define RTC_ALRMAR_PM_Msk (0x1U << RTC_ALRMAR_PM_Pos)
#define RTC_ALRMAR_MSK3_Pos 23
#define RTC_ALRMAR_DU_Pos 24
#define RTC_ALRMAR_DU_Msk (0xFU << RTC_ALRMAR_DU_Pos)
#define RTC_ALRMAR_DT_Pos 28
#define RTC_ALRMAR_DT_Msk (0x3U << RTC_ALRMAR_DT_Pos)
#define RTC_ALRMAR_WDSEL_Pos 30
#define RTC_ALRMAR_MSK4_Pos 31

/* RTC Alarm B Register */
#define RTC_ALRMBR_SU_Pos 0
#define RTC_ALRMBR_SU_Msk (0xFU << RTC_ALRMAR_SU_Pos)
#define RTC_ALRMBR_ST_Pos 4
#define RTC_ALRMBR_ST_Msk (0x7U << RTC_ALRMAR_ST_Pos)
#define RTC_ALRMBR_MSK1_Pos 7
#define RTC_ALRMBR_MNU_Pos 8
#define RTC_ALRMBR_MNU_Msk (0xFU << RTC_ALRMAR_MNU_Pos)
#define RTC_ALRMBR_MNT_Pos 12
#define RTC_ALRMBR_MNT_Msk (0x7U << RTC_ALRMAR_MNT_Pos)
#define RTC_ALRMBR_MSK2_Pos 15
#define RTC_ALRMBR_HU_Pos 16
#define RTC_ALRMBR_HU_Msk (0xFU << RTC_ALRMAR_HU_Pos)
#define RTC_ALRMBR_HT_Pos 20
#define RTC_ALRMBR_HT_Msk (0x3U << RTC_ALRMAR_HT_Pos)
#define RTC_ALRMBR_PM_Pos 22
#define RTC_ALRMBR_PM_Msk (0x1U << RTC_ALRMAR_PM_Pos)
#define RTC_ALRMBR_MSK3_Pos 23
#define RTC_ALRMBR_DU_Pos 24
#define RTC_ALRMBR_DU_Msk (0xFU << RTC_ALRMAR_DU_Pos)
#define RTC_ALRMBR_DT_Pos 28
#define RTC_ALRMBR_DT_Msk (0x3U << RTC_ALRMAR_DT_Pos)
#define RTC_ALRMBR_WDSEL_Pos 30
#define RTC_ALRMBR_MSK4_Pos 31

/* Time Stamp Time Register */
#define RTC_TSTR_SU_Pos 0
#define RTC_TSTR_SU_Msk (0xFU << RTC_TSTR_SU_Pos)
#define RTC_TSTR_ST_Pos 4
#define RTC_TSTR_ST_Msk (0x7U << RTC_TSTR_ST_Pos)
#define RTC_TSTR_MNU_Pos 8
#define RTC_TSTR_MNU_Msk (0xFU << RTC_TSTR_MNU_Pos)
#define RTC_TSTR_MNT_Pos 12
#define RTC_TSTR_MNT_Msk (0x7U << RTC_TSTR_MNT_Pos)
#define RTC_TSTR_HU_Pos 16
#define RTC_TSTR_HU_Msk (0xFU << RTC_TSTR_HU_Pos)
#define RTC_TSTR_HT_Pos 20
#define RTC_TSTR_HT_Msk (0x3U << RTC_TSTR_HT_Pos)
#define RTC_TSTR_PM_Pos 22
#define RTC_TSTR_PM_Msk (0x1U << RTC_TSTR_PM_Pos)

/* Time Stamp Date Registers */
#define RTC_TSDR_DU_Pos 0
#define RTC_TSDR_DU_Msk (0xFU << RTC_DR_DU_Pos)
#define RTC_TSDR_DT_Pos 4
#define RTC_TSDR_DT_Msk (0x3U << RTC_DR_DT_Pos)
#define RTC_TSDR_MU_Pos 8
#define RTC_TSDR_MU_Msk (0xFU << RTC_DR_MU_Pos)
#define RTC_TSDR_MT_Pos 12
#define RTC_TSDR_MT_Msk (0x1U << RTC_DR_MT_Pos)
#define RTC_TSDR_WDU_Pos 13
#define RTC_TSDR_WDU_Msk (0x7U << RTC_DR_WDU_Pos)