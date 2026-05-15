#ifndef TIM_BITFIELDS_H
#define TIM_BITFIELDS_H

#define TIM_CR1_CEN (1 << 0)
#define TIM_CR1_UDIS (1 << 1)
#define TIM_CR1_URS (1 << 2)
#define TIM_CR1_OPM (1 << 3)
#define TIM_CR1_DIR (1 << 4)
#define TIM_CR1_CMS (1 << 5)
#define TIM_CR1_ARPE (1 << 7)
#define TIM_CR1_CKD (1 << 8)

#define TIM_CR2_CCDS (1 << 3)
#define TIM_CR2_MMS (1 << 4)
#define TIM_CR2_TI1S (1 << 7)

#define TIM_SMCR_SMS (1 << 0)
#define TIM_SMCR_TS (1 << 4)
#define TIM_SMCR_MSM (1 << 7)
#define TIM_SMCR_MSM (1 << 7)
#define TIM_SMCR_ETF (1 << 8)
#define TIM_SMCR_ETPS (1 << 12)
#define TIM_SMCR_ECE (1 << 14)
#define TIM_SMCR_ETP (1 << 15)

#define TIM_SR_UIF (1 << 0)

#define TIM_DIER_UIE (1 << 0)
#define TIM_DIER_CC1IE (1 << 1)
#define TIM_DIER_CC2IE (1 << 2)
#define TIM_DIER_CC3IE (1 << 3)
#define TIM_DIER_CC4IE (1 << 4)
#define TIM_DIER_TIE (1 << 6)
#define TIM_DIER_UDE (1 << 8)
#define TIM_DIER_CC1DE (1 << 9)
#define TIM_DIER_CC2DE (1 << 10)
#define TIM_DIER_CC3DE (1 << 11)
#define TIM_DIER_CC4DE (1 << 12)
#define TIM_DIER_TDE (1 << 14)

#define TIM_EGR_UG (1 << 0)
#define TIM_EGR_CC1G (1 << 1)
#define TIM_EGR_CC2G (1 << 2)
#define TIM_EGR_CC3G (1 << 3)
#define TIM_EGR_CC4G (1 << 4)
#define TIM_EGR_TG (1 << 6)

#endif