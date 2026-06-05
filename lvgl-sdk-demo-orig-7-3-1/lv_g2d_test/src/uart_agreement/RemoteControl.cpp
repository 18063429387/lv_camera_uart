#ifndef WIN32
#include <stdio.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
// #include "CMyFile.h"
#include "RemoteControl.h"
#include "ComProtocols.h"
#include "threadUart.h"
// #include "version.h"

RemoteControl::RemoteControl() {
	RmtCtlID = 0x5F8557;        // default id.
	//RmtCtlID = 0x2c74f6;        // default id.
	ReadySaveIdFlag = 0;
	SetDispParamCnt = 0;
	SetDispKeyCnt = 0;
	RecvKeyCnt = 0;
	RmtCtlDisable = 1;
	SystemTickCnt = 0;
	SetDispParamStep1 = 0;
	SetDispParamStep2 = 0;
	SetDispParamStep3 = 0;
	SaveIdTimeoutCnt = GetTickCount();
	UpdateRmtCtlId();
}
RemoteControl::~RemoteControl() {
}
int32_t RemoteControl::SetRmtCtlId(uint8_t* pId) {
	uint8_t ucBuf[256];

	DIR* dir = NULL;
	char dirPath[128];
	sprintf(dirPath, "%s", "/usr/.bvdata/");
	if ((dir = opendir(dirPath)) == NULL) {
		printf("%s not exist, now mkdir it...\r\n", dirPath);
		int32_t status = mkdir(dirPath, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
		if (status != 0) {
			printf("<WARNING> mkdir(%s) failed!\r\n", dirPath);
			return -1;
		}
		else {
			printf("mkdir(%s) sucessed!\r\n", dirPath);
		}
	}
	else {
		closedir(dir);
	}

	// CMyFile rmtCtlId;
	char filePath[128];
	sprintf(filePath, "%s", "/usr/.bvdata/telCtlId");
	// if (rmtCtlId.open(filePath, "w") != 0) {
	// 	printf("open %s is failed !\r\n", filePath);
	// 	rmtCtlId.close();
	// 	return -1;
	// }
	// else {
	// 	printf("open telCtlId is sucess !\r\n");
	// 	uint32_t id = (pId[2] << 16) | (pId[1] << 8) | pId[0];
	// 	if (rmtCtlId.write((char*)&id, sizeof(id)) == sizeof(id)) {
	// 		RmtCtlID = id;
	// 	}
	// 	else {
	// 		printf("<ERROR:> write rmtCtlId is failed!\r\n");
	// 		rmtCtlId.close();
	// 		return -1;
	// 	}
	// 	rmtCtlId.close();
	// }
	return 0;
}

int32_t RemoteControl::UpdateRmtCtlId(void) {
	// CMyFile rmtCtlId;
	char filePath[128];
	sprintf(filePath, "%s", "/usr/.bvdata/telCtlId");
	// if (rmtCtlId.open(filePath, "rb") != 0) {
	// 	printf("open %s is failed ! recreate file.\r\n", filePath);
	// 	DIR* dir = NULL;
	// 	char dirPath[128];
	// 	sprintf(dirPath, "%s", "/usr/.bvdata/");
	// 	if ((dir = opendir(dirPath)) == NULL) {
	// 		printf("%s not exist, now mkdir it...\r\n", dirPath);
	// 		int32_t status = mkdir(dirPath, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
	// 		if (status != 0) {
	// 			printf("<WARNING> mkdir(%s) failed!\r\n", dirPath);
	// 			return -1;
	// 		}
	// 		else {
	// 			printf("mkdir(%s) sucessed!\r\n", dirPath);
	// 		}
	// 	}
	// 	else {
	// 		closedir(dir);
	// 	}
	// 	if (rmtCtlId.open(filePath, "w") != 0) {
	// 		printf("create %s is failed again! \r\n", filePath);
	// 		return -1;
	// 	}
	// 	else {
	// 		if (rmtCtlId.write((char*)&RmtCtlID, sizeof(RmtCtlID)) != sizeof(RmtCtlID)) {
	// 			printf("<ERROR:> write rmtCtlId is failed!\r\n");
	// 			rmtCtlId.write((char*)&RmtCtlID, sizeof(RmtCtlID));
	// 		}
	// 		rmtCtlId.close();
	// 	}
	// }
	// else {
	// 	printf("open rmtCtlId is sucess !\r\n");
	// 	uint32_t id;
	// 	if (rmtCtlId.read((char*)&id, sizeof(id)) != sizeof(id)) {
	// 		printf("<ERROR:> read rmtCtlId is failed!\r\n");
	// 		rmtCtlId.close();
	// 		return -1;
	// 	}
	// 	printf("Read file: 0x%x \r\n", id);
	// 	RmtCtlID = id;
	// 	rmtCtlId.close();
	// }
	return 0;
}

// 手持遥控器接收数据处理
int32_t HandRmtCtlRecvDataDeal(RemoteControlVal_t keyVal, uint8_t* pucCmd, uint8_t* ucBuf, uint8_t* pucLen) {
	if (keyVal.ModeBits != 0x06 && keyVal.Id[0] != 0x50) {
		printf("<WARNING> keyVal.ModeBits and id[0] is error, ModeBits=0x%02x, id[0]=0x%02x\r\n", keyVal.ModeBits, keyVal.Id[0]);
		return -1;
	}

	HandRemoteControlVal_t* pHandRmtCtrl = (HandRemoteControlVal_t*)&keyVal;

	uint8_t tmpKey = pHandRmtCtrl->KeyValue;
	uint8_t tmpTick = pHandRmtCtrl->TickCnt;

	if (tmpKey == 0x04 && tmpTick == 0) {    // 确认按键
		ucBuf[0] = 0x03;        // confirm key
		*pucLen = 1;

	}
	else if (tmpKey == 0x08 && (tmpTick == 0 || tmpTick > 20)) {
		//printf("Retrun key.\r\n");
		ucBuf[0] = 0x04;        // return key
		*pucLen = 1;

	}
	else if ((tmpKey == 0x10 || tmpKey == 0x20) &&        // 下键和右键
		(tmpTick == 0 || tmpTick > 20)) {
		//printf("Clockwise key tmpTick = %d.\r\n", tmpTick);
		*pucLen = 1;
		ucBuf[0] = tmpKey == 0x10 ? 0x06 : 0x08;

	}
	else if ((tmpKey == 0x02 || tmpKey == 0x40) &&        // 左键和上键
		(tmpTick == 0 || tmpTick > 20)) {
		// printf("Anticlockwise key tmpTick = %d.\r\n", tmpTick);
		// left^M
		*pucLen = 1;
		ucBuf[0] = tmpKey == 0x02 ? 0x07 : 0x05;

	}
	else if (tmpKey == 0x30 && tmpTick == 0) {                // menu 键
		//printf("menu  key.\r\n");
		ucBuf[0] = 0x02;        // menu key
		*pucLen = 1;
	}
	else if (tmpKey == 0x01 && tmpTick == 0) {                // 开关屏键
		ucBuf[0] = 0x32;
		ucBuf[1] = 0x20;
		*pucLen = 2;
	}
	else if ((tmpKey == 0x04 || tmpKey == 0x30) && tmpTick >= 100) {      // 长按ok键10s以上，发送进入cvbs同显命令
		*pucCmd = 0x25;
		ucBuf[0] = 0x01;
		ucBuf[1] = 0x01;
		ucBuf[2] = 0x30;
		ucBuf[3] = 0x02;
		*pucLen = 4;
		return 0;
	}
	else
	{

		return -1;
	}
	*pucCmd = CP_CMD_KEY;

	return 0;
}

int32_t KnobRmtCtlRecvDataDeal(RemoteControlVal_t keyVal, uint8_t* pucCmd, uint8_t* ucBuf, uint8_t* pucLen) {
	return 0;
}
int32_t RemoteControl::RmtCtlRecvDataDeal(RemoteControlVal_t keyVal, uint8_t* pucCmd, uint8_t* ucBuf, uint8_t* pucLen) {
	if (RmtCtlDisable == 0) {
		return -1;
	}

	if (keyVal.ModeBits == 0x06 && keyVal.Id[0] == 0x50 && *pucLen == 7) {
		return HandRmtCtlRecvDataDeal(keyVal, pucCmd, ucBuf, pucLen);
	}
	else if (keyVal.ModeBits != 0x07 || *pucLen != 6) {
		printf("<WARNING> keyVal.ModeBits error, %0x02x\r\n", keyVal.ModeBits);
		return -1;
	}

	if ((keyVal.KeyMode == 0x04) && (keyVal.KeyValue == 0x0A)) {
		ReadySaveIdFlag = 0x55;
		return -1;
	}
	if (ReadySaveIdFlag == 0x55) {
		if ((keyVal.KeyMode == 0x04) && (keyVal.KeyValue == 0x05)) {
			if (1 /*GetTickCount() - SaveIdTimeoutCnt < (30000)*/) {          // 30s
				if (SetRmtCtlId(keyVal.Id) == 0) {
					printf("SetRmtCtlId is sucessed.\r\n");
				}
			}
			else {
				printf("SetRmtCtlId is Timeout.\r\n");
			}

		}
		ReadySaveIdFlag = 0;
	}

	uint32_t id = (keyVal.Id[2] << 16) | (keyVal.Id[1] << 8) | keyVal.Id[0];
	char cDate[128] = { 0 };
	int iCustomId = 1;
#define CUSTOMID_SHIMENG   20
	// verGetProductBat2(cDate, iCustomId);

	//  世盟版本不需要匹配遥控器ID即可控
	//if (RmtCtlID == id ) {
	if ((RmtCtlID == id) ||
		((iCustomId == CUSTOMID_SHIMENG) && (0 != id) && (0xFFFFFF != (id & 0xFFFFFF)))) {
	}
	else {
		//printf(">>>Current Id=0x%x is error.Need KeyId=0x%x\r\n", id, RmtCtlID);
		return -1;
	}

	// led set
	SystemTickCnt = GetTickCount();
	if (SetDispParamStep3 == 1) {
		if (((keyVal.KeyMode == 0) && (keyVal.KeyValue == 0)) ||
			((keyVal.KeyMode == 0x09) && (keyVal.KeyValue <= 1)) ||
			((keyVal.KeyMode == 0x0A) && (keyVal.KeyValue <= 1)))
		{
			SetDispParamCnt++;
			printf("+++++++++++SetDispParamCnt = %d;\r\n", SetDispParamCnt);
		}
		else if (!((keyVal.KeyMode == 0) && (keyVal.KeyValue >= 10))) {
			SetDispParamStep1 = 0;
			SetDispParamStep2 = 0;
			SetDispParamStep3 = 0;
			SetDispParamCnt = 0;
		}
		return -1;
	}
	if (SetDispParamStep2 >= 1) {
		if ((keyVal.KeyMode == 0) && (keyVal.KeyValue >= 10)) {
			SetDispParamStep3 = 1;
			SetDispParamCnt = 0;
			printf("+++++++++++Step3 = 1;\r\n");
			/*uint8_t tmp = LED_ON;*/
			//xQueueSend(LedCtrlQueue, (void*)&tmp, pdMS_TO_TICKS(10));
		}
		else if (!(((keyVal.KeyMode == 0) && (keyVal.KeyValue < 10)) ||
			((keyVal.KeyMode == 0x09) && (keyVal.KeyValue <= 1)) ||
			((keyVal.KeyMode == 0x0A) && (keyVal.KeyValue <= 1)))) {
			SetDispParamStep1 = 0;
			SetDispParamStep2 = 0;
		}
		return -1;
	}

	if (SetDispParamStep1 == 1) {
		if (((keyVal.KeyMode == 0) && (keyVal.KeyValue == 0)) ||
			((keyVal.KeyMode == 0x09) && (keyVal.KeyValue <= 1)) ||
			((keyVal.KeyMode == 0x0A) && (keyVal.KeyValue <= 1)))
		{
			SetDispKeyCnt++;
			if (SetDispKeyCnt >= 4) {
				SetDispKeyCnt = 0;
				SetDispParamStep2 = 1;
				printf("+++++++++++Step2 = 1;\r\n");
			}

		}
		else if (!(keyVal.KeyMode == 0) && (keyVal.KeyValue >= 15)) {
			SetDispParamStep1 = 0;
		}
		if ((keyVal.KeyMode == 0) && (keyVal.KeyValue >= 15))
		{
			if (RecvKeyCnt++ >= 5) {                                 // 5*0.5=2.5s, add 7.5s wait for 10s.
				*pucCmd = 0x25;
				ucBuf[0] = 0x01;
				ucBuf[1] = 0x01;
				ucBuf[2] = 0x30;
				ucBuf[3] = 0x02;
				*pucLen = 4;
				RecvKeyCnt = 0;
				return 0;
			}
			else {
				SetDispParamStep1 = 1;
				SetDispParamStep2 = 0;
				SetDispParamStep3 = 0;
				SetDispParamCnt = 0;
				SetDispKeyCnt = 0;
				printf("+++++++++++Step1 = 1;\r\n");
			}
		}
		return -1;
	}
#if 0
	if ((keyVal.KeyMode == 0) && (keyVal.KeyValue >= 12) && (keyVal.KeyValue < 15))          // keyValue maxvalue is 15. ?ˉ?00ms??‘é�???�???????�??…±15*0.5=7.5s
	{
		SetDispParamStep1 = 1;
		SetDispParamStep2 = 0;
		SetDispParamStep3 = 0;
		SetDispParamCnt = 0;
		SetDispKeyCnt = 0;
		printf("+++++++++++Step1 = 1;\r\n");
		return -1;
	}
#else 
	if ((keyVal.KeyMode == 0) && (keyVal.KeyValue >= 15))
	{
		if (RecvKeyCnt++ >= 5) {                                 // 5*0.5=2.5s, add 7.5s wait for 10s.
			*pucCmd = 0x25;
			ucBuf[0] = 0x01;
			ucBuf[1] = 0x01;
			ucBuf[2] = 0x30;
			ucBuf[3] = 0x02;
			*pucLen = 4;
			RecvKeyCnt = 0;
			return 0;
		}
		return -1;
	}
#endif
	RecvKeyCnt = 0;

	if ((keyVal.KeyMode == 0) && (keyVal.KeyValue == 0))
	{
		//printf("Confirm key.\r\n");

		ucBuf[0] = 0x03;        // confirm key
		*pucLen = 1;
	}/*
	else if ((keyVal.KeyMode == 0) && (keyVal.KeyValue >= 10))
	{
		//printf("Power key.\r\n");
		ucBuf[0] = 0x09;        // power key
		*pucLen = 1;
	}*/
	else if ((keyVal.KeyMode == 0) && (keyVal.KeyValue >= 4))
	{
		//printf("Retrun key.\r\n");
		ucBuf[0] = 0x04;        // return key
		*pucLen = 1;
	}
	else if ((keyVal.KeyMode == 1) && (keyVal.KeyValue))
	{
		//printf("Clockwise key.\r\n");
		ucBuf[0] = 0x30;        // clockwise
		ucBuf[1] = 0;
		ucBuf[2] = keyVal.KeyValue;
		*pucLen = 3;
	}
	else if ((keyVal.KeyMode == 2) && (keyVal.KeyValue))
	{
		//printf("Anticlockwise key.\r\n");
		ucBuf[0] = 0x30;        // anticlockwise
		ucBuf[1] = 1;
		ucBuf[2] = keyVal.KeyValue;
		*pucLen = 3;
	}
	else if ((keyVal.KeyMode == 5) && (keyVal.KeyValue))
	{
		//printf("Other key.\r\n");
		ucBuf[0] = 0x32;        // anticlockwise
		if (keyVal.KeyValue == 1) {
			ucBuf[1] = 1;
		}
		else if (keyVal.KeyValue == 2) {
			ucBuf[1] = 0;
		}
		else if (keyVal.KeyValue > 2 && keyVal.KeyValue < 0x0B) {
			ucBuf[1] = keyVal.KeyValue - 1;
		}
		else {
			return -1;
		}
		*pucLen = 2;
	}
	*pucCmd = CP_CMD_KEY;

	return 0;
}

int32_t RemoteControl::RmtCtlNoRecvDataDeal(uint8_t* pucCmd, uint8_t* ucBuf, uint8_t* pucLen) {
	//printf("+%s\r\n",__FUNCTION__);
	if (RmtCtlDisable == 0) {
		return -1;
	}
	if (SetDispParamStep1 || SetDispParamStep2 || SetDispParamStep3) {
		if ((GetTickCount() - SystemTickCnt) >= 3000) {             // 3s未进行按键操作
			uint32_t setDispParamCnt = SetDispParamCnt;
			SetDispParamStep1 = 0;
			SetDispParamStep2 = 0;
			SetDispParamStep3 = 0;
			SetDispParamCnt = 0;
			SetDispKeyCnt = 0;

			uint16_t dispparamID = 0;
			if (setDispParamCnt == 2) {
				printf("++++++++++Set DispParam->AHD720P25\r\n");
				// set ahd
				dispparamID = 1;   // AHD 720P25

				//if (pSysData->avmData.dispParamID != dispparamID) {
				//    if (DispParamSet(dispparamID) == 0) {
				//        avmComCtrlFrame.cmd = CP_CMD_GP_SET2;
				//        avmComCtrlFrame.dataLen = 3;
				//        avmComCtrlFrame.data[0] = 0x02;
				//        avmComCtrlFrame.data[1] = (uint8_t)(dispparamID & 0x00ff);
				//        avmComCtrlFrame.data[2] = (uint8_t)(dispparamID >> 8);
				//        while (xQueueSend(AVMComCtrlQueue, &avmComCtrlFrame, 1000) != pdTRUE);
				//    }
				//}
			}
			else if (setDispParamCnt == 3) {
				printf("++++++++++Set DispParam->NTSC\r\n");
				// set ntsc
				dispparamID = 2;       // NTSC

				//if (pSysData->avmData.dispParamID != dispparamID) {
				//    if (DispParamSet(dispparamID) == 0) {
				//        avmComCtrlFrame.cmd = CP_CMD_GP_SET2;
				//        avmComCtrlFrame.dataLen = 3;
				//        avmComCtrlFrame.data[0] = 0x02;
				//        avmComCtrlFrame.data[1] = (uint8_t)(dispparamID & 0x00ff);
				//        avmComCtrlFrame.data[2] = (uint8_t)(dispparamID >> 8);
				//        while (xQueueSend(AVMComCtrlQueue, &avmComCtrlFrame, 1000) != pdTRUE);
				//    }
				//}
			}
			else if (setDispParamCnt == 4) {
				printf("++++++++++Set DispParam->VGA 672x480@60\r\n");
				// set VGA
				dispparamID = 10;       // VGA 672x480@60

				//if (pSysData->avmData.dispParamID != dispparamID) {
				//    if (DispParamSet(dispparamID) == 0) {
				//        avmComCtrlFrame.cmd = CP_CMD_GP_SET2;
				//        avmComCtrlFrame.dataLen = 3;
				//        avmComCtrlFrame.data[0] = 0x02;
				//        avmComCtrlFrame.data[1] = (uint8_t)(dispparamID & 0x00ff);
				//        avmComCtrlFrame.data[2] = (uint8_t)(dispparamID >> 8);
				//        while (xQueueSend(AVMComCtrlQueue, &avmComCtrlFrame, 1000) != pdTRUE);
				//    }
				//}
			}
			else if (setDispParamCnt == 5) {
				printf("++++++++++Set DispParam->VGA 1280x720@60\r\n");
				// set hdmi
				dispparamID = 23;       // VGA 1280x720@60

				//if (pSysData->avmData.dispParamID != dispparamID) {
				//    if (DispParamSet(dispparamID) == 0) {
				//        avmComCtrlFrame.cmd = CP_CMD_GP_SET2;
				//        avmComCtrlFrame.dataLen = 3;
				//        avmComCtrlFrame.data[0] = 0x02;
				//        avmComCtrlFrame.data[1] = (uint8_t)(dispparamID & 0x00ff);
				//        avmComCtrlFrame.data[2] = (uint8_t)(dispparamID >> 8);
				//        while (xQueueSend(AVMComCtrlQueue, &avmComCtrlFrame, 1000) != pdTRUE);
				//    }
				//}
			}
			else if (setDispParamCnt == 6) {
				printf("++++++++++Set DispParam->HDMI 1280x720@60\r\n");
				// set hdmi
				dispparamID = 4;       // HDMI 1280x720@60
			}
			else {
				return -1;
			}
			*pucCmd = CP_CMD_GP_SET2;
			ucBuf[0] = 0x02;
			ucBuf[1] = (uint8_t)(dispparamID & 0x00ff);
			ucBuf[2] = (uint8_t)(dispparamID >> 8);
			*pucLen = 3;
			return 0;

			//uint8_t tmp = LED_OFF;
			//xQueueSend(LedCtrlQueue, (void*)&tmp, pdMS_TO_TICKS(10));
		}
	}
	return -1;
}


uint32_t RemoteControl::GetRmtCtlId(void) {
	return RmtCtlID;
}

void RemoteControl::Enable(void) {
	RmtCtlDisable = 1;
}

void RemoteControl::Disable(void) {
	RmtCtlDisable = 0;
}

uint32_t RemoteControl::IsEnable(void) {
	return RmtCtlDisable;
}
#endif
