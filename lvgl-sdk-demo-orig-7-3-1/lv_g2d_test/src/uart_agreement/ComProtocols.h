/*
 * @Description: In User Settings Edit
 * @Author: your name
 * @Date: 2019-09-24 15:04:28
 * @LastEditTime: 2019-09-28 11:54:15
 * @LastEditors: Please set LastEditors
 */
#ifndef __COMPORTOCOLS_H
#define __COMPORTOCOLS_H

 //  �װ���360ģ��ͨ�ŵ�Э��ʵ��
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "uart.h"

#include "../ref_inc/osport.h" 

#ifndef WIN32

enum {
	ID_CP_GP2 = 0,          // com protocol id.
	ID_CP_SW = 1,
	ID_CP_GP1 = 2,
	ID_CP_GP3 = 3,
	ID_CP_RTS = 4,
	ID_CP_IR = 5,
	ID_CP_YXCX = 6,
	ID_CP_GP2_TS_TIME_VOICE = 7,
	ID_CP_GP2_TRAIL = 8,
	ID_CP_LAMP = 9,
	ID_CP_LD_Korea_TOUCHU = 10,
	ID_CP_XCP,

	ID_CP_NONE
};

typedef struct _Radar_t {
	uint8_t RadarMode;

	uint8_t RadarFront_2 : 4; //ǰ����̽�е�����̽ͷ
	uint8_t RadarFront_1 : 4; //ǰ����̽�е����̽ͷ

	uint8_t RadarFront_4 : 4; //ǰ����̽�е��ұ�̽ͷ
	uint8_t RadarFront_3 : 4; //ǰ����̽�е�����̽ͷ

	uint8_t RadarRear_2 : 4; //������̽�е����̽ͷ
	uint8_t RadarRear_1 : 4; //������̽�е�����̽ͷ
	uint8_t RadarRear_4 : 4; //������̽�е�����̽ͷ
	uint8_t RadarRear_3 : 4; //������̽�е��ұ�̽ͷ

	uint8_t RadarLeft_2 : 4; //������̽�е�1̽ͷ
	uint8_t RadarLeft_1 : 4; //������̽�е�2̽ͷ
	uint8_t RadarLeft_4 : 4; //������̽�е�3̽ͷ
	uint8_t RadarLeft_3 : 4; //������̽�е�4̽ͷ

	uint8_t RadarRight_2 : 4; //������̽�е�1̽ͷ
	uint8_t RadarRight_1 : 4; //������̽�е�2̽ͷ
	uint8_t RadarRight_4 : 4; //������̽�е�3̽ͷ
	uint8_t RadarRight_3 : 4; //������̽�е�4̽ͷ
} Radar_t;

typedef struct _Radar6644_t {
	uint8_t RadarFront[6];
	uint8_t RadarRear[6];
	uint8_t RadarLeft[4];
	uint8_t RadarRight[4];
} Radar6644_t;

typedef struct __CarInfo
{
	uint8_t gear : 4;
	uint8_t acc : 1;
	uint8_t autopark : 1;
	uint8_t cansleep : 1;
	uint8_t accValid : 1;
	uint8_t lightstatus;
	uint8_t doorstatus;
	uint8_t speed;
	uint32_t corner;
	Radar_t Radar;
	Radar6644_t Radar6644;
} CarInfo_t;

#define IOCTRL_MASK_BIT     ( 1 << 0 )
#define CAN_MASK_BIT        ( 1 << 1 )
#define COM1_MASK_BIT       ( 1 << 2 )
#define COM2_MASK_BIT       ( 1 << 3 )

typedef struct __CarInfoValidFlags
{   // ���ȼ�˳�� IO->COM1->COM2.    bit0:io��Ч��bit1��COM1��Ч��bit2: COM2��Ч
	uint8_t accFlag;
	uint8_t gearFlag;
	uint8_t lightFlag;
	uint8_t doorFlag;
	uint8_t speedFlag;
	uint8_t connerFlag;
	uint8_t radarFlag;
	uint8_t radarFrontFlag;
	uint8_t radarRearFlag;
	uint8_t radarLeftFlag;
	uint8_t radarRightFlag;
} CarInfoValidFlags_t;

typedef struct SW_SendFrame {
	uint8_t head[2];
	uint8_t dir;
	uint8_t len;
	uint8_t cmd;
	uint8_t buf[256 + 1];
} SW_CmdFrame_t;

typedef struct SW_AskFrame {
	uint8_t head[2];
	uint8_t dir;
	uint8_t len;
	uint8_t cmd;
	uint8_t status;
	uint8_t buf[256 + 1];
} SW_AskFrame_t;

typedef struct _GP2Frame {
	uint8_t head;
	uint8_t cmd;
	uint8_t len;
	uint8_t buf[256 + 1];
} GP2Frame_t;

typedef struct _XCPFrame {
	uint16_t head;
	uint8_t cmd;
	uint8_t len;
	uint8_t buf[256];
} XCPFrame_t;

typedef struct _GP1Frame {
	uint8_t head;
	uint8_t len;
	uint8_t cmd;
	uint8_t buf[256 + 1];
} GP1Frame_t;

typedef struct __DispParam {
	uint32_t pclk;
	uint16_t x;
	uint16_t y;
	uint16_t ht;
	uint16_t hbp;
	uint16_t hfp;
	uint16_t hspw;
	uint16_t vt;
	uint16_t vbp;
	uint16_t vfp;
	uint8_t vspw;
	uint8_t hpolarity;
	uint8_t vpolarity;
	uint8_t clk_phase;
	uint8_t dispmode;           // 0-VGA, 1-HDMI, 2-AHD, 3-AV
	uint16_t rotate;
	uint8_t lvds_link;          // 0-channel 0, 1-channel 1, 2-split
	uint8_t lvds_mode;          // 0-NS, 1-JEDIA
} DispParam_t;

enum dispmode {
	Mode_VGA = 0,
	Mode_HDMI,
	Mode_AHD,
	Mode_AV,
	Mode_LVDS,
	Mode_Preset_CVBS_NTSC = 0x80,
	Mode_Preset_CVBS_PAL,
	Mode_Preset_AHD720P25Hz,
	Mode_Preset_AHD720P30Hz,
	Mode_Preset_AHD1080P25Hz,
	Mode_Preset_AHD1080P30Hz,
	Mode_Preset_TVI720P25Hz,
	Mode_Preset_TVI720P30Hz,
	Mode_Preset_TVI1080P25Hz,
	Mode_Preset_TVI1080P30Hz,
	Mode_Preset_HDMI_CADILLAC,
	Mode_Preset_BT656_CADILLAC,

	Mode_BAGOO_RGB888 = 0,
	Mode_BAGOO_LVDS,
	Mode_BAGOO_HDMI,
	Mode_BAGOO_AV,
	Mode_BAGOO_VGA,
	Mode_BAGOO_AHD,
	Mode_BAGOO_MIPI,

	Mode_OTHER_RGB = 0,
	Mode_OTHER_LVDS,
	Mode_OTHER_HDMI,
	Mode_OTHER_AV,
	Mode_OTHER_VGA,
	Mode_OTHER_AHD,
	Mode_OTHER_MIPI,
	Mode_OTHER_LVDS_SPLIT,
};

extern CarInfo_t Com1CarInfo;
extern CarInfo_t Com2CarInfo;
extern CarInfo_t CanCarInfo;
extern uint8_t Com1CarSpeedLast;
extern uint8_t Com2CarSpeedLast;
extern uint8_t CanCarSpeedLast;

extern unsigned char crc7(unsigned char ucOri, unsigned char* pucData, unsigned int uiLen);

int32_t ProtocolPackSend_SW(Uart& ExtCom, uint8_t ucCmd, uint8_t* pucData, uint8_t ucDataLen, uint8_t ucStatus, uint8_t isAsk);
int32_t ProtocolPackSend_SW(CMyQueue& msgQueueSend, uint8_t ucCmd, uint8_t* pucData, uint8_t ucDataLen, uint8_t ucStatus, uint8_t isAsk);
int32_t ComProtocolSendCmd(Uart& ExtCom, uint8_t ucCmd, uint8_t* pucData, uint8_t ucDataLen);
int32_t ComProtocolSendCmd(CMyQueue& msgQueueSend, uint8_t ucCmd, uint8_t* pucData, uint8_t ucDataLen);
int32_t ComProtocolSendAsk(Uart& ExtCom, uint8_t ucCmd, uint8_t* pucData, uint8_t ucDataLen, uint8_t ucStatus);
int32_t ProtocolPackSend_GP2(Uart& ExtCom, uint8_t ucCmd, uint8_t* pucData, uint8_t ucDataLen);

extern int32_t ComRecvCmd_SW(Uart& ExtCom, uint8_t* pCmd, uint8_t* pData, uint8_t* pLen, int32_t iTimeout);
extern int32_t ComRecvCmd_SW(CMyQueue& msgQueueRecv, uint8_t* pCmd, uint8_t* pData, uint8_t* pLen, int32_t iTimeout);
extern int32_t ComRecvCmd_GP2(Uart& ExtCom, uint8_t* pucCmd, uint8_t* pucData, uint8_t* pucDatalen, int32_t iTimeout, bool bDebug = false, uint8_t* pBufRaw = nullptr);
extern int32_t ComRecvCmd_XCP(Uart& ExtCom, uint8_t* pucCmd, uint8_t* pucData, uint8_t* pucDatalen, int32_t iTimeout, bool bDebug = false, uint8_t* pBufRaw = nullptr);
extern int32_t ComRecvCmd_GP1(Uart& ExtCom, uint8_t* pucCmd, uint8_t* pucData, uint8_t* pucDatalen, int32_t iTimeout);
extern int32_t IRRecvCmd_NEC(IrRx& IrDev, uint8_t* pucCmd, uint8_t* pucData, uint8_t* pucDatalen, int32_t iTimeout);
extern int32_t IRRecvCmd_NEC_SW(IrRx& IrDev, uint8_t* pucCmd, uint8_t* pucData, uint8_t* pucDatalen, int32_t iTimeout);
extern int32_t ComRecvAsk(CMyQueue& msgQueueRecv, uint8_t* pCmd, uint8_t* pData, uint8_t* pucLen, uint8_t* pucStatus, int32_t iTimeout);
int32_t ComRecvCmd_KoreaTouch(Uart& ExtCom, uint8_t* pucCmd, uint8_t* pucData, uint8_t* pucDatalen, int32_t iTimeout);

#endif

#endif



