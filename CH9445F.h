/*******************************************************************************
 * CH9445F Type-C high-speed MUX driver
 ******************************************************************************/
#ifndef __CH9445F_H__
#define __CH9445F_H__

#include "CH543.H"

#define PIN_ATTR_IMPL(bank, attr)  (P##bank##_##attr)
#define PIN_ATTR(bank, attr)       PIN_ATTR_IMPL(bank, attr)
#define PIN_VALUE_IMPL(bank, pin)  (P##bank##_##pin)
#define PIN_VALUE(bank, pin)       PIN_VALUE_IMPL(bank, pin)
#define PIN_MASK(bank, pin)   (1 << pin)
#define CH_SCL_VALUE    PIN_VALUE(CH_SCL_BANK, CH_SCL_PIN)
#define CH_SDA_VALUE    PIN_VALUE(CH_SDA_BANK, CH_SDA_PIN)
#define DP_HPD_VALUE    PIN_VALUE(DP_HPD_BANK, DP_HPD_PIN)
#define POWER_EN_VALUE  PIN_VALUE(POWER_EN_BANK, POWER_EN_PIN)

#define SET_HIGH(bank, pin, attr) (PIN_ATTR(bank, attr) |= PIN_MASK(bank, pin))
#define SET_LOW(bank, pin, attr)  (PIN_ATTR(bank, attr) &= ~PIN_MASK(bank, pin))
#define SET_INPUT(name)      SET_LOW(name##_BANK, name##_PIN, MOD_OC);  SET_LOW(name##_BANK, name##_PIN, DIR_PU);
#define SET_PUSH_PULL(name)  SET_LOW(name##_BANK, name##_PIN, MOD_OC);  SET_HIGH(name##_BANK, name##_PIN, DIR_PU);
#define SET_OPEN_DRAIN(name) SET_HIGH(name##_BANK, name##_PIN, MOD_OC); SET_LOW(name##_BANK, name##_PIN, DIR_PU);
#define SET_INOUT(name)		 SET_HIGH(name##_BANK, name##_PIN, MOD_OC); SET_HIGH(name##_BANK, name##_PIN, DIR_PU);


#define CH9445F_REG_CVR         0x08
#define CH9445F_REG_MUX_PDR     0x10
#define CH9445F_REG_MUX_CTLR    0x11
#define CH9445F_REG_SYS_CTLR0   0x30
#define CH9445F_REG_SYS_CTLR1   0x31
#define CH9445F_REG_SYS_CTLR2   0x32

#define CH9445F_MODE_OFF        0
#define CH9445F_MODE_DP2        1
#define CH9445F_MODE_DP4        2


#define CH9445F_AUX_DIRECT      0x02
#define CH9445F_AUX_CROSS       0x01

/* MUX_CTLR fields from Table 5-2: POL/CT1/CT0. */
#define CH9445F_MUX_L           0x00
#define CH9445F_MUX_M           0x01
#define CH9445F_MUX_H           0x03
#define CH9445F_MUX_POL_L       0x00
#define CH9445F_MUX_POL_H       0x01
#define CH9445F_MUX_COMBINE(pol, ct1, ct0) \
	( ( (CH9445F_MUX_POL_ ## pol) << 4 ) | ( (CH9445F_MUX_ ## ct1) << 2 ) | (CH9445F_MUX_ ## ct0) )

#define CH9445F_MUX_DP2         CH9445F_MUX_COMBINE(L, L, M)
#define CH9445F_MUX_DP2_FLIP    CH9445F_MUX_COMBINE(H, L, M)
#define CH9445F_MUX_DP4         CH9445F_MUX_COMBINE(H, H, H)
#define CH9445F_MUX_DP4_FLIP    CH9445F_MUX_COMBINE(L, H, H)

#define CH9445F_SEL_A0          0x01
#define CH9445F_SEL_A1          0x02
#define CH9445F_SEL_A2          0x04
#define CH9445F_SEL_A3          0x08
#define CH9445F_SEL_B0          0x10
#define CH9445F_SEL_B1          0x20
#define CH9445F_SEL_B2          0x40
#define CH9445F_SEL_B3          0x80
#define CH9445F_SEL_C0          0x01
#define CH9445F_SEL_C1          0x02
#define CH9445F_SEL_C2          0x04
#define CH9445F_SEL_C3          0x08
#define CH9445F_SEL_D0          0x10
#define CH9445F_SEL_D1          0x20
#define CH9445F_SEL_D2          0x40
#define CH9445F_SEL_D3          0x80

#define CH9445F_CTRL0_SBU_CROSS     0x01
#define CH9445F_CTRL0_SBU_DIRECT    0x02

#define SS_RX1  CH9445F_SEL_B3
#define SS_TX1  CH9445F_SEL_D2
#define SS_TX2  CH9445F_SEL_D1
#define SS_RX2  CH9445F_SEL_B0

#define RX1_LNA CH9445F_SEL_A3
#define RX1_LND CH9445F_SEL_D3
#define TX1_LNB CH9445F_SEL_B2
#define TX1_LNC CH9445F_SEL_C2
#define TX2_LNB CH9445F_SEL_B1
#define TX2_LNC CH9445F_SEL_C1
#define RX2_LNA CH9445F_SEL_A0
#define RX2_LND CH9445F_SEL_D0


extern void CH9445F_Init( void );
extern void CH9445F_RequestMode(UINT8 req_mode, UINT8 req_flip);
extern void CH9445F_Apply();
extern bit DP_GetHPD();
extern void DP_SetHPD(UINT8 level);
extern void SetPowerEn(UINT8 level);
extern xdata UINT8 cc_flip;

#endif
