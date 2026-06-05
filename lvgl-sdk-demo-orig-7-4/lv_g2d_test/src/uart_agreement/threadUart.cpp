#include "threadUart.h"

#include <stdio.h>
#include <string.h>

#include "./control/com_defs.h"
#include "crc.h"
// #include "crp.h"
#include "CSystemConf.h"
#include "CSetings.h"
#include "CayenneCarData.h"
#include "CarStateMonitor.h"
#include "CMyFile.h"
#include "ExternalIoCtrl.h"
#include "uart_agreement_c_api.h"
#include "./tool/util.h"
#include "ComProtocols.h"
#include "core/bv_types.h"
CayenneCarData GCarData;
CayenneCarData GCarDataLast;

ComProt GsComProt(CP_DEVICE_ID_360, CP_DEVICE_ID_BASE);

#ifndef WIN32
CMyQueue AvmMsgQueueRx("AvmMsgQueueRx", sizeof(SW_AskFrame_t), 32);
CMyQueue AvmMsgQueueTx("AvmMsgQueueTx", sizeof(SW_AskFrame_t), 32);
static  RemoteControl RmtCtl;
#endif 

// #ifndef WIN32
#if 1
Uart AvmComToMcu("/dev/ttySx", 19200, 8, 1, 'n');
Uart AvmCom1("/dev/ttyS5", 19200, 8, 1, 'n');
// Uart AvmCom1("/dev/ttyS2", 115200, 8, 1, 'n');
// Uart AvmCom2("/dev/ttyS4", 19200, 8, 1, 'n');
Uart AvmCom2("/dev/ttyS4", 19200, 8, 1, 'n');
IrRx AvmComIr("/dev/input/event3");
IrRx AvmIr("/dev/input/event3");
#endif
static bool GMcuPortConnected = false;

#include <sys/types.h>
#include <sys/stat.h>

int fun_socket_tx(unsigned char* pucBuf, int len, int flag, void* pvData, int iTimeOut)
{
	ProcessComBase* p = (ProcessComBase*)pvData;
	if (!p) {
		printf("<WARNING> %s: pvData is NULL\r\n", __FUNCTION__);
		return -1;
	}
	return p->WriteCom(pucBuf, len);
}

int fun_socket_rx(unsigned char* pucBuf, int len /*  rx 时 len 无意义 */, int flag, void* pvData, int iTimeOut)
{
	ProcessComBase* p = (ProcessComBase*)pvData;
	if (!p) {
		printf("<WARNING> %s: pvData is NULL\r\n", __FUNCTION__);
		return -1;
	}
	pucBuf[0] = pucBuf[1] = 0;
	int iRt = p->ReadCom(pucBuf, 256);         //  对 socketpair，内部是按帧方式存储的，若读的长度小于实际长度，则未读的会被丢弃，若指定读的长度大小实际长度，则只读出实际长度，因此此处直接输入参数读256字节
	if (iRt < 0) {
		//printf("%s:\r\n", p->m_cName);
		//perror("socket_rx:");
		return iRt;
	}
	if (pucBuf[0] != 'S' || pucBuf[1] != 'W') {
		printf("<WARNING> %s , %s, iRt = %d, ( the read data is not SW )\r\n", __FUNCTION__, p->m_cName, iRt);
		return -1;
	}
	return iRt;
}

int fun_msgqueue_tx(unsigned char* pucBuf, int len, int flag, void* pvData, int iTimeOut)
{
	unsigned char ucBuf[256];
	//printf(">>>>>>>>>>%s,len=%d.\r\n",__FUNCTION__, len);
	memcpy(ucBuf, pucBuf, len);
	return AvmMsgQueueTx.PostMsg(ucBuf);
}

int fun_msgqueue_rx(unsigned char* pucBuf, int len, int flag, void* pvData, int iTimeOut)
{
	unsigned char ucBuf[256];
	int iRet = -1;
	int dataLen = 0;
	iRet = AvmMsgQueueRx.RecvMsg(ucBuf, iTimeOut);
	if (iRet >= 0) {
		if (ucBuf[0] == 'S' && ucBuf[1] == 'W') {
			dataLen = 4 + ucBuf[3] + 1;
			memcpy(pucBuf, ucBuf, dataLen);
			return dataLen;
		}
	}
	return iRet;
}

void InitComPort(void)
{
	// GsComPortPlay.RegisterTxRxFuc(fun_socket_tx, fun_socket_rx, &ProcessComPlay);
	// GsComPortCarLife2BvThread.RegisterTxRxFuc(fun_socket_tx, fun_socket_rx, &ProcessComCarLife2BvThread);

	GsComProt.RegisterTxRxFuc(fun_msgqueue_tx, fun_msgqueue_rx, NULL);
}

#define COM1        (1)
#define COM2        (2)
#define COM3        (3)
typedef struct __ComMUXInfo {
	uint8_t validFlag;
	uint8_t comId;
	uint8_t protId;
	uint8_t res;
} ComMUXInfo_t;

ComMUXInfo_t comMUXInfo = { 0,0,0,0 };
MyCritialData<ComMUXInfo_t> ComMUXInfo(comMUXInfo);

int threadAvmCom1(void* p) {
	uint8_t ucBuf[256];
	uint8_t ucLstPos[4];
	uint8_t ucCmd, ucLen;
	uint32_t protID = GCSetings.GetFirstPort();
	printf("[threadUart.c -> %d] protID = %d\n", __LINE__, protID);

	CarInfo_t* pCarInfo = (CarInfo_t*)ucBuf;
	static uint32_t TestFlag = 0;
	// INICANData cmdfilter, cmdfilterLast;

	bool bUsingRadar6644 = false;
	bool bRadar3D = GCSetings.GetRadar3D();
	if (protID == ID_CP_IR) {
		// Open IR interface
	}
	else if (protID == ID_CP_GP1) {
		if (AvmCom1.Open(38400) != 0) {
			printf("AvmCom1 open failed!\n");
			return -1;
		}
	}
	else {
		if (AvmCom1.Open() != 0) {
			printf("AvmCom1 open failed!\n");
			return -1;
		}
	}

	uint32_t RequestDispParamSucess = 0;
	DWORD RequestDispParamTickCnt = GetTickCount();
	DWORD RequestBaboogDispParamTickCnt = RequestDispParamTickCnt - 1000;
	uint32_t RequestDispParamCnt = 0;
	DWORD RequestCarDateTickCnt = RequestDispParamTickCnt - 3000;
	uint32_t RequestCarDateCnt = 0;
	DWORD RequestDecoderVerTickCnt = RequestDispParamTickCnt - 2000;
	uint32_t RequestDecoderVerCnt = 0;
	DWORD HeartBeatTickCnt = RequestDispParamTickCnt;
	bool DecVerProfile = false;
	auto decInfoRequestRestart = [&] {
		RequestDispParamTickCnt = GetTickCount();
		RequestBaboogDispParamTickCnt = RequestDispParamTickCnt - 1000;
		RequestDispParamCnt = 0;
		RequestCarDateTickCnt = RequestDispParamTickCnt - 3000;
		RequestCarDateCnt = 0;
		RequestDecoderVerTickCnt = RequestDispParamTickCnt - 2000;
		RequestDecoderVerCnt = 0;
		HeartBeatTickCnt = RequestDispParamTickCnt;
	};
	while (1) {
		uint32_t curPortID = GCSetings.GetFirstPort();
		if (curPortID != protID) {
			printf("%s: curID = %d , newID = %d\n", __FUNCTION__, protID, curPortID);

			AvmCom1.Close();
			util::msleep(500);
			protID = curPortID;

			switch (protID) {
			case ID_CP_GP1:
				AvmCom1.Open(38400);
				break;
			default:
				AvmCom1.Open(19200);
				break;
			}
			util::msleep(1000);
		}
		if ((protID == ID_CP_GP2) && (RequestDispParamSucess == 0) && (RequestDispParamCnt < 10)) {
			// For Bagoo, start
			if (GetTickCount() - RequestBaboogDispParamTickCnt > 4000) {
				ucBuf[0] = 0x0B;
				ucBuf[1] = 0x0D;    // siwei
				ucBuf[2] = 0x00;    // 0x00: T5; 0x01:T7
				ProtocolPackSend_GP2(AvmCom1, 0xf1, ucBuf, 0x03);
				util::msleep(10);
				ProtocolPackSend_GP2(AvmCom1, 0xf1, ucBuf, 0x01);       // for 车畅芯
				RequestBaboogDispParamTickCnt = GetTickCount();
			}
			// For Bagoo, end
			if (GetTickCount() - RequestDispParamTickCnt > 4000) {
				/*	畅芯解码器，在收到0xfd后又收到0x30请求，会回复两个命令的屏参，因0xfd请求长度0x1b不带相位，0x30回复的屏参命令又带相位
					两个屏参不一致，使得核心板反复重启，为此调换0x30、0xfd请求顺序，畅芯解码器收到0x30后又收到0xfd请求，只会回复0x30屏参
				*/
				memset(ucBuf, 0x00, sizeof(ucBuf));
				ucBuf[0] = 0x01;
				ProtocolPackSend_GP2(AvmCom1, 0x30, ucBuf, 1);          // for 车畅芯
				util::msleep(10);
				memset(ucBuf, 0x00, sizeof(ucBuf));
				ProtocolPackSend_GP2(AvmCom1, 0xfd, ucBuf, 0x1b);
				RequestDispParamTickCnt = GetTickCount();
				RequestDispParamCnt++;
			}
		}
		if ((protID == ID_CP_GP2) && (RequestDecoderVerCnt < 10)) {
			if (GetTickCount() - RequestDecoderVerTickCnt > 4000) {
				ucBuf[0] = 0x71;
				ucBuf[1] = 0x0D;    // siwei
				ucBuf[2] = 0x00;    // 0x00: T5; 0x01:T7
				ProtocolPackSend_GP2(AvmCom1, 0xf1, ucBuf, 0x03);
				RequestDecoderVerTickCnt = GetTickCount();
				RequestDecoderVerCnt++;
				util::msleep(10);
				ProtocolPackSend_GP2(AvmCom1, 0xf1, ucBuf, 0x01);
				// 悦驾底板
				util::msleep(10);
				ucBuf[0] = 0xFC;
				ucBuf[1] = 0x08;    // siwei
				ucBuf[2] = 0x00;    // 0x00: T5; 0x01:T7
				ProtocolPackSend_GP2(AvmCom1, 0xf1, ucBuf, 0x03);
				RequestDecoderVerTickCnt = GetTickCount();
			}
		}
		if ((protID == ID_CP_GP2) && (RequestCarDateCnt < (30 * 60 / 4))) { // 30 minutes
			if (GetTickCount() - RequestCarDateTickCnt > 4000) {
				ucBuf[0] = 0x04;
				ProtocolPackSend_GP2(AvmCom1, 0xf1, ucBuf, 0x1);
				RequestCarDateTickCnt = GetTickCount();
				RequestCarDateCnt++;
			}
		}
		if ((protID == ID_CP_GP2) && (GetTickCount() - HeartBeatTickCnt > 500)) {
			HeartBeatTickCnt = GetTickCount();
					ucBuf[0] = 0x1;
			}
			else {
				ucBuf[0] = 0;
			}
			ucBuf[1] = 0xFF;
			ProtocolPackSend_GP2(AvmCom1, 0x92, ucBuf, 0x02);
		int32_t iRet = 0;
		memset(ucBuf, 0, sizeof(ucBuf));
		if ((TestFlag != 0) && (protID == ID_CP_GP2 || protID == ID_CP_SW)) {
			iRet = ComRecvCmd_SW(AvmCom1, &ucCmd, ucBuf, &ucLen, 100);
		}
		else {
			switch (protID) {
			case ID_CP_SW:
				iRet = ComRecvCmd_SW(AvmCom1, &ucCmd, ucBuf, &ucLen, 100);
				break;
			case ID_CP_GP1:
				iRet = ComRecvCmd_GP1(AvmCom1, &ucCmd, ucBuf, &ucLen, 100);
				break;
			case ID_CP_IR:
				sleep(100);      // IR receive
				iRet = -1;
				break;
			default:
				// printf("[threadUart.c -> %d] protID = %d\n", __LINE__, protID); // 目前走这里
				iRet = ComRecvCmd_GP2(AvmCom1, &ucCmd, ucBuf, &ucLen, 100);
				break;
			}
		}
		if (iRet != 0) {
			continue;
		}
		switch (ucCmd) {
		case CP_CMD_CARDATA:
			if (ID_CP_GP2_TS_TIME_VOICE == protID) {
				break;
			}
			if (ID_CP_GP2_TRAIL == protID) {
				if (/*cmdfilter.track*/0) {
					int corner = ((pCarInfo->corner) << 24) | ((pCarInfo->corner & 0x0000ff00) << 8) |
						((pCarInfo->corner & 0x00ff0000) >> 8) | ((pCarInfo->corner) >> 24);
					unsigned int uCorner = -corner;
					Com1CarInfo.corner = ((uCorner) << 24) | ((uCorner & 0x0000ff00) << 8) | ((uCorner & 0x00ff0000) >> 8) | ((uCorner) >> 24);
				}
				else {
					Com1CarInfo.corner = pCarInfo->corner;
				}
			}
			else {
				if (/*cmdfilter.acc*/1) {
					Com1CarInfo.accValid = pCarInfo->accValid;
					Com1CarInfo.acc = pCarInfo->acc;
				}
				else {
					Com1CarInfo.accValid = 0;
					Com1CarInfo.acc = 0;
				}
				if (/*cmdfilter.autopark*/1) {
					Com1CarInfo.autopark = pCarInfo->autopark;
				}
				else {
					Com1CarInfo.autopark = 0;
				}
				Com1CarInfo.lightstatus &= (~0x03);
				if (/*cmdfilter.led*/1) {
					Com1CarInfo.lightstatus |= (pCarInfo->lightstatus & 0x03);
				}
				Com1CarInfo.lightstatus &= (~0x04);
				if (/*cmdfilter.doubleled*/1) {
					Com1CarInfo.lightstatus |= (pCarInfo->lightstatus & 0x04);
				}
				Com1CarInfo.lightstatus &= (~0x40);
				if (/*cmdfilter.pkey*/1) {
					Com1CarInfo.lightstatus |= (pCarInfo->lightstatus & 0x40);
				}
				if (/*cmdfilter.tap*/0) {
					Com1CarInfo.gear = (pCarInfo->gear == tappos_R ? tappos_R : tappos_D);
				}
				else {
					Com1CarInfo.gear = pCarInfo->gear;
				}
				if (/*cmdfilter.shieldgear*/0)
					Com1CarInfo.gear = tappos_P;
				if (/*cmdfilter.door*/1) {
					Com1CarInfo.doorstatus = pCarInfo->doorstatus;

					if (/*cmdfilter.door_conversion*/0)
					{
						uint8_t u0 = !!(Com1CarInfo.doorstatus & 0x01);
						uint8_t u1 = !!(Com1CarInfo.doorstatus & 0x02);
						uint8_t u2 = !!(Com1CarInfo.doorstatus & 0x04);
						uint8_t u3 = !!(Com1CarInfo.doorstatus & 0x08);

						Com1CarInfo.doorstatus = (Com1CarInfo.doorstatus & 0xf0) | (u0 << 1) | u1 | (u2 << 2) | (u3 << 3);
					}
				}
				else {
					Com1CarInfo.doorstatus = 0;
				}
				if (/*cmdfilter.speed*/1) {
					Com1CarInfo.speed = pCarInfo->speed;
				}
				else {
					Com1CarInfo.speed = 0;
				}
				if (/*cmdfilter.track*/0) {
					int corner = ((pCarInfo->corner) << 24) | ((pCarInfo->corner & 0x0000ff00) << 8) |
						((pCarInfo->corner & 0x00ff0000) >> 8) | ((pCarInfo->corner) >> 24);
					unsigned int uCorner = -corner;
					Com1CarInfo.corner = ((uCorner) << 24) | ((uCorner & 0x0000ff00) << 8) | ((uCorner & 0x00ff0000) >> 8) | ((uCorner) >> 24);
				}
				else {
					Com1CarInfo.corner = pCarInfo->corner;
				}
				if (/*cmdfilter.radar &&*/ (!bUsingRadar6644 || !bRadar3D)) {
					memcpy((void*)&Com1CarInfo.Radar, (void*)&pCarInfo->Radar, sizeof(Radar_t));
				}
				else {
					auto RadarMode = Com1CarInfo.Radar.RadarMode;
					memset((void*)&Com1CarInfo.Radar, 0, sizeof(Radar_t));
					Com1CarInfo.Radar.RadarMode = RadarMode;
				}
			}
			// printf("Com1CarInfo.gear=%d\n", Com1CarInfo.gear);
			break;
		case CP_CMD_TOUCHCOOR:
			if (!/*cmdfilter.touch*/1) {
				break;
			}
			if (ucBuf[0] == 0x00) {
				ucBuf[0] = 0x11;
				if (ucBuf[1] == 0x00 && ucBuf[2] == 0x00 && ucBuf[3] == 0x00 && ucBuf[4] == 0x00)
				{
					ucBuf[1] = ucLstPos[0];
					ucBuf[2] = ucLstPos[1];
					ucBuf[3] = ucLstPos[2];
					ucBuf[4] = ucLstPos[3];
				}

				ComProtocolSendCmd(AvmMsgQueueRx, ucCmd, ucBuf, ucLen);
			}
			else if (ucBuf[0] == 0x01) {
				ucBuf[0] = 0x10;
				ucLstPos[0] = ucBuf[1];
				ucLstPos[1] = ucBuf[2];
				ucLstPos[2] = ucBuf[3];
				ucLstPos[3] = ucBuf[4];
				ComProtocolSendCmd(AvmMsgQueueRx, ucCmd, ucBuf, ucLen);

				if (protID == ID_CP_RTS | protID == ID_CP_IR) {
					Sleep(70);
					ucBuf[0] = 0x11;
					ComProtocolSendCmd(AvmMsgQueueRx, ucCmd, ucBuf, ucLen);
				}
			}
			else if (ucBuf[0] == 0x10 || ucBuf[0] == 0x11) {
				ComProtocolSendCmd(AvmMsgQueueRx, ucCmd, ucBuf, ucLen);
			}
			else if (ucBuf[0] == 0x20 || ucBuf[0] == 0x21) {
				ComProtocolSendCmd(AvmMsgQueueRx, ucCmd, ucBuf, ucLen);
			}
			break;
		case CP_CMD_SYNCTIME:
			if (/*cmdfilter.time*/1) {
				ComProtocolSendCmd(AvmMsgQueueRx, ucCmd, ucBuf, ucLen);
			}
			break;
		case CP_CMD_IN_TEST_MODE:
			if (ucLen == 12) {
				// 255,0,'S','W',0,0,0x55,0xaau,0x56,0xa9u,0x57,0xa8;
				const uint8_t specialChars[] = { 255, 0, 'S', 'W', 0, 1, 0x55, 0xaau, 0x56, 0xa9u, 0x57, 0xa8 };
				const uint8_t resetChars[] = { 0xFF, 0x00, 0x53, 0x57, 0x00, 0x0F, 0x23, 0x05, 0x55, 0xAA, 0x96, 0xC3 };
				if (!memcmp(resetChars, ucBuf, sizeof(resetChars))) {
					uint8_t resetCmdData[] = { 0x05, 0x55, 0xAA, 0x96, 0xC3 };
					ComProtocolSendCmd(AvmMsgQueueRx, CP_CMD_GP_SET2, resetCmdData, sizeof(resetCmdData));
				}
				else if (((memcmp(&specialChars[6], &ucBuf[6], 6)) == 0) && ((memcmp(specialChars, ucBuf, 5)) == 0)) {
					if (ucBuf[5] == 0) { // 0-- exit test mode.
						comMUXInfo = ComMUXInfo.GetNum();
						if (comMUXInfo.validFlag == 0x55 && comMUXInfo.protId == ID_CP_SW && comMUXInfo.comId == COM1) {
							comMUXInfo.comId = 0;
							comMUXInfo.protId = 0;
							comMUXInfo.validFlag = 0;
							ComMUXInfo.SetNum(comMUXInfo);
							TestFlag = 0;
						}
					}
					else {
						comMUXInfo = ComMUXInfo.GetNum();
						if (comMUXInfo.validFlag != 0x55) {
							comMUXInfo.comId = COM1;
							comMUXInfo.protId = ID_CP_SW;
							comMUXInfo.validFlag = 0x55;
							ComMUXInfo.SetNum(comMUXInfo);
							TestFlag = 1;
						}
					}
					ucBuf[6] = 0xaa;
					ucBuf[7] = 0x55;
					ComProtocolSendAsk(AvmCom1, ucCmd, ucBuf, ucLen, 0);
				}
			}
			break;
		case CP_CMD_GP_SET2:
			if (ID_CP_GP2 == protID && ucLen == (sizeof(DispParam_t) + 1) && ucBuf[0] == 0x03) {
				if (!/*cmdfilter.lcd*/1) {
					break;
				}

				DispParam_t* pDispparam = (DispParam_t*)&ucBuf[1];
				disp_param_t param;
				bool isPresetParam = false;

				printf("CP_CMD_GP_SET2:\n");
				printf("pclk=%d, x=%d, y=%d, ht=%d, vt=%d\n", pDispparam->pclk, pDispparam->x, pDispparam->y, pDispparam->ht, pDispparam->vt);
				printf("hbp=%d, hspw=%d, hfp=%d, vbp=%d, vspw=%d, vfp=%d\n",
					pDispparam->hbp, pDispparam->hspw, pDispparam->hfp, pDispparam->vbp, pDispparam->vspw, pDispparam->vfp);
				printf("hpolarity=%d, vpolarity=%d, clk_phase=%d, dispmode=%d, lvds_mode=%d, lvds_link=%d\n",
					pDispparam->hpolarity, pDispparam->vpolarity, pDispparam->clk_phase, pDispparam->dispmode, pDispparam->lvds_mode, pDispparam->lvds_link);

				RequestDispParamSucess = 1;

				memset(&param, 0, sizeof(param));
				param.id = DISP_PARAM_CUSTOM_ID;

				switch (pDispparam->dispmode)
				{
				case Mode_VGA:
					strcpy(param.iface, "VGA");
					break;
				case Mode_Preset_HDMI_CADILLAC:
				case Mode_HDMI:
					strcpy(param.iface, "HDMI");
					break;
				case Mode_Preset_BT656_CADILLAC:
				case Mode_AHD:
					strcpy(param.iface, "AHD");
					// BT656时序中的行参数需要x2。因为是非preset屏参，故直接修改pDispparam
					pDispparam->ht = pDispparam->ht * 2;
					pDispparam->hbp = (pDispparam->hbp + pDispparam->hspw) * 2;
					pDispparam->hspw = pDispparam->hspw * 2;
					break;

				case Mode_Preset_AHD720P25Hz:
					strcpy(param.iface, "AHD");
					isPresetParam = true;
					strcpy(param.param, "720p@25");
					break;
				case Mode_Preset_AHD720P30Hz:
					strcpy(param.iface, "AHD");
					isPresetParam = true;
					strcpy(param.param, "720p@30");
					break;
				case Mode_Preset_AHD1080P25Hz:
					strcpy(param.iface, "AHD");
					isPresetParam = true;
					strcpy(param.param, "1080p@25");
					break;
				case Mode_Preset_AHD1080P30Hz:
					strcpy(param.iface, "AHD");
					isPresetParam = true;
					strcpy(param.param, "1080p@30");
					break;

				case Mode_LVDS:
					strcpy(param.iface, "LVDS");
					break;

				case Mode_Preset_TVI720P25Hz:
					strcpy(param.iface, "TVI");
					isPresetParam = true;
					strcpy(param.param, "720p@25");
					break;
				case Mode_Preset_TVI720P30Hz:
					strcpy(param.iface, "TVI");
					isPresetParam = true;
					strcpy(param.param, "720p@30");
					break;
				case Mode_Preset_TVI1080P25Hz:
					strcpy(param.iface, "TVI");
					isPresetParam = true;
					strcpy(param.param, "1080p@25");
					break;
				case Mode_Preset_TVI1080P30Hz:
					strcpy(param.iface, "TVI");
					isPresetParam = true;
					strcpy(param.param, "1080p@30");
					break;
				case Mode_Preset_CVBS_NTSC:
					strcpy(param.iface, "CVBS");
					isPresetParam = true;
					strcpy(param.param, "NTSC");
					break;
				case Mode_Preset_CVBS_PAL:
					strcpy(param.iface, "CVBS");
					isPresetParam = true;
					strcpy(param.param, "PAL");
					break;
				default:
					strcpy(param.iface, "CVBS");
					break;
				}

				printf("Param.iface:%s\n", param.iface);
				if (isPresetParam) {
					printf("Param.param:%s\n", param.param);
					if (GCSystemConf.SetDisplayinfoPreset(param.iface, param.param))
					{
						printf("Set displayinfo %s by Decoder.\n", param.iface);
						GCSetings.SetClockFreq(0);
						GCSystemConf.Wirte2File();
						GCSetings.Wirte2File();
						PowerOffRequest(true);
						system("sync");
						system("sync");
						system("reboot");
					}
				}
				else {
					sprintf(param.param, "%s-%dx%d", DISP_PARAM_CUSTOM_PARAM, pDispparam->x, pDispparam->y);
					param.lcd_dclk_freq = pDispparam->pclk / 1000 - (pDispparam->pclk > 1000 ? GCSetings.GetClockFreq() * 1000 : GCSetings.GetClockFreq());
					param.lcd_x = pDispparam->x;
					param.lcd_y = pDispparam->y;
					param.lcd_vt = pDispparam->vt;
					param.lcd_ht = pDispparam->ht;
					param.lcd_vbp = pDispparam->vbp + pDispparam->vspw;
					param.lcd_hbp = pDispparam->hbp + pDispparam->hspw;
					param.lcd_vspw = pDispparam->vspw;
					param.lcd_hspw = pDispparam->hspw;
					param.lcd_hv_clk_phase = pDispparam->clk_phase;
					param.lcd_hv_sync_polarity = (pDispparam->hpolarity) | (pDispparam->vpolarity << 1);
					param.lcd_lvds_mode = pDispparam->lvds_mode;
					param.lcd_lvds_link = pDispparam->lvds_link;

					if (GCSystemConf.CheckDispParam(param) && GCSystemConf.SetDisplayinfo(param))
					{
						// 为适应凯迪拉克HDMI或BT656畸变输出范围而调整render区域
						if (pDispparam->dispmode == Mode_Preset_HDMI_CADILLAC ||
							pDispparam->dispmode == Mode_Preset_BT656_CADILLAC) {
							GCSystemConf.SetRenderROI(170, 360, 1318, 640);
							GCSystemConf.SetCadillacDistort(true);
						}
						else {
							GCSystemConf.SetCadillacDistort(false);
						}
						printf("Set displayinfo %s by Decoder.\n", param.iface);
						GCSetings.SetClockFreq(0);
						GCSystemConf.Wirte2File();
						GCSetings.Wirte2File();
						PowerOffRequest(1);
						system("sync");
						system("sync");
						system("reboot");
					}
				}
			}
			break;
		case CP_CMD_DISP_ROTATE:
			if (/*cmdfilter.rotate_command*/1) {
				rotate_angle setAngle = ucBuf[0] ? rotate_270 : rotate_0;

				rotate_angle getAngle = GCSetings.GetDispRotateAngle();
				if (setAngle != getAngle) {
					disp_param_t dispParam;
					GCSystemConf.GetDisplayinfo(dispParam);
					int w, h;
					if (setAngle == rotate_0 || setAngle == rotate_180)
					{
						w = dispParam.lcd_x;
						h = dispParam.lcd_y;
					}
					else {
						w = dispParam.lcd_y;
						h = dispParam.lcd_x;
					}
					GCSetings.SetResolution(w, h);
					GCSetings.SetDispRotateAngle(setAngle);
					GCSetings.SetTSRotateAngle(setAngle);
					GCSetings.Wirte2File();
					// FeedDog();
					CMyFile file;
					file.open("/tmp/disp_rotate.txt", "wb+");
					file.close();
					GCarMonitor.SetOpType(caroptype_close_quit);
				}
			}
			break;
		case CP_CMD_MB_VERSION:
			if (ID_CP_GP2 == protID && ucBuf[0] == 0x07) {
				// ExpBd::instance().SetExpBdVer((DecoderVer_t*)&ucBuf[1], ucLen);
				if (!DecVerProfile) {
					char verBuf[128] = { 0 };
					char profileBuf[256] = { 0 };
					if (/*ExpBd::instance().GetExpBdVer(verBuf)*/0) {
						sprintf(profileBuf, "EXPANSION=%s", verBuf);
					}
					if (/*ExpBd::instance().GetMcu2Ver(verBuf)*/0) {
						strcat(profileBuf, " / ");
						strcat(profileBuf, verBuf);
					}
					// WriteProfile(profileBuf);
					DecVerProfile = true;
				}
			}
			break;
		case CP_CMD_UPDATA_PROGRAM:
			/*
			if (ID_CP_GP2 == protID) {
				if (ucLen == 0x02 && ucBuf[0] == 0x01)			// 请求总包数
				{
					if (GsMCUGP2UpdateInfo.UpdateState.GetNum() == 0) {
						printf("mcu gp2 query upgrade pack amount, but core waitting for usb disk prepare\n");
						break;
					}
					unsigned char buf[128] = { 0 };
					unsigned int len128 = GsMCUGP2UpdateInfo.iTotalTransferLen;
					buf[0] = 0x01;
					buf[1] = GsMCUGP2UpdateInfo.UpdateFlag.GetNum();	// MCU NO.
					buf[2] = len128 >> 24;
					buf[3] = (len128 >> 16) & 0xFF;
					buf[4] = (len128 >> 8) & 0xFF;
					buf[5] = len128 & 0xFF;
					printf("MCU NO.= %d, len=%d * 128Bytes, ucBuf[2:4] = 0x%02x%02x%02x%02x\n", buf[1], len128, buf[2], buf[3], buf[4], buf[5]);
					ProtocolPackSend_GP2(AvmCom1, 0xF3, buf, 0x06);
				}
				else if (ucLen == 0x06 && ucBuf[0] == 0x02)		// 请求某个包
				{
					unsigned char buf[256] = { 0 };
					unsigned int index = (ucBuf[2] << 24) | (ucBuf[3] << 16) | (ucBuf[4] << 8) | ucBuf[5];
					printf("mcu gp2 request pack %d\n", index);
					if (GsMCUGP2UpdateInfo.UpdateState.GetNum() == 0) {
						printf("but core waitting for usb disk prepare\n");
						break;
					}
					if ((index + 1) > GsMCUGP2UpdateInfo.iTotalTransferLen) {
						printf("mcu gp2 request pack %d is invalid.\n", index);
						break;
					}
					buf[0] = 0x02;
					buf[1] = GsMCUGP2UpdateInfo.UpdateFlag.GetNum();	// MCU NO.
					buf[2] = ucBuf[2];
					buf[3] = ucBuf[3];
					buf[4] = ucBuf[4];
					buf[5] = ucBuf[5];
					// GsMCUGP2UpdateInfo.pucBuf 有预留足够空间，故此处直接取128个字节，不需要考虑尾部不足字节
					memcpy(buf + 6, (unsigned char*)(GsMCUGP2UpdateInfo.pucBuf) + 128 * index, 128);
					ProtocolPackSend_GP2(AvmCom1, 0xF3, buf, 134);
					GsMCUGP2UpdateInfo.EndTransferLen.SetNum(index);
				}
				else if (ucLen == 0x02 && ucBuf[0] == 0x03)		// 升级结束
				{
					printf("mcu gp2 upgrade finished.\n");
					unsigned char buf[128] = { 0 };
					system("sync");				// 有的MCU在此处会强行断电，故同步一下再应答
					buf[0] = 0x03;
					buf[1] = GsMCUGP2UpdateInfo.UpdateFlag.GetNum();	// MCU NO.
					ProtocolPackSend_GP2(AvmCom1, 0xF3, buf, 0x02);
					util::msleep(10);
					ProtocolPackSend_GP2(AvmCom1, 0xF3, buf, 0x02);
					printf("core send ACK for mcu gp2 upgrade finished.\n");
					GsMCUGP2UpdateInfo.UpdateState.SetNum(MB_UPDATE_STATE_END);

				}
			}
			*/
			break;
		default:
			if ((ID_CP_SW == protID) && (ucCmd == CP_CMD_RADAR) /*&& (!cmdfilter.radar)*/) {
				break;
			}
			ComProtocolSendCmd(AvmMsgQueueRx, ucCmd, ucBuf, ucLen);
			break;
		}
	}
	return 0;
}

typedef struct __Key_Rf_t
{
	uint8_t FrameHead0;
	uint8_t FrameHead1;
	uint8_t FrameMode;
	uint8_t FrameLen;
	uint8_t Id[3];
	uint8_t FrameIndex : 5;
	uint8_t FrameModeBit : 3;

	uint8_t KeyValue : 4;
	uint8_t KeyMode : 4;
	uint8_t CRCData;
} Key_Rf_t;

typedef struct __Test_Cmd_t
{
	uint8_t FrameHead0;
	uint8_t FrameHead1;
	uint8_t FrameMode;
	uint8_t Cmd;
	uint8_t Id[3];
	uint8_t CmdData[2];
	uint8_t CRCData;
} Test_Cmd_t;

int CheckTestMode(void) {
	uint8_t ucBuf[256];
	uint8_t ucCmd, ucLen;

	// printf("CheckTestMode++\r\n");

	if (AvmCom2.Open(19200) != 0) {
		printf("<ERROR:> AvmCom2 open is failed!!!!!!\r\n");
		return -1;
	}

#define RF_DATA_SIZE    (10)
	uint32_t testModeCnt = 0;
	uint32_t KeyPressCnt = 0;
	int32_t ilen = 0;
	int32_t size = 0;
	Key_Rf_t* pKeyRf = NULL;
	Test_Cmd_t* pTestCmd = NULL;
	DWORD NoKeySystick = 0;
	while (1) {
		memset(ucBuf, 0, sizeof(ucBuf));
		ilen = AvmCom2.Read(ucBuf, RF_DATA_SIZE, 50);
		if (ilen <= 0) {
		}
		else if (ilen < RF_DATA_SIZE) {
			size = AvmCom2.Read(&ucBuf[ilen], RF_DATA_SIZE - ilen, 20);
			if (size > 0) {
				ilen += size;
			}
		}
		//printf("++++++++++++++%s:Receive data len= %d.\r\n",__FUNCTION__, ilen);
		if (testModeCnt++ > 30) {
			break;
		}
		if (ilen != RF_DATA_SIZE) {
			continue;
		}
		if (ucBuf[ilen - 1] != crc7(0, ucBuf, ilen - 1)) {
			continue;
		}
		testModeCnt = 0;
		//printf("++++++++++++++%s:Receive data CRC is ok.\r\n");
		pKeyRf = (Key_Rf_t*)ucBuf;
		if (pKeyRf->FrameMode == 0x01) {
			uint32_t id = (pKeyRf->Id[2] << 16) | (pKeyRf->Id[1] << 8) | pKeyRf->Id[0];
			if (id == 0x01) {
				//printf("++++++++++++++%s:Receive data Test Cmd is ok.\r\n");
				pTestCmd = (Test_Cmd_t*)ucBuf;
				if (pTestCmd->Cmd == 1 && pTestCmd->CmdData[0] == 0x55 && pTestCmd->CmdData[1] == 0xaa) {
					ucCmd = CP_CMD_GP_SET2;
					ucBuf[0] = 0x05;                           //恢复出厂默认参数命令
					ucBuf[1] = 0x55;
					ucBuf[2] = 0xaa;
					ucBuf[3] = 0x96;
					ucBuf[4] = 0xc3;
					ucLen = 5;
					ComProtocolSendCmd(AvmMsgQueueRx, ucCmd, ucBuf, ucLen);
					printf("++++++++++++++Set default system parament.\r\n");
				}
				else if (pTestCmd->Cmd == 2) {      // set disp parament id.
					ucCmd = CP_CMD_GP_SET2;
					ucBuf[0] = 0x02;
					ucBuf[1] = pTestCmd->CmdData[0];
					ucBuf[2] = pTestCmd->CmdData[1];
					ucLen = 3;
					ComProtocolSendCmd(AvmMsgQueueRx, ucCmd, ucBuf, ucLen);
					printf("++++++++++++++Set display ID to %d.\r\n", (uint16_t)((pTestCmd->CmdData[1] << 8) + pTestCmd->CmdData[0]));
					Sleep(2000);
				}
			}
			else if (id == 0x00) {
				if (KeyPressCnt) {
					if (GetTickCount() - NoKeySystick > 2000) {
						if (KeyPressCnt == 1) {
							uint16_t dispparamID = 2;   // NTSC
							ucCmd = CP_CMD_GP_SET2;
							ucBuf[0] = 0x02;
							ucBuf[1] = (uint8_t)(dispparamID & 0x00ff);
							ucBuf[2] = (uint8_t)(dispparamID >> 8);
							ucLen = 3;
							ComProtocolSendCmd(AvmMsgQueueRx, ucCmd, ucBuf, ucLen);
							printf("++++++++++++++Set display to NTSC.\r\n");
							Sleep(2000);
						}
						else if (KeyPressCnt == 2) {
							uint16_t dispparamID = 1;   // AHD 720P25
							ucCmd = CP_CMD_GP_SET2;
							ucBuf[0] = 0x02;
							ucBuf[1] = (uint8_t)(dispparamID & 0x00ff);
							ucBuf[2] = (uint8_t)(dispparamID >> 8);
							ucLen = 3;
							ComProtocolSendCmd(AvmMsgQueueRx, ucCmd, ucBuf, ucLen);
							printf("++++++++++++++Set display to AHD.\r\n");
							Sleep(2000);
						}
						else if (KeyPressCnt == 3) {
							// set vga
							uint16_t dispparamID = 16;       // vga 720p50
							ucCmd = CP_CMD_GP_SET2;
							ucBuf[0] = 0x02;
							ucBuf[1] = (uint8_t)(dispparamID & 0x00ff);
							ucBuf[2] = (uint8_t)(dispparamID >> 8);
							ucLen = 3;
							ComProtocolSendCmd(AvmMsgQueueRx, ucCmd, ucBuf, ucLen);
							printf("++++++++++++++Set display to VGA.\r\n");
							Sleep(2000);
						}
						else if (KeyPressCnt == 4) {
							// set HDMI
							uint16_t dispparamID = 4;       // HDMI 720p50
							ucCmd = CP_CMD_GP_SET2;
							ucBuf[0] = 0x02;
							ucBuf[1] = (uint8_t)(dispparamID & 0x00ff);
							ucBuf[2] = (uint8_t)(dispparamID >> 8);
							ucLen = 3;
							ComProtocolSendCmd(AvmMsgQueueRx, ucCmd, ucBuf, ucLen);
							printf("++++++++++++++Set display to HDMI.\r\n");
							Sleep(2000);
						}
						KeyPressCnt = 0;
					}
				}
				else {
					NoKeySystick = GetTickCount();
				}

			}
			else {
				if (RmtCtl.GetRmtCtlId() != id) {
					RmtCtl.SetRmtCtlId(pKeyRf->Id);
					// printf("++++++++++++++SetRmtCtlId\r\n");
				}
				// printf("++++++++++++++++++++Receive data KeyMode=%d, KeyValue=%d\r\n",pKeyRf->KeyMode, pKeyRf->KeyValue);
				if (((pKeyRf->KeyMode == 0) && (pKeyRf->KeyValue == 0)) ||
					((pKeyRf->KeyMode == 0x09) && (pKeyRf->KeyValue <= 1)) ||
					((pKeyRf->KeyMode == 0x0A) && (pKeyRf->KeyValue <= 1)))
				{
					KeyPressCnt++;
					NoKeySystick = GetTickCount();
					// printf("KeyPressCnt = %d \r\n", KeyPressCnt);
					continue;
				}
				else {
					KeyPressCnt = 0;
				}

				if ((pKeyRf->KeyMode == 0x09) && (pKeyRf->KeyValue >= 2)) {
					// set ahd
					uint16_t dispparamID = 1;   // AHD 720P25
					ucCmd = CP_CMD_GP_SET2;
					ucBuf[0] = 0x02;
					ucBuf[1] = (uint8_t)(dispparamID & 0x00ff);
					ucBuf[2] = (uint8_t)(dispparamID >> 8);
					ucLen = 3;
					ComProtocolSendCmd(AvmMsgQueueRx, ucCmd, ucBuf, ucLen);
					printf("++++++++++++++Set display to AHD.\r\n");
					Sleep(2000);
				}
				else if ((pKeyRf->KeyMode == 0x0A) && (pKeyRf->KeyValue >= 2)) {
					// set vga
					uint16_t dispparamID = 16;       // vga 720p50
					ucCmd = CP_CMD_GP_SET2;
					ucBuf[0] = 0x02;
					ucBuf[1] = (uint8_t)(dispparamID & 0x00ff);
					ucBuf[2] = (uint8_t)(dispparamID >> 8);
					ucLen = 3;
					ComProtocolSendCmd(AvmMsgQueueRx, ucCmd, ucBuf, ucLen);
					printf("++++++++++++++Set display to VGA.\r\n");
					Sleep(2000);
				}
				else if ((pKeyRf->KeyMode == 0) && (pKeyRf->KeyValue >= 6)) {		// 3s exit test mode.
					ucCmd = CP_CMD_GP_SET2;
					ucBuf[0] = 0x05;                           //恢复出厂默认参数命令
					ucBuf[1] = 0x55;
					ucBuf[2] = 0xaa;
					ucBuf[3] = 0x96;
					ucBuf[4] = 0xc3;
					ucLen = 5;
					ComProtocolSendCmd(AvmMsgQueueRx, ucCmd, ucBuf, ucLen);
					printf("++++++++++++++Set default system parament.\r\n");
					// quit test mode
					uint8_t ack[6];
					ack[0] = 'S';
					ack[1] = 'W';
					ack[2] = 0x01;
					ack[3] = 0x01;
					ack[4] = 0xf5;
					ack[5] = 0x31;       // 0x31

					AvmCom2.Write(ack, 6);
					Sleep(50);
					AvmCom2.Write(ack, 6);
					Sleep(50);
					break;
				}
			}
		}
		else if (pKeyRf->FrameMode == 0x02) {
			//printf("++++++++++++++FrameMode=%d,Receive data Test Cmd is ok.\r\n", pKeyRf->FrameMode);
			uint32_t id = (pKeyRf->Id[2] << 16) | (pKeyRf->Id[1] << 8) | pKeyRf->Id[0];
			if (id == 0x01) {
				//printf("++++++++++++++%s:Receive data Test Cmd is ok.\r\n");
				pTestCmd = (Test_Cmd_t*)ucBuf;
				if (pTestCmd->Cmd == 1 && pTestCmd->CmdData[0] == 0x55 && pTestCmd->CmdData[1] == 0xaa) {
					ucCmd = CP_CMD_GP_SET2;
					ucBuf[0] = 0x05;                           //恢复出厂默认参数命令
					ucBuf[1] = 0x55;
					ucBuf[2] = 0xaa;
					ucBuf[3] = 0x96;
					ucBuf[4] = 0xc3;
					ucLen = 5;
					ComProtocolSendCmd(AvmMsgQueueRx, ucCmd, ucBuf, ucLen);
					printf("++++++++++++++Set default system parament.\r\n");
				}
				else if (pTestCmd->Cmd == 2) {      // set disp parament id.
					ucCmd = CP_CMD_GP_SET2;
					ucBuf[0] = 0x02;
					ucBuf[1] = pTestCmd->CmdData[0];
					ucBuf[2] = pTestCmd->CmdData[1];
					ucLen = 3;
					ComProtocolSendCmd(AvmMsgQueueRx, ucCmd, ucBuf, ucLen);
					printf("++++++++++++++Set display ID to %d.\r\n", (uint16_t)((pTestCmd->CmdData[1] << 8) + pTestCmd->CmdData[0]));
					Sleep(2000);
				}
			}
			else if (id == 0) {
				if (RmtCtl.IsEnable()) {
					RmtCtl.Disable();
					printf("++++++++++++++RmtCtl disable.\r\n");
				}
			}
			else {
				if (RmtCtl.GetRmtCtlId() != id) {
					RmtCtl.SetRmtCtlId(pKeyRf->Id);
					// printf("++++++++++++++SetRmtCtlId\r\n");
				}
				// printf("++++++++++++++++++++Receive data KeyMode=%d, KeyValue=%d\r\n",pKeyRf->KeyMode, pKeyRf->KeyValue);
				if ((pKeyRf->KeyMode == 0) && (pKeyRf->KeyValue == 0))
				{
					ucCmd = CP_CMD_KEY;
					ucLen = 1;
					ucBuf[0] = 0x3;                      // confirm key
					ComProtocolSendCmd(AvmMsgQueueRx, ucCmd, ucBuf, ucLen);
					continue;
				}
				else if ((pKeyRf->KeyMode == 0) && (pKeyRf->KeyValue >= 10))
				{
					ucCmd = CP_CMD_KEY;
					ucLen = 1;
					ucBuf[0] = 0x9;                      // power key
					ComProtocolSendCmd(AvmMsgQueueRx, ucCmd, ucBuf, ucLen);
					continue;
				}
				else if ((pKeyRf->KeyMode == 0) && (pKeyRf->KeyValue >= 4))
				{
					ucCmd = CP_CMD_KEY;
					ucLen = 1;
					ucBuf[0] = 0x4;                      // return key
					ComProtocolSendCmd(AvmMsgQueueRx, ucCmd, ucBuf, ucLen);
					continue;
				}

				if ((pKeyRf->KeyMode == 1) && (pKeyRf->KeyValue))
				{
					ucCmd = CP_CMD_KEY;
					ucLen = 3;
					ucBuf[0] = 0x30;                     // clockwise
					ucBuf[1] = 0x0;
					ucBuf[2] = pKeyRf->KeyValue;
					ComProtocolSendCmd(AvmMsgQueueRx, ucCmd, ucBuf, ucLen);
					continue;
				}
				else if ((pKeyRf->KeyMode == 2) && (pKeyRf->KeyValue))
				{
					ucCmd = CP_CMD_KEY;
					ucLen = 3;
					ucBuf[0] = 0x30;                     // anticlockwise
					ucBuf[1] = 0x1;
					ucBuf[2] = pKeyRf->KeyValue;
					ComProtocolSendCmd(AvmMsgQueueRx, ucCmd, ucBuf, ucLen);
					continue;
				}
				else if ((pKeyRf->KeyMode == 5) && (pKeyRf->KeyValue))
				{
					ucCmd = CP_CMD_KEY;
					ucLen = 2;
					ucBuf[0] = 0x32;                     // 

					if (pKeyRf->KeyValue == 1) {
						ucBuf[1] = 1;
					}
					else if (pKeyRf->KeyValue == 2) {
						ucBuf[1] = 0;
					}
					else if (pKeyRf->KeyValue > 2 && pKeyRf->KeyValue < 0x0B) {
						ucBuf[1] = pKeyRf->KeyValue - 1;
					}
					else {
						continue;
					}
					ComProtocolSendCmd(AvmMsgQueueRx, ucCmd, ucBuf, ucLen);
					continue;
				}
			}
		}
	}
	AvmCom2.Close();
	RmtCtl.Enable();
}

int threadAvmCom2(void* p) {
	uint8_t ucBuf[256];
	uint8_t ucCmd, ucLen;
	uint32_t protID = GCSetings.GetSecondPort();
	uint8_t ucLstPos[4];

	printf("+%s: protID=%d.\r\n", __FUNCTION__, protID);

	CarInfo_t* pCarInfo = (CarInfo_t*)ucBuf;
	static uint32_t TestFlag = 0;
	CheckTestMode();				// 出厂前测试用
	bool bUsingRadar6644 = false;
	bool bRadar3D = GCSetings.GetRadar3D();
	if (protID == ID_CP_IR) {
		// Open IR interface
		if (AvmComIr.Open("/dev/input/event6") != 0)
		{
			if (AvmComIr.Open("/dev/input/event3") != 0) {
				printf("AvmCom2 open failed!\n");
				return -1;
			}
		}
	}
	else if (protID == 2) {
		if (AvmCom2.Open(38400) != 0) {
			printf("AvmCom2 open failed!\n");
			return -1;
		}
	}
	else if (protID == ID_CP_LD_Korea_TOUCHU) {
		if (AvmCom2.Open(9600) != 0) {
			printf("AvmCom2 open failed!\n");
			return -1;
		}
	}
	else if (protID == ID_CP_LAMP)
	{
		if (AvmCom2.Open(38400) != 0) {
			printf("AvmCom2 open failed!\n");
			return -1;
		}
	}
	else {
		if (AvmCom2.Open() != 0) {
			printf("AvmCom2 open failed!\n");
			return -1;
		}
	}

	while (1) {
		int32_t iRet = 0;
		uint32_t curPortID = GCSetings.GetSecondPort();

		if (curPortID != protID) {
			printf("%s: curID = %d , newID = %d\n", __FUNCTION__, protID, curPortID);

			if (protID == ID_CP_IR)
				AvmComIr.Close();
			else
				AvmCom2.Close();

			util::msleep(500);
			protID = curPortID;
			switch (protID)
			{
			case ID_CP_GP1:
				printf("[%d - %s] \r\n",__LINE__, __FILE__);
				AvmCom2.Open(38400);
				break;
			case ID_CP_IR:
			{
				if (AvmComIr.Open("/dev/input/event6") != 0)
				{
					if (AvmComIr.Open("/dev/input/event3") != 0) {
						printf("AvmCom2 open failed!\n");
						return -1;
					}
				}
				break;
			}
			case ID_CP_LD_Korea_TOUCHU:
				AvmCom2.Open(9600);
				break;
			default:
				AvmCom2.Open(19200);
				break;
			}

			//?????????????????
			util::msleep(1000);
		}
		
		memset(ucBuf, 0, sizeof(ucBuf));


		if ((TestFlag != 0) && (protID == ID_CP_GP2 || protID == ID_CP_SW)) {
			iRet = ComRecvCmd_SW(AvmCom2, &ucCmd, ucBuf, &ucLen, 200);
		}
		else {
			switch (protID) {
			case ID_CP_SW:
				iRet = ComRecvCmd_SW(AvmCom2, &ucCmd, ucBuf, &ucLen, 200);
				break;
			case ID_CP_GP1:
				iRet = ComRecvCmd_GP1(AvmCom2, &ucCmd, ucBuf, &ucLen, 200);
				break;
			case ID_CP_IR:
				iRet = IRRecvCmd_NEC(AvmComIr, &ucCmd, ucBuf, &ucLen, 200);
				break;
			case ID_CP_LAMP:
				iRet = -1;
				util::msleep(1000);
				break;
			default:
				iRet = ComRecvCmd_GP2(AvmCom2, &ucCmd, ucBuf, &ucLen, 200);
				break;
			}
		}
		if (iRet != 0) {
			//printf("%s receive frame is failed!!!!!\r\n", __FUNCTION__);
			continue;
		}
		switch (ucCmd) {
		case CP_CMD_CARDATA:    // COM2 暂时不处理车身数据，避免跟COM1数据冲突
			if (ID_CP_GP2_TS_TIME_VOICE == protID) {
				break;
			}
			if (ID_CP_GP2_TRAIL == protID) {
				if (/*cmdfilter.track*/0) {
					int corner = ((pCarInfo->corner) << 24) | ((pCarInfo->corner & 0x0000ff00) << 8) |
						((pCarInfo->corner & 0x00ff0000) >> 8) | ((pCarInfo->corner) >> 24);
					unsigned int uCorner = -corner;
					Com2CarInfo.corner = ((uCorner) << 24) | ((uCorner & 0x0000ff00) << 8) | ((uCorner & 0x00ff0000) >> 8) | ((uCorner) >> 24);
				}
				else {
					Com2CarInfo.corner = pCarInfo->corner;
				}
			}
			else {
				if (/*cmdfilter.acc*/1) {
					Com2CarInfo.accValid = pCarInfo->accValid;
					Com2CarInfo.acc = pCarInfo->acc;
				}
				else {
					Com2CarInfo.accValid = 0;
					Com2CarInfo.acc = 0;
				}
				if (/*cmdfilter.autopark*/1) {
					Com2CarInfo.autopark = pCarInfo->autopark;
				}
				else {
					Com2CarInfo.autopark = 0;
				}
				Com2CarInfo.lightstatus &= (~0x03);
				if (/*cmdfilter.led*/1) {
					Com2CarInfo.lightstatus |= (pCarInfo->lightstatus & 0x03);
				}
				Com2CarInfo.lightstatus &= (~0x04);
				if (/*cmdfilter.doubleled*/1) {
					Com2CarInfo.lightstatus |= (pCarInfo->lightstatus & 0x04);
				}
				Com2CarInfo.lightstatus &= (~0x40);
				if (/*cmdfilter.pkey*/1) {
					Com2CarInfo.lightstatus |= (pCarInfo->lightstatus & 0x40);
				}
				if (/*cmdfilter.tap*/0) {
					Com2CarInfo.gear = (pCarInfo->gear == tappos_R ? tappos_R : tappos_D);
				}
				else {
					Com2CarInfo.gear = pCarInfo->gear;
				}
				if (/*cmdfilter.shieldgear*/0)
					Com2CarInfo.gear = tappos_P;
				if (/*cmdfilter.door*/1) {
					Com2CarInfo.doorstatus = pCarInfo->doorstatus;

					if (/*cmdfilter.door_conversion*/0)
					{
						uint8_t u0 = !!(Com2CarInfo.doorstatus & 0x01);
						uint8_t u1 = !!(Com2CarInfo.doorstatus & 0x02);
						uint8_t u2 = !!(Com2CarInfo.doorstatus & 0x04);
						uint8_t u3 = !!(Com2CarInfo.doorstatus & 0x08);

						Com2CarInfo.doorstatus = (Com2CarInfo.doorstatus & 0xf0) | (u0 << 1) | u1 | (u2 << 2) | (u3 << 3);
					}
				}
				else {
					Com2CarInfo.doorstatus = 0;
				}
				if (/*cmdfilter.speed*/1) {
					Com2CarInfo.speed = pCarInfo->speed;
				}
				else {
					Com2CarInfo.speed = 0;
				}
				if (/*cmdfilter.track*/0) {
					int corner = ((pCarInfo->corner) << 24) | ((pCarInfo->corner & 0x0000ff00) << 8) |
						((pCarInfo->corner & 0x00ff0000) >> 8) | ((pCarInfo->corner) >> 24);
					unsigned int uCorner = -corner;
					Com2CarInfo.corner = ((uCorner) << 24) | ((uCorner & 0x0000ff00) << 8) | ((uCorner & 0x00ff0000) >> 8) | ((uCorner) >> 24);
				}
				else {
					Com2CarInfo.corner = pCarInfo->corner;
				}
				if (/*cmdfilter.radar &&*/ (!bUsingRadar6644 || !bRadar3D)) {
					memcpy((void*)&Com2CarInfo.Radar, (void*)&pCarInfo->Radar, sizeof(Radar_t));
				}
				else {
					auto RadarMode = Com2CarInfo.Radar.RadarMode;
					memset((void*)&Com2CarInfo.Radar, 0, sizeof(Radar_t));
					Com2CarInfo.Radar.RadarMode = RadarMode;
				}
			}
			Sleep(20);
			break;

		case CP_CMD_RADAR_6644:
			bUsingRadar6644 = true;
			if (/*cmdfilter.radar &&*/ bRadar3D) {
				auto RadarMode = Com2CarInfo.Radar.RadarMode;
				memset((void*)&Com2CarInfo.Radar, 0, sizeof(Radar_t));
				Com2CarInfo.Radar.RadarMode = RadarMode;

				memcpy((void*)&Com2CarInfo.Radar6644, ucBuf, sizeof(Radar6644_t));
			}
			else {
				memset((void*)&Com2CarInfo.Radar6644, 0, sizeof(Radar6644_t));
			}
			Sleep(20);
			break;

		case CP_CMD_KEY:
			if ((ID_CP_GP3 != protID) || (ID_CP_GP2_TRAIL != protID)) {
				if ((/*cmdfilter.key &&*/ (ucBuf[0] != 0x32)) ||
					(/*cmdfilter.voice &&*/ (ucBuf[0] == 0x32))) {
					ComProtocolSendCmd(AvmMsgQueueRx, ucCmd, ucBuf, ucLen);
				}
			}
			break;
		case CP_CMD_TOUCHCOOR:
			if (!/*cmdfilter.touch*/1) {
				break;
			}
			if (ucBuf[0] == 0x00) {
				ucBuf[0] = 0x11;
				if (ucBuf[1] == 0x00 && ucBuf[2] == 0x00 && ucBuf[3] == 0x00 && ucBuf[4] == 0x00)
				{
					ucBuf[1] = ucLstPos[0];
					ucBuf[2] = ucLstPos[1];
					ucBuf[3] = ucLstPos[2];
					ucBuf[4] = ucLstPos[3];
				}
				ComProtocolSendCmd(AvmMsgQueueRx, ucCmd, ucBuf, ucLen);
			}
			else if (ucBuf[0] == 0x01) {
				ucBuf[0] = 0x10;
				ucLstPos[0] = ucBuf[1];
				ucLstPos[1] = ucBuf[2];
				ucLstPos[2] = ucBuf[3];
				ucLstPos[3] = ucBuf[4];
				ComProtocolSendCmd(AvmMsgQueueRx, ucCmd, ucBuf, ucLen);
				if (protID == ID_CP_RTS | protID == ID_CP_IR) {
					Sleep(40);
					ucBuf[0] = 0x11;
					ComProtocolSendCmd(AvmMsgQueueRx, ucCmd, ucBuf, ucLen);
				}
			}
			else if (ucBuf[0] == 0x10 || ucBuf[0] == 0x11) {
				ComProtocolSendCmd(AvmMsgQueueRx, ucCmd, ucBuf, ucLen);
			}
			break;
		case CP_CMD_SYNCTIME:
			static DWORD updateSysTimeCnt = 0;
			if (/*cmdfilter.time*/1) {
				if ((updateSysTimeCnt == 0) || (GetTickCount() - updateSysTimeCnt > (60000))) {        // 60s update 1 time
					ComProtocolSendCmd(AvmMsgQueueRx, ucCmd, ucBuf, ucLen);
					updateSysTimeCnt = GetTickCount();
				}
			}
			break;

		case CP_CMD_DISP_ROTATE:
			if (/*cmdfilter.rotate_command*/1) {
				rotate_angle setAngle = ucBuf[0] ? rotate_270 : rotate_0;

				rotate_angle getAngle = GCSetings.GetDispRotateAngle();
				if (setAngle != getAngle) {
					disp_param_t dispParam;
					GCSystemConf.GetDisplayinfo(dispParam);
					int w, h;
					if (setAngle == rotate_0 || setAngle == rotate_180)
					{
						w = dispParam.lcd_x;
						h = dispParam.lcd_y;
					}
					else {
						w = dispParam.lcd_y;
						h = dispParam.lcd_x;
					}
					GCSetings.SetResolution(w, h);
					GCSetings.SetDispRotateAngle(setAngle);
					GCSetings.SetTSRotateAngle(setAngle);
					GCSetings.Wirte2File();
					// FeedDog();
					CMyFile file;
					file.open("/tmp/disp_rotate.txt", "wb+");
					file.close();
					GCarMonitor.SetOpType(caroptype_close_quit);
				}
			}
			break;

		case CP_CMD_IN_TEST_MODE:
			if (ucLen == 12) {
				// 255,0,'S','W',0,0,0x55,0xaau,0x56,0xa9u,0x57,0xa8;
				const uint8_t specialChars[] = { 255, 0, 'S', 'W', 0, 1, 0x55u, 0xaau, 0x56u, 0xa9u, 0x57u, 0xa8u };
				const uint8_t resetChars[] = { 0xFF, 0x00, 0x53, 0x57, 0x00, 0x0F, 0x23, 0x05, 0x55, 0xAA, 0x96, 0xC3 };
				if (!memcmp(resetChars, ucBuf, sizeof(resetChars))) {
					uint8_t resetCmdData[] = { 0x05, 0x55, 0xAA, 0x96, 0xC3 };
					ComProtocolSendCmd(AvmMsgQueueRx, CP_CMD_GP_SET2, resetCmdData, sizeof(resetCmdData));
				}
				else if (((memcmp(&specialChars[6], &ucBuf[6], 6)) == 0) && ((memcmp(specialChars, ucBuf, 5)) == 0)) {
					if (ucBuf[5] == 0) { // 0-- exit test mode.
						comMUXInfo = ComMUXInfo.GetNum();
						if (comMUXInfo.validFlag == 0x55 && comMUXInfo.protId == ID_CP_SW && comMUXInfo.comId == COM2) {
							comMUXInfo.comId = 0;
							comMUXInfo.protId = 0;
							comMUXInfo.validFlag = 0;
							ComMUXInfo.SetNum(comMUXInfo);
							TestFlag = 0;
						}
					}
					else {
						comMUXInfo = ComMUXInfo.GetNum();
						if (comMUXInfo.validFlag != 0x55) {
							comMUXInfo.comId = COM2;
							comMUXInfo.protId = ID_CP_SW;
							comMUXInfo.validFlag = 0x55;
							ComMUXInfo.SetNum(comMUXInfo);
							TestFlag = 1;
						}
					}
				}
				ucBuf[6] = 0xaa;
				ucBuf[7] = 0x55;
				ComProtocolSendAsk(AvmCom2, ucCmd, ucBuf, ucLen, 0);
			}
			break;
		default:
			ComProtocolSendCmd(AvmMsgQueueRx, ucCmd, ucBuf, ucLen);
			break;
		}
	}
	return 0;
}

int threadAvmMsgQueueTx(void* p) {
	uint8_t ucBuf[256];
	uint8_t ucCmd;
	uint8_t ucLen;
	uint8_t ucStatus;

	printf("+%s\r\n", __FUNCTION__);
	while (1) {
		int iRet = ComRecvAsk(AvmMsgQueueTx, &ucCmd, ucBuf, &ucLen, &ucStatus, -1);
		if (iRet != 0) {
			continue;
		}

		if ((ucCmd == CP_CMD_NOTICE) || (ucCmd == CP_CMD_UPDATA_PROGRAM) || (ucCmd == CP_CMD_GP_QUERYSTATE2)) {
			//printf(">>>>>>>>%s:AVM->MCU,cmd=%02x,data=%x,len=%d,", __FUNCTION__, ucCmd, ucBuf[0], ucLen);
			ComProtocolSendAsk(AvmComToMcu, ucCmd, ucBuf, ucLen, ucStatus);
			continue;
		}

		comMUXInfo = ComMUXInfo.GetNum();
		if (comMUXInfo.validFlag == 0x55) {
			if (comMUXInfo.comId == COM1) {
				if (comMUXInfo.protId == ID_CP_SW) {
					ComProtocolSendAsk(AvmCom1, ucCmd, ucBuf, ucLen, ucStatus);
					continue;
				}
			}
			else if (comMUXInfo.comId == COM2) {
				if (comMUXInfo.protId == ID_CP_SW) {
					ComProtocolSendAsk(AvmCom2, ucCmd, ucBuf, ucLen, ucStatus);
					continue;
				}
			}
			else if (comMUXInfo.comId == COM3) {
				if (comMUXInfo.protId == ID_CP_SW) {
					ComProtocolSendAsk(AvmComToMcu, ucCmd, ucBuf, ucLen, ucStatus);
					continue;
				}
			}
		}

		ComProtocolSendAsk(AvmComToMcu, ucCmd, ucBuf, ucLen, ucStatus);
	}

	while (1) {

		sleep(1);
	}
	return 0;
}
/*****************************************************************************
  UART 的控制线程: 全景模式下
*****************************************************************************/
int threadControl_Uart(void* p)
{
	char cStr[128];
	char cStr2[128];

	// int iAppMode = APP_MODE_NORMAL;

	printf("%s\n", __FUNCTION__);

	unsigned char ucCmd, ucLen, ucStatus, ucKey;
	unsigned char ucLastUnknowCmd = 0;

	unsigned char ucData[256];
	unsigned char ucData2[256];

	int iRt = 0;
	int flag, x, y, viewid;
	// mouse_action ts;

	// CvPoint3D32f pos, lookat;
	float fov, aspect;
	int iIsMirror, iIsMillimeter;
	int iModelId;
	int iCam;
	// CvRect rect;

	int iRtMd5 = 0;
	// md5_status md5Status;

	int iPrintCnt = 0;

	float fTmp;

	int iMbUpdateFlag = 0;
	int iTotalLen, iCurLen, iResLen, iOffset;

	do {

		iRt = GsComProt.ReadCmd(&ucCmd, &ucData[0], &ucLen);
		if (iRt < 0) {
			continue;
		}

		
		switch (ucCmd) {

		case 0x07:                                                  //  触摸屏事件
			flag = ucData[0];                                       //  0x00: 触摸屏短按；0x01：触摸屏长按
			x = ucData[1] * 256 + ucData[2];
			y = ucData[3] * 256 + ucData[4];
			uart_ts_push(x, y, flag == 0x10);
			break;

		case 0x09:                                                  //  车身数据
			if (ucLen == 0x08) {
				static unsigned char ucDataLast[8];
				static unsigned int  uiCarTick;
				static unsigned char ucAccLast = 0;
				unsigned char        ucAcc = 0;
				unsigned char        ucLedstatus;

				if (memcmp(&ucDataLast[0], &ucData[0], 3)) {        //  数据有变化，只关注前3项的档位，ACC, 灯，门 信号
					printf("CarData: ");
					for (int n = 0; n < ucLen; n++) {
						printf("%02x, ", ucData[n]);
					}
					printf("\n");
					iPrintCnt = 0;
					uiCarTick = GetTickCount();
				}
				else {
					iPrintCnt++;
					unsigned int uiCurrentTick;
					uiCurrentTick = GetTickCount();
					if (uiCarTick + 30000 < uiCurrentTick) {        //  隔 30 秒打印一次
						iPrintCnt = 0;
						printf("CarData: ");
						for (int n = 0; n < ucLen; n++) {
							printf("%02x, ", ucData[n]);
						}
						printf("\n");
						uiCarTick = uiCurrentTick;
					}
				}
				memcpy(&ucDataLast[0], &ucData[0], 8);
				GCarData.SetCarData(&ucData[0], 8);
				GCarData.Sync();

				GCarMonitor.SetCarData(&ucData[0], ucLen);

				GCarDataLast.SetCarData(&ucData[0], 8);
				GCarDataLast.Sync();
			}
			else {
				printf("<WARNING> CarData Not 8 bytes, but %d bytes\r\n", ucLen);
				GsComProt.WriteAck(ucCmd, CP_ERROR_PARAMINVALID);
			}
			break;

		case 0x10:
			if ((ucData[0] == 0x00) || (ucData[0] == 0x01) || (ucData[0] == 0x02) || (ucData[0] == 0x03)) {
				unsigned char stage[] = { 0, 0, 0, 0 };
				unsigned char stagePack[] = { 0, 0 };

				// if (cmdfilter.radar) {
				// 	for (int i = 0; (i < 4) && ((i * 2 + 3) < ucLen); i++) {
				// 		stage[i] = ucData[i * 2 + 3] & 0xF;
				// 	}
				// }
				stagePack[0] = stage[0] << 4;
				stagePack[0] |= stage[1];
				stagePack[1] = stage[2] << 4;
				stagePack[1] |= stage[3];

			}
			else if (ucData[0] == 0xB2 || ucData[0] == 0xB3) {			// 锟斤拷0xB2锟斤拷0xB3锟斤拷锟斤拷锟斤拷4锟斤拷前锟斤拷+ 4锟斤拷锟襟）革拷锟阶达”锟斤拷锟斤拷

				if (!GCSetings.GetRadarState()) {
					bzero(&ucData[1], 4);
				}
			}
			else {
				printf("Radar data (ucData[0]=0x%02X)is invalid!\n", ucData[0]);
			}

			break;

		case 0x1E:
			if ((ucData[0] == 0x00 || ucData[0] == 0x01) && ucData[1] == 0x56 && ucData[2] == 0xA9) {
				// vrEnableRecord(0);

				ucData2[0] = ucData[0];
				ucData2[1] = ucData[1];
				ucData2[2] = ucData[2];

				int i = 0;
				// while (vrIsDisEnableDone() == 0) {
				// 	i++;
				// 	if (i > 100 * 20)
				// 		break;
				// 	Sleep(10);
				// }
				system("sync");
				system("sync");
				// DiskManage::DoUmount();
				GsComProt.WriteAck(ucCmd, CP_ERROR_SUCCESS, ucData2, 3);
				Sleep(20);
				GsComProt.WriteAck(ucCmd, CP_ERROR_SUCCESS, ucData2, 3);

				if (ucData[0] == 0x00)
					system("reboot");
				else
					system("poweroff");
			}
			break;
		default:
			ucLastUnknowCmd = ucCmd;
			// GsComProt.WriteAck(ucCmd, CP_ERROR_UNSUPPORT);
			break;
		}

		uart_agreement_internal_on_frame(ucCmd, ucData, ucLen);
	} while (1);
	return 0;
}


void NotifyParkMonitorState(void)
{
#ifndef WIN32
	unsigned char ucData[32] = { 0xFE };
	//printf("%s, %d\n", __FUNCTION__, __LINE__);
	GsComProt.WriteAck(0x20, CP_ERROR_SUCCESS, ucData, 1);
	Sleep(10);
	GsComProt.WriteAck(0x20, CP_ERROR_SUCCESS, ucData, 1);
#endif
}

void PowerOffRequest(int iIsReboot)
{
#ifndef WIN32
	// vrEnableRecord(0);
	// int i = 0;
	// while (vrIsDisEnableDone() == 0) {
	// 	if (i++ > 10)
	// 		break;
	// 	printf("PowerOffRequest: iIsReboot = %d, cnt = %d\n", iIsReboot, i);
	// 	Sleep(10);
	// }
	// DiskManage::DoUmount();

	unsigned char ucData[32] = { 0xF1 };
	// 重启请求
	if (iIsReboot)
		ucData[0] = 0xF0;

	GsComProt.WriteAck(0x20, CP_ERROR_SUCCESS, ucData, 1);
	Sleep(10);
	GsComProt.WriteAck(0x20, CP_ERROR_SUCCESS, ucData, 1);
	system("sync");
#endif
}

bool McuPortConnected()
{
#ifndef WIN32
	return GMcuPortConnected;
#else
	return true;
#endif
}
