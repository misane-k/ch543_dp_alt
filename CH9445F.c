/*******************************************************************************
 * CH9445F software I2C and high-speed MUX control
 ******************************************************************************/
#include "CH9445F.h"
#include "PD_PHY.H"
#include "DEBUG.H"

#define CH_SCL_BANK 1
#define CH_SCL_PIN 5
#define CH_SDA_BANK 3
#define CH_SDA_PIN 2
#define DP_HPD_BANK 1
#define DP_HPD_PIN 4
#define POWER_EN_BANK 3
#define POWER_EN_PIN 4

#define CH9445F_I2C_ADDR 0x48

void CH9445F_Init( void ) {
    SET_OPEN_DRAIN(CH_SCL);
    SET_OPEN_DRAIN(CH_SDA);
    if (PD_PHY_Stat.PortDataRole) {
        SET_PUSH_PULL(DP_HPD);
        DP_SetHPD( 0 );
    } else {
        SET_INPUT(DP_HPD);
    }
    SET_PUSH_PULL(POWER_EN);
    SetPowerEn( 0 );

    CH9445F_RequestMode(CH9445F_MODE_DP2, 0);
    CH9445F_Apply();
}


#define debug_printf printf

static void CH9445F_SDA_High( void ) {
    CH_SDA_VALUE = 1;
}

static void CH9445F_SDA_Low( void ) {
    CH_SDA_VALUE = 0;
}

static void CH9445F_SCL_High( void ) {
    CH_SCL_VALUE = 1;
}

static void CH9445F_SCL_Low( void ) {
    CH_SCL_VALUE = 0;
}

void DP_SetHPD(UINT8 level) {
    DP_HPD_VALUE = level ? 1 : 0;
    // debug_printf("SetHPD=%u\r\n", (UINT16)level);
}

bit DP_GetHPD() {
    // debug_printf("GetHPD=%u\r\n", (UINT16)DP_HPD_VALUE);
    return DP_HPD_VALUE ? 1 : 0;
}

void SetPowerEn(UINT8 level) {
    POWER_EN_VALUE = level ? 0 : 1;
    // debug_printf("POWER_EN_N=%u\r\n", (UINT16)level);
}

static void CH9445F_Start( void )
{
    CH9445F_SDA_High( );
    CH9445F_SCL_High( );
    mDelayuS( 2 );
    CH9445F_SDA_Low( );
    mDelayuS( 2 );
    CH9445F_SCL_Low( );
}

static void CH9445F_Stop( void )
{
    CH9445F_SDA_Low( );
    CH9445F_SCL_High( );
    mDelayuS( 2 );
    CH9445F_SDA_High( );
    mDelayuS( 2 );
}

static UINT8 CH9445F_WriteByte( UINT8 value )
{
    UINT8 bitIndex;
    UINT8 ack;

    for( bitIndex = 0; bitIndex < 8; bitIndex++ )
    {
        if( value & 0x80 ) CH9445F_SDA_High( );
        else CH9445F_SDA_Low( );
        CH9445F_SCL_High( );
        mDelayuS( 2 );
        CH9445F_SCL_Low( );
        value <<= 1;
    }

    CH9445F_SDA_High( );
    CH9445F_SCL_High( );
    mDelayuS( 2 );
    ack = CH_SDA_VALUE ? 0 : 1;
    CH9445F_SCL_Low( );
    return ack;
}

static UINT8 CH9445F_ReadByte( void )
{
    UINT8 bitIndex;
    UINT8 value;

    value = 0;
    CH9445F_SDA_High( );
    for( bitIndex = 0; bitIndex < 8; bitIndex++ )
    {
        value <<= 1;
        CH9445F_SCL_High( );
        mDelayuS( 2 );
        if( CH_SDA_VALUE ) value |= 1;
        CH9445F_SCL_Low( );
        mDelayuS( 2 );
    }
    return value;
}

static void CH9445F_WriteNack( void )
{
    CH9445F_SDA_High( );
    CH9445F_SCL_High( );
    mDelayuS( 2 );
    CH9445F_SCL_Low( );
}

static UINT8 CH9445F_WriteReg( UINT8 reg, UINT8 value )
{
    UINT8 result;

    CH9445F_Start( );
    result = CH9445F_WriteByte( ( CH9445F_I2C_ADDR << 1 ) | 0 );
    if( result ) result = CH9445F_WriteByte( reg );
    if( result ) result = CH9445F_WriteByte( value );
    CH9445F_Stop( );
	// debug_printf("CH9445F W %02X=%02X ack=%u\r\n", (UINT16)reg, (UINT16)value, (UINT16)result);
    return result;
}

static UINT8 CH9445F_ReadReg( UINT8 reg )
{
    UINT8 result;
    UINT8 value;

    value = 0xFF;
    CH9445F_Start( );
    result = CH9445F_WriteByte( ( CH9445F_I2C_ADDR << 1 ) | 0 );
    if( result ) result = CH9445F_WriteByte( reg );
    if( result )
    {
        CH9445F_Start( );
        result = CH9445F_WriteByte( ( CH9445F_I2C_ADDR << 1 ) | 1 );
    }
    if( result )
    {
        value = CH9445F_ReadByte( );
        CH9445F_WriteNack( );
    }
    CH9445F_Stop( );
    // debug_printf("CH9445F R %02X=%02X ack=%u\r\n", (UINT16)reg, (UINT16)value, (UINT16)result);
    return value;
}

#define CH9445F_VerifyReg(reg, expected)                  \
    do {                                                  \
        UINT8 actual;                                     \
        actual = CH9445F_ReadReg(reg);                    \
        if (actual != (UINT8)(expected)) {                \
            printf("CH9445F verify failed at %d: "        \
                   "reg=%02X value=%02X expect=%02X\r\n", \
                   (int)__LINE__,                         \
                   (UINT16)(reg),                         \
                   (UINT16)actual,                        \
                   (UINT16)(expected));                   \
            while (1);                                    \
        }                                                 \
    } while (0)
// #define CH9445F_VerifyReg(reg, value) \
//     (CH9445F_ReadReg(reg) == (UINT8)(value))

static xdata UINT8 mode;
static xdata UINT8 flip;
static xdata UINT8 pending;

void CH9445F_RequestMode(UINT8 req_mode, UINT8 req_flip) {
    mode = req_mode;
    flip = req_flip;
    pending = 1;
}

#include "PD_PHY.H"
void CH9445F_Apply() {
    UINT8 ctrl0 = 0;
    UINT8 ctrl1 = 0;
    UINT8 ctrl2 = 0;

    if (!pending) return;
    if( mode == CH9445F_MODE_DP2 || mode == CH9445F_MODE_DP4 ) {
        
        if (PD_PHY_Stat.PortDataRole) {
            ctrl0 = flip ? CH9445F_CTRL0_SBU_CROSS : CH9445F_CTRL0_SBU_DIRECT;
            ctrl1 = flip ? CH9445F_CTRL1_SOURCE_DP2_FLIP : CH9445F_CTRL1_SOURCE_DP2;
            ctrl2 = flip ? CH9445F_CTRL2_SOURCE_DP2_FLIP : CH9445F_CTRL2_SOURCE_DP2;
        } else {
            ctrl0 = flip ? CH9445F_CTRL0_SBU_DIRECT : CH9445F_CTRL0_SBU_CROSS;
            ctrl1 = flip ? CH9445F_CTRL1_SINK_DP2_FLIP : CH9445F_CTRL1_SINK_DP2;
            ctrl2 = flip ? CH9445F_CTRL2_SINK_DP2_FLIP : CH9445F_CTRL2_SINK_DP2;
        }

        CH9445F_WriteReg( CH9445F_REG_MUX_PDR, 0x00 );
        CH9445F_VerifyReg( CH9445F_REG_MUX_PDR, 0x00 );
        // SYS_CTLR0/1/2 and MUX_CTLR are alternative switch controls. 
        CH9445F_WriteReg( CH9445F_REG_MUX_CTLR, 0x00 );
        CH9445F_VerifyReg( CH9445F_REG_MUX_CTLR, 0x00 );
        
        CH9445F_WriteReg( CH9445F_REG_SYS_CTLR0, ctrl0 );
        CH9445F_VerifyReg( CH9445F_REG_SYS_CTLR0, ctrl0 );
        CH9445F_WriteReg( CH9445F_REG_SYS_CTLR1, ctrl1 );
        CH9445F_WriteReg( CH9445F_REG_SYS_CTLR2, ctrl2 );
        CH9445F_VerifyReg( CH9445F_REG_SYS_CTLR1, ctrl1 );
        CH9445F_VerifyReg( CH9445F_REG_SYS_CTLR2, ctrl2 );
    }
    else
    {
        CH9445F_WriteReg( CH9445F_REG_MUX_PDR, 0x01 );
        CH9445F_VerifyReg( CH9445F_REG_MUX_PDR, 0x01 );
    }
    pending = 0;

    debug_printf("MUX mode=%u flip=%u\r\n", (UINT16)mode, (UINT16)flip);
    debug_printf("MUX ctrl0=%02X ctrl1=%02X ctrl2=%02X\r\n", (UINT16)ctrl0, (UINT16)ctrl1, (UINT16)ctrl2);
}
