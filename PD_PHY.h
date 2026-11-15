/********************************** (C) COPYRIGHT *******************************
* File Name          : PD_PHY.H
* Author             : WCH
* Version            : V1.0
* Date               : 2022/01/13
* Description        : 
*********************************************************************************
* Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
* Attention: This software (modified or not) and binary are used for 
* microcontroller manufactured by Nanjing Qinheng Microelectronics.
********************************************************************************/


#ifndef	__PD_PHY_H__
#define __PD_PHY_H__

/*******************************************************************************/
/* ͷ�ļ����� */
#include "CH543.H"

/*******************************************************************************/
/* ��غ궨�� */
/* Message Type : Control Message */
#define GoodCRC		0x01		// 5B:00001
#define Accept		0x03		// 5B:00011
#define Reject		0x04		// 5B:00100
#define PS_RDY		0x06		// 5B:00110
#define GetSrcCap	0x07		// 5B:00111
#define GetSnkCap	0x08		// 5B:01000
#define DR_Swap		0x09		// 5B:01001
#define PR_Swap		0x0A		// 5B:01010
#define SoftRst		0x0D		// 5B:01101
#define NotSup		0x10		// 5B:10000

/* Message Type : Data Message */
#define SRCCap		0x01		// 5B:00001
#define Request		0x02		// 5B:00010
#define SinkCap     0x04 
#define VDef		0x0F

/* VDM Header */
#define VDM_SVID_DISCOVERY  0x0000
#define VDM_SVID_DP     0xFF01
#define VDM_SVID_STD    0xFF00
#define VDM_SVID_LIST_DP 0xFF010000

#define VDM_SVID_MASK       0xFFFF0000UL
#define VDM_CMDT_MASK       0x000000C0UL
#define VDM_CMD_MASK        0x0000003FUL
#define VDM_STRUCTURED      0x00008000UL
#define VDM_VERSION_10  	0x0000

#define VDM_CMDT_INIT   0x0000
#define VDM_CMDT_ACK    0x0040
#define VDM_CMDT_NAK    0x0080
#define VDM_CMDT_BUSY   0x00C0

#define VDM_DISC_ID     0x01
#define VDM_DISC_SVID   0x02
#define VDM_DISC_MODE   0x03
#define VDM_ENTER_MODE  0x04
#define VDM_EXIT_MODE   0x05
#define VDM_ATTENTION   0x06
#define VDM_DP_STATUS   0x10
#define VDM_DP_CONFIG   0x11

#define VDM_HDR(svid, cmdt, cmd) \
    (((UINT32)(svid) << 16) | VDM_STRUCTURED | VDM_VERSION_10 | \
     (cmdt) | (cmd))

#define DP_ATTN_HPD_STATE  0x00000080UL
#define DP_ATTN_HPD_IRQ    0x00000100UL

#define DP_PIN_A  0x01
#define DP_PIN_B  0x02
#define DP_PIN_C  0x04
#define DP_PIN_D  0x08
#define DP_PIN_E  0x10
#define DP_PIN_F  0x20
#define LOCAL_DP_PIN_2LANE  (DP_PIN_C | DP_PIN_D)
#define LOCAL_DP_PIN_4LANE  (DP_PIN_A | DP_PIN_B)
#define LOCAL_DP_PIN_MASK   (LOCAL_DP_PIN_2LANE | LOCAL_DP_PIN_4LANE)


#define DP_CFG_PIN_A  	0x01
#define DP_CFG_PIN_B  	0x02
#define DP_CFG_PIN_C  	0x04
#define DP_CFG_PIN_D  	0x08


#define DP_MODE_UFP_D_USB2_USB3 \
    (0x00000001UL | /* UFP_D */ \
     0x00000004UL | /* USB 2.0 signaling */ \
     0x00000008UL | /* USB 3.0 signaling */ \
     ((UINT32)(DP_PIN_C | DP_PIN_D) << 8))

#define DP_STATUS_CONNECTION_DFP_D  0x00000001UL
#define DP_STATUS_CONNECTION_UFP_D  0x00000002UL
#define DP_STATUS_ENABLED           0x00000008UL
#define DP_STATUS_PREFER_MULTI_FUNC 0x00000010UL
#define DP_STATUS_HPD_HIGH          0x00000080UL
#define DP_STATUS_HPD_IRQ           0x00000100UL
#define DP_STATUS_POWER_LOW         0x00000004UL
#define DP_CONFIG_UFP_U_AS_UFP_D  0x00000002UL
#define DP_CONFIG_SIGNALING  0x00000004UL
#define DP_CONFIG_PIN_MASK   0x0000FF00UL


/*******************************************************************************/
/* ȫ�ֱ������� */
extern xdata UINT8 PD_RX_BUF[30];
extern xdata UINT8 PD_TX_BUF[30];
extern xdata UINT8 PD_GCRC_BUF[2];

typedef struct  
{
	UINT8 	tmp;
	UINT8	SendMsgID;
	UINT8	WaitingGCRC;		// 0-δ׼������GoodCRC    x-�ƻ�����GoodCRC
	UINT8	QueueSendGCRC;		// 0-δ׼������GoodCRC    x-�ƻ�����GoodCRC
	UINT8	SOPType;			// Ŀǰ���͵�SOP״̬
		
	UINT8	RetryCnt;
		
	UINT8	RecvMsgID;
		
	UINT16	MsgCnt;				//��Ϣ�����͵���ʱ
	UINT8	MsgLen;				//��Ϣ����
		
	UINT8	PortPwrRole;
	UINT8	PortDataRole;
		
	UINT8	ExpectPwrRole;
	UINT8	ExpectDataRole;

	UINT8	status;
} _PD_PHY_STAT_st;

extern _PD_PHY_STAT_st  PD_PHY_Stat;

typedef union {				/* Header */
	struct _Header_Struct {
			
		UINT8	MsgType:5;
		UINT8	DataRole:1;		// 0:UFP  1:DFP
		UINT8	SpecRev:2;			// 01:Rev2.0
		
		UINT8	PwrRole:1;		// 0:Sink  1:Source
		UINT8	MsgID:3;
		UINT8	NDO:3;
		UINT8	Extended:1;
		
	} Struct;
	UINT16		Data;
} _Union_Header;

extern _Union_Header	*Header;

/*******************************************************************************/
/* �������� */
extern void Set_TX_GoodCRC( void );
extern void Check_If_GoodCRC( void );
extern void HeaderAutoFill( void );
extern void PD_PHY_Init( void );
extern void PD_PHY_RX_INIT( void );
extern void PD_PHY_TX_INIT( PUINT8 Addr, UINT8 Len, UINT8 sop );

#endif

