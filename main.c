/********************************** (C) COPYRIGHT *******************************
* File Name          : main.C
* Author             : WCH
* Version            : V1.0
* Date               : 2022/04/28
* Description        : CH543 PD Sink功能, 固定请求5V3A档位
*                      Notice that this example also needs to add TIMER.C file.
*********************************************************************************
* Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
* Attention: This software (modified or not) and binary are used for 
* microcontroller manufactured by Nanjing Qinheng Microelectronics.
********************************************************************************/


/*******************************************************************************/
/* 相关宏定义 */
#define PD_PROT_EXAM 1

/*******************************************************************************/
/* 头文件包含 */
#include <stdio.h>
#include "CH543.H"
#include "DEBUG.H"
#include "PD_PHY.H"
#include "PD_PROT.H"
#include "CH9445F.h"

/********************************************************************************
* Function Name  : main
* Description    : 主循环
* Input          : None
* Output         : None
* Return         : None
********************************************************************************/ 

#define CC1 CC1_CTRL
#define CC2 CC2_CTRL
#define UPD_USE_CC1 UPD_CC_CTRL1
#define UPD_USE_CC2 UPD_CC_CTRL2

#define LED1_BANK 1
#define LED1_PIN 3
#define LED2_BANK 1
#define LED2_PIN 2

void ChooseRole() {
	PD_PHY_Stat.PortDataRole = 0;
}


xdata UINT8 cc_flip = 0;
#define UPD_CC_CTRL1 (UPD_CTRL &= ~bPD_CC_SEL)
#define UPD_CC_CTRL2 (UPD_CTRL |= bPD_CC_SEL)
#define CC_HIGH(cc) (cc & bCC_CMPO)
#define CC_LOW(cc)  (!CC_HIGH(cc))

void main( void ) 
{
	UINT8 cc_dis_cnt = 0;

	CfgFsys( );							                                        /* 24MHz */
	mInitSTDIO( );						                                        /* UART初始化 */
	ChooseRole();
	CH9445F_Init( );
	PD_PHY_Init( );
	
	SetPowerEn(PD_PHY_Stat.PortPwrRole);
	SET_PUSH_PULL(LED1);
	SET_PUSH_PULL(LED2);
	PIN_VALUE(LED1_BANK, LED1_PIN) = 1;
	PIN_VALUE(LED2_BANK, LED2_PIN) = 1;

	mDelaymS( 10 );
	printf("CH543D %s (Version %s %s)\r\n", PD_PHY_Stat.PortDataRole ? "Source" : "Sink", __DATE__, __TIME__);
	
	while( 1 ) 
    {					
        /* 检测设备接入 */

		if (PD_PHY_Stat.PortDataRole) {
			if (CC_LOW(CC2) && CC_HIGH(CC1)) {
				cc_flip = 1;
				break;
			}
			if (CC_LOW(CC1) && CC_HIGH(CC2)) {
				cc_flip = 0;
				break;
			}
			if (CC_LOW(CC1) && CC_LOW(CC2)) {
				break;
			}
		} else {
			if (CC_HIGH(CC2) && CC_LOW(CC1)) {
				cc_flip = 1;
				break;
			}
			if (CC_HIGH(CC1) && CC_LOW(CC2)) {
				cc_flip = 0;
				break;
			}
		}
		mDelaymS( 5 );
	}

	// 检查E-marker
	if (PD_PHY_Stat.PortDataRole) {
		// 强上拉，5k1会识别成1，1k会识别成0
		CC1_CTRL = CC2_CTRL = bCC_CE | bCC_PU1;
		mDelaymS(100);
		if (CC_HIGH(CC1) && CC_HIGH(CC2)) {
			// keep cc_flip
		} else if (CC_HIGH(CC1) || CC_HIGH(CC2)) {
			cc_flip = CC_HIGH(CC2);
			printf("E-marker detected\n");
		} else {
			printf("invalid CC state\n");
		}
		// 弱上拉，用于检测是否断开
		CC1_CTRL = CC2_CTRL = bCC_CE | bCC_PU1 | bCC_PU0;
		mDelaymS(100);
	}
	
	if (cc_flip) {
		UPD_USE_CC2;
		PIN_VALUE(LED2_BANK, LED2_PIN) = 0;
	} else {
		UPD_USE_CC1;
		PIN_VALUE(LED1_BANK, LED1_PIN) = 0;
	}

	printf("type-c attached, cc_flip=%d\n", (int)cc_flip);
	PD_PHY_Stat.status = PD_ATTACHED;
	
	UPD_CTRL |= bPD_DMA_EN;		//启动PD DMA
	PD_PHY_RX_INIT( );			//PD进入接收状态
	IE_USBPD = 1;				//开启中断
	EA = 1;
	
	if (PD_PHY_Stat.PortDataRole) {
		mDelaymS( 500 );
		PD_PROT_HANDLE_SRC();
	}
	// else {
	// 	PD_PROT_HANDLE();
	// }
	
	while (cc_dis_cnt < 100) {
		if (PD_PHY_Stat.PortDataRole) {
			if (CC_HIGH(CC1) && CC_HIGH(CC2)) {
				cc_dis_cnt++;
			} else {
				cc_dis_cnt  = 0;
			}
		} else {
			if (CC_LOW(CC1) && CC_LOW(CC2)) {
				cc_dis_cnt++;
			} else {
				cc_dis_cnt = 0;
			}
			// FIXME：这里应该轮询/中断检测HPD
		}
		CH9445F_Apply();
		mDelaymS( 10 );
	}

	printf("type-c disconnected\n");
	mDelaymS( 10 );
	CH543SoftReset();
	while (1);
}


