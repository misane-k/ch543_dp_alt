/********************************** (C) COPYRIGHT *******************************
* File Name          : PD_PORT.C
* Author             : WCH
* Version            : V1.0
* Date               : 2022/04/28
* Description        : 
*********************************************************************************
* Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
* Attention: This software (modified or not) and binary are used for 
* microcontroller manufactured by Nanjing Qinheng Microelectronics.
********************************************************************************/


/*******************************************************************************/
/* 头文件包含 */
#include <stdio.h>
#include "CH543.H"
#include "DEBUG.H"
#include "PD_PHY.H"
#include "PD_PROT.H"
#include "CH9445F.h"

/*******************************************************************************/
/* 全局变量定义 */
_Union_Fixed	*Fixed;
// #define debug_printf printf
#define debug_printf (void*)

/********************************************************************************
* Function Name  : PD_PROT_HANDLE
* Description    : PD 协议层消息处理，此时已经生成了GoodCRC消息但未发送，
					所以此处生成的消息需要设定发送延时
* Input          : None
* Output         : None
* Return         : None
********************************************************************************/ 

static UINT32 Swap32(UINT32 value) {
    UINT32 result;
    UINT8 *source;
    UINT8 *target;

    source = (UINT8 *)&value;
    target = (UINT8 *)&result;

    target[0] = source[3];
    target[1] = source[2];
    target[2] = source[1];
    target[3] = source[0];

    return result;
}

static void SendCtrlMsg(UINT8 MsgType, UINT8 RetryCount) {
    Header = PD_TX_BUF;

    HeaderAutoFill();
    Header->Struct.NDO = 0;
    Header->Struct.MsgType = MsgType;

    PD_PHY_Stat.RetryCnt = RetryCount;
    PD_PHY_Stat.MsgCnt = 4;
    PD_PHY_Stat.MsgLen = 2;
}

static void SendDataMsg(UINT8 MsgType, UINT8 RetryCount, UINT32 Data) {
    Header = PD_TX_BUF;
    Fixed = &PD_TX_BUF[2];

    HeaderAutoFill();
    Header->Struct.NDO = 1;
    Header->Struct.MsgType = MsgType;
    Fixed->Data = Swap32(Data);

    PD_PHY_Stat.RetryCnt = RetryCount;
    PD_PHY_Stat.MsgCnt = 4;
    PD_PHY_Stat.MsgLen = 6;
}

static xdata UINT32 response[4];
static void SendVDM(UINT8 RetryCount, UINT8 ObjectCount, UINT32 *Objects) {
    UINT8 index;

    Header = PD_TX_BUF;
	Fixed = &PD_TX_BUF[2];
    HeaderAutoFill();

    Header->Struct.NDO = ObjectCount;
    Header->Struct.MsgType = VDef;

    for (index = 0; index < ObjectCount; index++) {
        (&Fixed->Data)[index] = Swap32(Objects[index]);
    }

    PD_PHY_Stat.RetryCnt = RetryCount;
    PD_PHY_Stat.MsgCnt = 4;
    PD_PHY_Stat.MsgLen = 2 + ObjectCount * 4;
}

static xdata UINT8 dp_pin_assignment;


	// 局部变量
	xdata UINT32 data2, data6;
	xdata UINT16 voltage, current;
	xdata UINT8 pdo_type;
	xdata UINT16 svid;
    xdata UINT8 command;
    xdata UINT8 mode_index;

void PD_PROT_HANDLE( void ) {

	PD_PHY_Stat.MsgCnt = 0;
	Header = PD_RX_BUF;
	Fixed = &PD_RX_BUF[2];
	
	switch (PD_PHY_Stat.status) {
		// case PD_ATTACHED:
		// 	SendCtrlMsg(GetSrcCap, 3);
		// 	PD_PHY_Stat.status = PD_WAIT_SOURCE_CAP;
		// 	return;
		case PD_DP_ATTENTION:
			response[0] = VDM_HDR(VDM_SVID_DP, VDM_CMDT_INIT, VDM_ATTENTION);
			response[1] = DP_STATUS_CONNECTION_UFP_D | DP_STATUS_ENABLED |
                  DP_STATUS_PREFER_MULTI_FUNC | DP_STATUS_HPD_IRQ;
			if (DP_GetHPD()) response[1] |= DP_ATTN_HPD_STATE;
			// response[1] |= DP_ATTN_HPD_STATE;
			SendVDM(3, 2, response);
			PD_PHY_Stat.status = PD_DP_ACTIVE;
			debug_printf("[snk] active, HPD: %d\n", (int)DP_GetHPD());
			return;
		case PD_DP_ACTIVE:
			return;
		default:
			break;
	}

	debug_printf("[snk] NDO: %d, msgtype: %x\n", (int)Header->Struct.NDO, (int)Header->Struct.MsgType);
	if( Header->Struct.Extended == 1 ) {
		// Extended Message
		return;
    }

	if( Header->Struct.NDO ) {
        /* Data Message */
		switch( Header->Struct.MsgType ) {
			case SRCCap:
				data2 = Swap32(Fixed->Data);
				voltage = ((data2 >> 10) & 0x3FF) * 50;
				current = (data2 & 0x3FF) * 10;
				pdo_type = (UINT8)((data2 >> 30) & 0x03);
				debug_printf("[sink] voltage: %d mV, current: %d mA, pdo_type: %d\n", (int)voltage, (int)current, (int)pdo_type);

				// 认为是 Fixed Supply PD
				SendDataMsg(Request, 3, (1UL << 28) | (20 << 10) | (current / 10) );
				break;
			case VDef:
			 	data2 = Swap32(Fixed->Data);
				if (!(data2 & VDM_STRUCTURED) || (data2 & VDM_CMDT_MASK) != VDM_CMDT_INIT) {
					break;
				}

				svid = (UINT16)(data2 >> 16);
				command = (UINT8)(data2 & VDM_CMD_MASK);
				mode_index = (UINT8)((data2 >> 8) & 0x07);

				if (/*svid == VDM_SVID_DISCOVERY &&*/ command == VDM_DISC_ID) {
					response[0] = VDM_HDR(svid, VDM_CMDT_ACK, command);
					response[1] = 0x1A860000UL;
					response[2] = 0x00000000UL;
					response[3] = 0x00000000UL;
					// FIXME: 超过3个会栈溢出
					SendVDM(3, 3, response);
					break;
				}

				if (/*svid == VDM_SVID_DISCOVERY &&*/ command == VDM_DISC_SVID) {
					response[0] = VDM_HDR(svid, VDM_CMDT_ACK, command);
					response[1] = VDM_SVID_LIST_DP;
					SendVDM(3, 2, response);
					break;
				}

				if (svid == VDM_SVID_DP && command == VDM_DISC_MODE) {
					response[0] = VDM_HDR(svid, VDM_CMDT_ACK, command);
					response[1] = DP_MODE_UFP_D_USB2_USB3;
					SendVDM(3, 2, response);
					break;
				}

				if (svid == VDM_SVID_DP && command == VDM_ENTER_MODE) {
					debug_printf("DP Enter Mode, mode_index=%u\n", (UINT16)mode_index);
					response[0] = VDM_HDR(svid, VDM_CMDT_ACK, command);
					SendVDM(3, 1, response);
					break;
				}

				if (svid == VDM_SVID_DP && command == VDM_DP_STATUS) {
					response[0] = VDM_HDR(svid, VDM_CMDT_ACK, command);
					response[1] = DP_STATUS_CONNECTION_UFP_D | DP_STATUS_ENABLED |
							DP_STATUS_PREFER_MULTI_FUNC;
					if (DP_GetHPD()) response[1] |= DP_STATUS_HPD_HIGH;
					SendVDM(3, 2, response);
					break;
				}

				if (svid == VDM_SVID_DP && command == VDM_DP_CONFIG) {
					data6 = Swap32((&Fixed->Data)[1]);

					if ((data6 & DP_CONFIG_SIGNALING) == 0 ||
						(data6 & 0x03UL) != DP_CONFIG_UFP_U_AS_UFP_D) {
						response[0] = VDM_HDR(svid, VDM_CMDT_NAK, command);
						SendVDM(3, 1, response);
						break;
					}

					dp_pin_assignment = (UINT8)((data6 & DP_CONFIG_PIN_MASK) >> 8);
					response[0] = VDM_HDR(svid, VDM_CMDT_ACK, command);

					if (dp_pin_assignment & (DP_PIN_C | DP_PIN_D)) {
						CH9445F_RequestMode(CH9445F_MODE_DP2, cc_flip);
					} else if (dp_pin_assignment & (DP_PIN_A | DP_PIN_B)) {
						CH9445F_RequestMode(CH9445F_MODE_DP4, cc_flip);
					} else {
						response[0] = VDM_HDR(svid, VDM_CMDT_NAK, command);
						SendVDM(3, 1, response);
						break;
					}

					PD_PHY_Stat.status = PD_DP_ATTENTION;
					SendVDM(3, 1, response);
					break;
				}

				response[0] = VDM_HDR(svid, VDM_CMDT_NAK, command);
				SendVDM(3, 1, response);
				break;
			
			default:
				break;
		}
	} else {
        /* Control Message */
		switch (Header->Struct.MsgType) {
			case GetSnkCap:
				SendDataMsg(SinkCap, 3, (100UL << 10) | 300UL);
				break;
			default:
				break;
		}
	}
}

static bit IsVDMAck(UINT16 SVID, UINT8 Command) {
    UINT32 vdm;
    if (Header->Struct.MsgType != VDef) return 0;
    if (Header->Struct.NDO < 1) return 0;
    vdm = Swap32(Fixed->Data);
    if (!(vdm & VDM_STRUCTURED)) return 0;
    if ((UINT16)(vdm >> 16) != SVID) return 0;
    if ((vdm & VDM_CMDT_MASK) != VDM_CMDT_ACK) return 0;
    if ((vdm & VDM_CMD_MASK) != Command) return 0;
    return 1;
}

static UINT8 DP_CMD() {
    UINT32 vdm;
    if (Header->Struct.MsgType != VDef) return -1;
    if (Header->Struct.NDO < 2) return -1;
    vdm = Swap32(Fixed->Data);
    if (!(vdm & VDM_STRUCTURED)) return -1;
    if ((UINT16)(vdm >> 16) != VDM_SVID_DP) return -1;
    if ((vdm & VDM_CMDT_MASK) != VDM_CMDT_INIT) return -1;
    return (UINT8)(vdm & VDM_CMD_MASK);
}

static xdata UINT8 selected_dp_pos;
static xdata UINT8 selected_dp_pin;

static UINT8 SelectDPPin(UINT32 mode_vdo) {
    xdata UINT8 partner_pin;
    xdata UINT8 common_pin;
    partner_pin = (UINT8)((mode_vdo >> 8) & 0x3F);
    common_pin = partner_pin & LOCAL_DP_PIN_MASK;
    if (common_pin & (DP_PIN_A | DP_PIN_B)) {
        return common_pin & (DP_PIN_A | DP_PIN_B);
    }
    if (common_pin & (DP_PIN_C | DP_PIN_D)) {
        return common_pin & (DP_PIN_C | DP_PIN_D);
    }
    return 0;
}


	// 局部变量
	xdata UINT8 pdo_pos;
	xdata UINT16 ops_current, max_current;
	xdata int i;
	xdata UINT32 mode_vdo;

	xdata UINT8 old_status;
	xdata UINT8 old_ndo, old_msgtype;

void PD_PROT_HANDLE_SRC() {


	PD_PHY_Stat.MsgCnt = 0;
	Header = PD_RX_BUF;
	Fixed = &PD_RX_BUF[2];
	old_ndo = Header->Struct.NDO;
	old_msgtype = Header->Struct.MsgType;
	old_status = PD_PHY_Stat.status;

	switch (PD_PHY_Stat.status) {
		case PD_DISCONNECTED: {
			printf("error: status disconnected\n");
		} break;
		case PD_ATTACHED: {
			/* 5 V / 3 A Fixed PDO */
            SendDataMsg(SRCCap, 3, (0 << 28) | (100UL << 10) | 300);
			PD_PHY_Stat.status = PD_WAIT_REQUEST;
		} break;
		case PD_WAIT_REQUEST: {
			if (Header->Struct.MsgType == Request) {
				data2 = Swap32(Fixed->Data);
				pdo_pos = (UINT8)((data2 >> 28) & 0x07);
				ops_current = (UINT16)((data2 >> 10) & 0x3FF);
				max_current = (UINT16)(data2 & 0x3FF);
				debug_printf("PDO: %u, OpsCurrent: %u mA, MaxCurrent: %u mA\n", (int)(pdo_pos), (int)(ops_current), (int)(max_current));
				if (pdo_pos == 1 && ops_current <= max_current && max_current <= 3000) {
					SendCtrlMsg(Accept, 1);
					PD_PHY_Stat.status = PD_WAIT_ACCEPT;
					break;
				}
			}
			SendCtrlMsg(Reject, 1);
			PD_PHY_Stat.status = PD_WAIT_REQUEST;
		} break;
		case PD_WAIT_ACCEPT: {
			SendCtrlMsg(PS_RDY, 1);
			PD_PHY_Stat.status = PD_WAIT_PS_RDY;
		} break;
		case PD_WAIT_PS_RDY: {
			SendDataMsg(VDef, 3, VDM_HDR(VDM_SVID_STD, VDM_CMDT_INIT, VDM_DISC_ID));
			PD_PHY_Stat.status = PD_DISCOVER_IDENTITY;
		} break;
		case PD_DISCOVER_IDENTITY: {
			if (IsVDMAck(VDM_SVID_STD, VDM_DISC_ID)) {
				SendDataMsg(VDef, 3, VDM_HDR(VDM_SVID_STD, VDM_CMDT_INIT, VDM_DISC_SVID));
				PD_PHY_Stat.status = PD_DISCOVER_SVIDS;
			} 
		} break;
		case PD_DISCOVER_SVIDS: {
			if (IsVDMAck(VDM_SVID_STD, VDM_DISC_SVID)) {
				SendDataMsg(VDef, 3, VDM_HDR(VDM_SVID_DP, VDM_CMDT_INIT, VDM_DISC_MODE));
				PD_PHY_Stat.status = PD_DISCOVER_MODES;
			} 
		} break;
		case PD_DISCOVER_MODES: {
			if (IsVDMAck(VDM_SVID_DP, VDM_DISC_MODE)) {
				selected_dp_pin = 0;
				for (i = 0; i < Header->Struct.NDO - 1; i++) {
					mode_vdo = Swap32((&Fixed->Data)[i+1]);
					selected_dp_pin = SelectDPPin(mode_vdo);
					debug_printf("mode %d: vdo=0x%08x, sel_pin=0x%02x\n", i + 1, (int)mode_vdo, (int)selected_dp_pin);
					if (selected_dp_pin != 0) {
						selected_dp_pos = i + 1;
						break;
					}
				}
				if (selected_dp_pin != 0) {
					SendDataMsg(VDef, 3, VDM_HDR(VDM_SVID_DP, VDM_CMDT_INIT, VDM_ENTER_MODE) |
						((UINT32)selected_dp_pos << 8) );
				}
				PD_PHY_Stat.status = PD_ENTER_DP_MODE;
			} 
		} break;
		case PD_ENTER_DP_MODE: {
			if (IsVDMAck(VDM_SVID_DP, VDM_ENTER_MODE)) {
				response[0] = VDM_HDR(VDM_SVID_DP, VDM_CMDT_INIT, VDM_DP_STATUS);
				response[1] = DP_STATUS_CONNECTION_DFP_D;
				SendVDM(3, 2, response);
				PD_PHY_Stat.status = PD_DP_STATUS;
			}
		} break;
		case PD_DP_STATUS: {
			if (IsVDMAck(VDM_SVID_DP, VDM_DP_STATUS)) {
				data6 = Swap32((&Fixed->Data)[1]);
				DP_SetHPD(data6 & DP_STATUS_HPD_HIGH);
				response[0] = VDM_HDR(VDM_SVID_DP, VDM_CMDT_INIT, VDM_DP_CONFIG);
				response[1] = DP_CONFIG_UFP_U_AS_UFP_D | DP_CONFIG_SIGNALING | ((UINT32)selected_dp_pin << 8);
				SendVDM(3, 2, response);
				PD_PHY_Stat.status = PD_DP_CONFIGURE;
			} 
		} break;
		case PD_DP_CONFIGURE: {
			if (IsVDMAck(VDM_SVID_DP, VDM_DP_CONFIG)) {
				if (selected_dp_pin & (DP_PIN_C | DP_PIN_D)) {
					CH9445F_RequestMode(CH9445F_MODE_DP2, cc_flip);
				} else if (selected_dp_pin & (DP_PIN_A | DP_PIN_B)) {
					CH9445F_RequestMode(CH9445F_MODE_DP4, cc_flip);
				}
				PD_PHY_Stat.status = PD_DP_ACTIVE;
				debug_printf("[src] active\n");
			} 
		} break;
		case PD_DP_ACTIVE: {
			if (DP_CMD() == VDM_ATTENTION) {
				data6 = Swap32((&Fixed->Data)[1]);
				DP_SetHPD(data6 & DP_ATTN_HPD_STATE);
				debug_printf("DP Attention: HPD=%u, IRQ=%u\n", (int)(data6 & DP_ATTN_HPD_STATE), (int)((data6 & DP_ATTN_HPD_IRQ) != 0));
			}
		} break;
		default: {
			debug_printf("error: unknown status %u\n", (int)PD_PHY_Stat.status);
		} break;
	}

	// if (old_status != PD_PHY_Stat.status) {
	debug_printf("[src] NDO: %d, msgtype: %x\n", (int)old_ndo, (int)old_msgtype);
	debug_printf("[src] %u -> %u\n", (int)old_status, (int)PD_PHY_Stat.status);
}