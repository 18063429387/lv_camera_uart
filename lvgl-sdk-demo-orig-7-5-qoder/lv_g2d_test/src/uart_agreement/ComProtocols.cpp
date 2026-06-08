
#ifndef WIN32
#include "ComProt.h"
#include <stdint.h>
#include "ComProtocols.h"
#include "uart.h"
#include "core/bv_types.h"
#include "CSystemConf.h"
#include "control/com_defs.h"

CarInfo_t Com1CarInfo = { CAR_DATA_GEAR_INVALID, 0, 0, CAR_DATA_CAN_SLEEP, 0 };
CarInfo_t Com2CarInfo = { CAR_DATA_GEAR_INVALID, 0, 0, CAR_DATA_CAN_SLEEP, 0 };
CarInfo_t CanCarInfo = { CAR_DATA_GEAR_INVALID, 0, 0, CAR_DATA_CAN_SLEEP, 0 };
uint8_t Com1CarSpeedLast = 0;
uint8_t Com2CarSpeedLast = 0;
uint8_t CanCarSpeedLast = 0;

static int GiRemoteType;

unsigned char crc7(unsigned char ucOri, unsigned char* pucData, unsigned int uiLen) {
	unsigned int  i, j;
	unsigned char ucRlst = ucOri;

	for (i = 0; i < uiLen; i++) {
		for (j = 0; j < 8; j++) {
			ucRlst <<= 1;
			ucRlst ^= ((((pucData[i] << j) ^ ucRlst) & 0x80) ? 0x9 : 0);
		}
	}
	return ((ucRlst & 0x7F));
}

#if 0
int ComRecv_SW(void* pExtCom, unsigned char* pucBuf, int len, int iTimeOut)
{
	int32_t iLen;
	DWORD refSystemTick, tmpSystemTick;
	int timeOut;
	uint32_t times;

	refSystemTick = GetTickCount();
	while (1) {
		tmpSystemTick = GetTickCount() - refSystemTick;
		if (tmpSystemTick < iTimeOut)
			timeOut = iTimeOut - tmpSystemTick;
		else
			timeOut = 0;

		iLen = UartReadBuf((DmaUart_t*)pExtCom, (void*)pucBuf, 1, timeOut);

		if (iLen <= 0) {
			return -1;
		}
		if (pucBuf[0] != 'S') {
			continue;
		}
		tmpSystemTick = GetTickCount() - refSystemTick;
		if (tmpSystemTick < iTimeOut)
			timeOut = iTimeOut - tmpSystemTick;
		else
			timeOut = 0;
		if (timeOut < 2)
			timeOut = 2;

		iLen = UartReadBuf((DmaUart_t*)pExtCom, (void*)&pucBuf[1], 1, timeOut);

		if (iLen <= 0) {
			return -1;
		}
		if (pucBuf[1] != 'W') {
			continue;
		}
		tmpSystemTick = GetTickCount() - refSystemTick;
		if (tmpSystemTick < iTimeOut)
			timeOut = iTimeOut - tmpSystemTick;
		else
			timeOut = 0;
		if (timeOut < 2)
			timeOut = 2;

		iLen = UartReadBuf((DmaUart_t*)pExtCom, (void*)&pucBuf[2], 2, timeOut);

		if (iLen < 2) {
			return -1;
		}
		if (pucBuf[3] > len) {
			return -1;
		}
		tmpSystemTick = GetTickCount() - refSystemTick;
		if (tmpSystemTick < iTimeOut)
			timeOut = iTimeOut - tmpSystemTick;
		else
			timeOut = 0;

		times = ((pucBuf[3] + 1) * 10 * configTICK_RATE_HZ) / pCom->ComBond + 20;    //mini timeout 

		if (timeOut < times)
			timeOut = times;

		iLen = UartReadBuf((DmaUart_t*)pExtCom, (void*)&pucBuf[4], (int)(unsigned int)pucBuf[3] + 1, timeOut);

		if (iLen < ((int)(unsigned int)pucBuf[3] + 1)) {
			return -1;
		}
		if (pucBuf[iLen + 3] != crc7(0, pucBuf, iLen + 3))
			return -1;

		return iLen + 4;
	}
}

int32_t ComRecvUnpack_SW(uint32_t ComType, void* pExtCom, uint8_t* pucCmd, uint8_t* pucData, uint8_t* pucDataLen, uint8_t* pStatus, uint8_t isAsk, int32_t iTimeOut)
{
	uint8_t ucBuf[256];
	int32_t iLen;

	iLen = ComRecv_SW(ComType, pExtCom, ucBuf, sizeof(ucBuf), iTimeOut);
	if (isAsk) {
		SW_AskFrame_t* pData = (SW_AskFrame_t*)ucBuf;
		if (iLen < 7)
			return -1;
		*pucCmd = pData->cmd;
		*pStatus = pData->status;
		memcpy(pucData, pData->buf, iLen - 7);         // head[2],dir,len,cmd,status,crc
		*pucDataLen = iLen - 7;
		return 0;
	}
	else {
		SW_CmdFrame_t* pData = (SW_CmdFrame_t*)ucBuf;
		if (iLen < 6)
			return -1;
		*pucCmd = pData->cmd;
		memcpy(pucData, pData->buf, iLen - 6);     // head[2],dir,len,cmd,crc
		*pucDataLen = iLen - 6;
		return 0;
	}
}

int32_t ProtocolPackComSend(Uart& uartSend, unsigned char* pucBuf, int len, int iTimeOut)
{
	int32_t iRet;

	iRet = uartSend.Write(pucBuf, len);

	return iRet;
}
#endif


int32_t ProtocolSend_SW(CMyQueue& msgQueueSend, unsigned char* pucBuf, int len, int iTimeOut) {
	int32_t iRet;

	iRet = msgQueueSend.PostMsg(pucBuf);

	return iRet;
}
int32_t ProtocolSend_SW(Uart& ExtCom, unsigned char* pucBuf, int len, int iTimeOut) {
	int32_t iRet;

	iRet = ExtCom.Write(pucBuf, len);

	return iRet;
}
int32_t ProtocolPackMsgQueueSend(CMyQueue& msgQueueSend, unsigned char* pucBuf, int len, int iTimeOut)
{
	int32_t iRet;

	iRet = msgQueueSend.PostMsg(pucBuf);

	return iRet;
}

int32_t ProtocolPackSend_SW(Uart& ExtCom, uint8_t ucCmd, uint8_t* pucData, uint8_t ucDataLen, uint8_t ucStatus, uint8_t isAsk) {
	uint8_t ucBuf[256];
	uint8_t len = 0;

	ucBuf[len++] = 'S';
	ucBuf[len++] = 'W';
	ucBuf[len++] = ((CP_DEVICE_ID_BASE & 0x07) << 4) | (CP_DEVICE_ID_360 & 0x07);   // dir
	ucBuf[len++] = ucDataLen + (isAsk ? 2 : 1); // len
	ucBuf[len++] = ucCmd; // cmd
	if (isAsk) {
		ucBuf[len++] = ucStatus;
	}
	memcpy(&ucBuf[len], pucData, ucDataLen);
	len += ucDataLen;
	ucBuf[len] = crc7(0, ucBuf, len);               //crc
	len++;

	return ProtocolSend_SW(ExtCom, ucBuf, len, 0);
}

int32_t ProtocolPackSend_GP2(Uart& ExtCom, uint8_t ucCmd, uint8_t* pucData, uint8_t ucDataLen) {
	uint8_t ucBuf[256];
	uint8_t len = 0;
	uint8_t checksum = 0;

	ucBuf[len++] = 0x2E;            // frame head.
	ucBuf[len++] = ucCmd;           // data type.
	ucBuf[len++] = ucDataLen;       // data length.
	memcpy(&ucBuf[len], pucData, ucDataLen);
	len += ucDataLen;

	for (uint32_t i = 1; i <= (ucDataLen + 2); i++) {
		checksum += ucBuf[i];
	}
	ucBuf[len] = checksum ^ 0xFF;               //checksum
	len++;
	//printf("<==========> %s, line=%d\r\n", __FUNCTION__, __LINE__);
	//for (uint32_t i = 0; i < len; i++) {
	//    printf("%x",ucBuf[i]);
	//}
	//printf("\r\n");

	return ExtCom.Write(ucBuf, len);;
}

int32_t ProtocolPackSend_SW(CMyQueue& msgQueueSend, uint8_t ucCmd, uint8_t* pucData, uint8_t ucDataLen, uint8_t ucStatus, uint8_t isAsk) {
	uint8_t ucBuf[256];
	uint8_t len = 0;

	ucBuf[len++] = 'S';
	ucBuf[len++] = 'W';
	ucBuf[len++] = ((CP_DEVICE_ID_BASE & 0x07) << 4) | (CP_DEVICE_ID_360 & 0x07);   // dir
	ucBuf[len++] = ucDataLen + (isAsk ? 2 : 1); // len
	ucBuf[len++] = ucCmd; // cmd
	if (isAsk) {
		ucBuf[len++] = ucStatus;
	}
	memcpy(&ucBuf[len], pucData, ucDataLen);
	len += ucDataLen;
	ucBuf[len] = crc7(0, ucBuf, len);               //crc
	len++;

	return ProtocolSend_SW(msgQueueSend, ucBuf, len, 0);
}

int32_t ComProtocolSendCmd(CMyQueue& msgQueueSend, uint8_t ucCmd, uint8_t* pucData, uint8_t ucDataLen) {
	return ProtocolPackSend_SW(msgQueueSend, ucCmd, pucData, ucDataLen, 0, 0);
}
int32_t ComProtocolSendCmd(Uart& ExtCom, uint8_t ucCmd, uint8_t* pucData, uint8_t ucDataLen) {
	if (!ExtCom.IsOpen()) {
		return -1;
	}
	return ProtocolPackSend_SW(ExtCom, ucCmd, pucData, ucDataLen, 0, 0);
}
int32_t ComProtocolSendAsk(Uart& ExtCom, uint8_t ucCmd, uint8_t* pucData, uint8_t ucDataLen, uint8_t ucStatus) {
	if (!ExtCom.IsOpen()) {
		return -1;
	}
	return ProtocolPackSend_SW(ExtCom, ucCmd, pucData, ucDataLen, ucStatus, 1);
}

int32_t ComProtocolRecv(Uart& ExtCom, uint8_t* pucBuf, uint8_t ucLen, int timeoutms = 20)
{
	int ret = 0;
	int left = ucLen;
	int count = 0;
	int t = ucLen;

	do {
		left = ucLen - count;
		ret = ExtCom.Read(&pucBuf[count], left, timeoutms);
		if (ret > 0) {
			//for (int i=0; i<ret; i++)
			//{
			//    printf("0x%02x ", pucBuf[count+i]);
			//}
			count += ret;
		}
	} while ((--t > 0) && (left > 0));
	//if (count > 0)
	//{
	//    printf(", %d\n", count);
	//}

	return count;
}


int32_t ComProtocolRecv_SW(Uart& ExtCom, unsigned char* pucBuf, int len, int iTimeout)
{
	int32_t iLen;
	DWORD refSystemTick, tmpSystemTick;
	int timeOut;
	int32_t times;

	refSystemTick = GetTickCount();
	while (1) {
		tmpSystemTick = GetTickCount() - refSystemTick;
		if (iTimeout < 0) {
			timeOut = -1;
		}
		else if (tmpSystemTick < iTimeout)
			timeOut = iTimeout - tmpSystemTick;
		else
			timeOut = 0;
		iLen = ExtCom.Read(pucBuf, 1, timeOut);
		if (iLen <= 0) {
			return -1;
		}
		if (pucBuf[0] != 'S') {
			continue;
		}
		//printf("======Head S is receive ok.=====\r\n");

		iLen = ExtCom.Read(&pucBuf[1], 1, 50);
		if (iLen <= 0) {
			continue;
		}
		if (pucBuf[1] != 'W') {
			continue;
		}
		//printf("======Head W is receive ok.=====\r\n");
		iLen = ComProtocolRecv(ExtCom, &pucBuf[2], 2, 50);
		if (iLen < 2) {
			printf("recv len %d != %d bytes\n", iLen, 2);
			continue;
		}
		//printf("======cmd is receive ok.=====\r\n");

		if (pucBuf[3] > len) {
			continue;
		}
		//printf("======len is receive ok.=====\r\n");
		iLen = ComProtocolRecv(ExtCom, &pucBuf[4], pucBuf[3] + 1, 20);
		if (iLen < pucBuf[3] + 1) {
			printf("recv data %d != %d bytes\n", iLen, pucBuf[3] + 1);
			continue;
		}
		//printf("======datas is receive ok.=====\r\n");
		uint8_t tmp = crc7(0, pucBuf, iLen + 3);
		if (pucBuf[iLen + 3] != tmp) {
			printf("======crc error!! need %d ,now is %d.=====\r\n", pucBuf[iLen + 3], tmp);
			continue;
		}
		//printf("======crc is receive ok.=====\r\n");
		return iLen + 4;
	}
}

/*
概述：该函数用于通过串口接收符合GP2协议的数据包，并进行校验和验证，返回有效数据包的长度。

参数：
ExtCom：串口对象引用，用于数据接收
pucBuf：接收数据的缓冲区指针
ucLen：接收缓冲区的长度
iTimeout：接收超时时间（毫秒），-1表示无限等待

返回值：
成功时返回接收到的数据包长度（包括校验和）
失败时继续尝试接收，直到超时或接收到有效数据包
*/
int32_t ComProtocolRecv_GP2(Uart& ExtCom, uint8_t* pucBuf, uint8_t ucLen, int32_t iTimeout) {
	int32_t iLen;
	DWORD refSystemTick, tmpSystemTick;
	int timeOut;
	uint8_t checksum;
	uint32_t i;

	refSystemTick = GetTickCount();
	while (1) {
		tmpSystemTick = GetTickCount() - refSystemTick;
		if (iTimeout < 0) {
			timeOut = -1;
		}
		else if (tmpSystemTick < iTimeout)
			timeOut = iTimeout - tmpSystemTick;
		else
			timeOut = 0;
		iLen = ExtCom.Read(pucBuf, 1, timeOut);
		if (iLen <= 0) {
			return -1;
		}
		if (pucBuf[0] != 0x2e) {
			continue;
		}
		//printf("======Head 0x2e is receive ok.=====\r\n");
        iLen = ComProtocolRecv(ExtCom, &pucBuf[1], 2);
		if (iLen < 2) {
			continue;
		}
		if ((pucBuf[2] + 1) > ucLen) {
			continue;
		}
		//printf("======Data len is receive ok.0x%02x=====\r\n", pucBuf[2]);
		timeOut = ((pucBuf[2] + 1) * 10 * 1000) / 19200 + 20;    //mini timeout 

        iLen = ComProtocolRecv(ExtCom, &pucBuf[3], pucBuf[2] + 1);
		if (iLen < (pucBuf[2] + 1)) {
			continue;
		}
		checksum = 0;
		for (i = 1; i <= (pucBuf[2] + 2); i++) {
			checksum += pucBuf[i];
		}
		checksum ^= 0xff;

		if (checksum != pucBuf[pucBuf[2] + 3]) {
			//printf("======crc error!! need %d ,now is %d.=====\r\n", pucBuf[pucBuf[2] + 3], checksum);
			//for (i = 0; i < pucBuf[2] + 4; i++) {
			//    printf("0x%02x ", pucBuf[i]);
			//}
			//printf("\r\n");
			continue;
		}
		//printf("======crc is receive ok.=====\r\n");

		return pucBuf[2] + 3;
	}
}

int32_t ComProtocolRecvXCP(Uart& ExtCom, uint8_t* pucBuf, uint8_t ucLen, int32_t iTimeout) {
	int32_t iLen;
	DWORD refSystemTick, tmpSystemTick;
	int timeOut;
	uint8_t checksum;
	uint32_t i;

	refSystemTick = GetTickCount();
	while (1) {
		tmpSystemTick = GetTickCount() - refSystemTick;
		if (iTimeout < 0) {
			timeOut = -1;
		}
		else if (tmpSystemTick < iTimeout)
			timeOut = iTimeout - tmpSystemTick;
		else
			timeOut = 0;

		iLen = ExtCom.Read(pucBuf, 1, timeOut);
		if (iLen <= 0) {
			return -1;
		}
		if (pucBuf[0] != 0xAB) {
			continue;
		}

		int retry = 10;
		bool match = false;
		do {
			uint8_t head;
			int ret = ExtCom.Read(&head, 1, timeOut);
			if (ret <= 0) {
				return -1;
			}
			if (head == 0xCD) {
				pucBuf[1] = head;
				match = true;
				break;
			}
			else if (head != 0xAB) {
				break;
			}
		} while (retry--);

		if (!match) {
			continue;
		}

		iLen = ComProtocolRecv(ExtCom, &pucBuf[2], 2);
		if (iLen < 2) {
			continue;
		}
		if ((pucBuf[3] + 1) > ucLen) {
			continue;
		}
		//printf("======Data len is receive ok.0x%02x=====\r\n", pucBuf[2]);
		timeOut = ((pucBuf[2] + 1) * 10 * 1000) / 19200 + 20;    //mini timeout 

		iLen = ComProtocolRecv(ExtCom, &pucBuf[4], pucBuf[3] + 1);
		if (iLen < (pucBuf[3] + 1)) {
			continue;
		}
		checksum = 0;
		for (i = 2; i <= (pucBuf[4] + 3); i++) {
			checksum += pucBuf[i];
		}
		checksum ^= 0xff;

		if (checksum != pucBuf[pucBuf[4] + 4]) {
			//printf("======crc error!! need %d ,now is %d.=====\r\n", pucBuf[pucBuf[2] + 3], checksum);
			//for (i = 0; i < pucBuf[2] + 4; i++) {
			//    printf("0x%02x ", pucBuf[i]);
			//}
			//printf("\r\n");
			continue;
		}
		//printf("======crc is receive ok.=====\r\n");

		return pucBuf[2] + 4;
	}
}


int32_t ComProtocolRecv_GP1(Uart& ExtCom, uint8_t* pucBuf, uint8_t ucLen, int32_t iTimeout)
{
	int32_t iLen;
	DWORD refSystemTick, tmpSystemTick;
	int timeOut;
	uint8_t tmp;
	uint8_t checksum;
	uint32_t i;
	uint8_t buf[2];

	refSystemTick = GetTickCount();
	while (1) {
		tmpSystemTick = GetTickCount() - refSystemTick;
		if (iTimeout < 0) {
			timeOut = -1;
		}
		else if (tmpSystemTick < iTimeout)
			timeOut = iTimeout - tmpSystemTick;
		else
			timeOut = 0;
		iLen = ExtCom.Read(pucBuf, 1, timeOut);
		if (iLen <= 0) {
			return -1;
		}
		if (pucBuf[0] != 0x21) {
			continue;
		}

		iLen = ExtCom.Read(&pucBuf[1], 2, 20);
		if (iLen < 2) {
			return -1;
		}
		if ((pucBuf[1] + 1) > ucLen) {            // buf[0] -- data length, buf[1] -- data type
			continue;
		}

		timeOut = ((pucBuf[1] + 1) * 10 * 1000) / 19200 + 20;    //mini timeout 
		iLen = ExtCom.Read(&pucBuf[3], pucBuf[1] + 1, timeOut);
		if (iLen < (pucBuf[1] + 1)) {
			continue;
		}
		checksum = 0;
		for (i = 0; i < pucBuf[1] + 3; i++) {
			checksum ^= pucBuf[i];
		}
		if (checksum != pucBuf[pucBuf[1] + 3])
			continue;

		return pucBuf[1] + 3;
	}
}

int32_t ComRecvCmd_SW(Uart& ExtCom, uint8_t* pCmd, uint8_t* pData, uint8_t* pLen, int32_t iTimeout) {
	uint8_t ucBuf[256];
	int32_t iLen = 0;

	iLen = ComProtocolRecv_SW(ExtCom, ucBuf, 128, iTimeout);
	if (iLen >= 6) {
		SW_CmdFrame_t* pSwCmdFrame = (SW_CmdFrame_t*)ucBuf;

		*pCmd = pSwCmdFrame->cmd;
		*pLen = pSwCmdFrame->len - 1;
		memcpy(pData, pSwCmdFrame->buf, *pLen);
		return 0;
	}

	return -1;
}

int32_t IR_Receive_TimeStamp(IrRx& ExtCom, uint8_t* pucBuf, uint8_t ucLen, int32_t iTimeout)
{
#define TIME1_US    900000LL		// 为兼容无线转红外遥控器，按下会持续发送600ms以上的重复码，故将长按延时判断增大到900ms
#define TIME2_US    100000LL
#define TIME_INVALID_US    30000000LL
    int iRet;
    struct input_event inputEvent;                      // 当前按键信息
    static struct input_event inputEventRecord = {0};   // 用于记录按下键的信息
    static struct input_event inputEventRecord2 = {0};  // 用于记录XXXff00帧的信息
    static int timesRecord = -1;                        // 用于记录长按时下发序号
    bool bPressStart = false;                           // 标记是否是开始按下

    iRet = ExtCom.Read((unsigned char *)&inputEvent, sizeof(inputEvent), iTimeout);
    if (iRet == sizeof(inputEvent)) {
        // 红外遥控
        if ((inputEvent.value & 0xffffff) == 0xff80 || 
            ((inputEvent.value & 0xffffff) == 0xff00 && (inputEvent.value & 0xff000000) != 0xfb000000)) {  // 释放或长按发XXXff00,与诺维达、鼎威、方易通语音控制有共同尾号，故需排除
            //printf("%x, %lld, %lld\n", inputEvent.value, inputEvent.time.tv_sec, inputEvent.time.tv_usec);
            uint8_t tmp = inputEvent.value >> 24;
            // 无线转红外旋钮的左(0xee)、右(0x99)旋转直接响应，不作过滤处理
            if ( (tmp == 0x99 || tmp == 0xee)) {
                if ((inputEvent.value & 0xffffff) == 0xff80) {
                    // 20240401 由于某些界面如标定描点需左右旋，且0x90与享车派红外遥控器键值冲突，故此处不转换成0x90、0xe0，而用0x99、0xee作为左右旋转
                    //inputEvent.value = inputEvent.value & 0xf0ffffff;   // 转换成0x90或0xe0
                    memcpy(pucBuf, &inputEvent.value, sizeof(inputEvent.value));
                    return sizeof(inputEvent.value);
                }
                return -1;
            }

            if (!(inputEvent.value & 0xFF)) {
                // 记录XXXff00帧的时间戳
                inputEventRecord2 = inputEvent;
            } else {
                long long timeDiffus = (inputEvent.time.tv_sec-inputEventRecord2.time.tv_sec)*1e6 + 
                    (inputEvent.time.tv_usec-inputEventRecord2.time.tv_usec);
                if (timeDiffus > 1000) {          // 实测长按时XXXff80与上一个XXXff00帧的时间戳相同，所以如果时间间隔>1000时必然是重新按下了某键。
                    bPressStart = true;
                }
            }

            inputEvent.value = (inputEvent.value & 0xFFFFFF00) | 0x80;
            if (bPressStart || inputEvent.value != (inputEventRecord.value)) {     // 新键按下即转发
                memcpy(pucBuf, &inputEvent.value, sizeof(inputEvent.value));
                inputEventRecord = inputEvent;
                timesRecord = -1;
                //printf("timesRecord=%d\n", timesRecord);
                return sizeof(inputEvent.value);
            } else {                                                // 旧键长按时,按间隔TIME1_US、0TIME2_US、TIME2_US……转发
                uint8_t tmp = inputEvent.value >> 24;
                // power键不支持长按
                if (tmp == 0xa2/* || tmp == 0xa8*/) {	// 0xa8即enter 键支持长按，用于退出和同显控制
                    return -1;
                }

                if (inputEventRecord.time.tv_sec <= inputEvent.time.tv_sec) {
                    long long timeDiffus = (inputEvent.time.tv_sec-inputEventRecord.time.tv_sec)*1e6 + 
                                           (inputEvent.time.tv_usec-inputEventRecord.time.tv_usec);
                    if (timeDiffus > TIME1_US && timeDiffus < TIME_INVALID_US) {
                        long long lTimes = (timeDiffus - TIME1_US ) / TIME2_US;
                        if (lTimes != timesRecord) {
                            memcpy(pucBuf, &inputEvent.value, sizeof(inputEvent.value));
                            timesRecord = lTimes;
                            //printf("timesRecord=%d\n", timesRecord);
                            return sizeof(inputEvent.value);
                        }
                    }
                    return -1;
                }
                inputEventRecord = inputEvent;
                return -1;
            }
        } else {
            // 其它设备还按原先的处理方式进行
            memcpy(pucBuf, &inputEvent.value, sizeof(inputEvent.value));
            return sizeof(inputEvent.value);
        }
    }

    return -1;
}

int32_t IRRecvCmd_NEC_SW(IrRx& IrDev, uint8_t* pucCmd, uint8_t* pucData, uint8_t* pucDatalen, int32_t iTimeout) {
    uint32_t irData;
    int32_t iLen = 0;
    iLen = IR_Receive_TimeStamp(IrDev, (uint8_t*)&irData, 4, iTimeout);
    if (iLen == 4) {
        if ((irData & 0xffffff) == 0xffff80) {
            uint8_t tmp;
            tmp = irData >> 24;
            if (tmp == 0x80) {
                pucData[0] = 0x09;                  // power
                *pucDatalen = 1;
            } else if (tmp == 0x10) {
                pucData[0] = 0x03;                  // enter
                *pucDatalen = 1;
            } else if (tmp == 0xe0) {
                // left
                *pucDatalen = 1;
                pucData[0] = 0x07;
            } else if (tmp == 0x90) {
                // right
                *pucDatalen = 1;
                pucData[0] = 0x08;
            }  else if (tmp == 0xa0) {
                // up
                *pucDatalen = 1;
                pucData[0] = 0x05;
            }  else if (tmp == 0xd0) {
                // down
                *pucDatalen = 1;
                pucData[0] = 0x06;
            }  else if (tmp == 0xb0) {
                pucData[0] = 0x04;                  // return
                *pucDatalen = 1;
            } else {
                return -1;
            }
            *pucCmd = CP_CMD_KEY;
            return 0;
        } else if ((irData & 0xffffff) == 0x4fb80) {
            uint8_t tmp;
            tmp = irData >> 24;
            if (tmp == 0x30) {
                pucData[0] = 0x09;                  // power
                *pucDatalen = 1;
            } else if (tmp == 0xd0) {
                pucData[0] = 0x03;                  // enter
                *pucDatalen = 1;
            } else if (tmp == 0x08) {
                // left
                *pucDatalen = 1;
                pucData[0] = 0x07;
            } else if (tmp == 0x28) {
                // right
                *pucDatalen = 1;
                pucData[0] = 0x08;
            }  else if (tmp == 0x90) {
                // up
                *pucDatalen = 1;
                pucData[0] = 0x05;
            }  else if (tmp == 0x80) {
                // down
                *pucDatalen = 1;
                pucData[0] = 0x06;
            }  else if (tmp == 0x10) {
                pucData[0] = 0x04;                  // return
                *pucDatalen = 1;
            } else {
                return -1;
            }
            *pucCmd = CP_CMD_KEY;
            return 0;
        } else if ((irData & 0xffffff) == 0xff80) {
            // 享车派红外值: 1:0x08; 2:0x88; 3:0x48; 4:0x28; 5:0xa8; 6:0x68; 7:0x18; 8:0x98; 9:0x58
            uint8_t tmp;
            tmp = irData >> 24;
            // 以下tmp的第二个值,如tmp == 0x00是车享的红外遥控器值
            if (tmp == 0xa2 || tmp == 0x00) {
                pucData[0] = 0x09;                  // power
                *pucDatalen = 1;
            } else if (tmp == 0xa8 || tmp == 0xa0) {
                pucData[0] = 0x03;                  // enter
                *pucDatalen = 1;
            } else if (tmp == 0xe0 || tmp == 0x20) {
                // left
                *pucDatalen = 1;
                pucData[0] = 0x07;
            } else if (tmp == 0x60) {
                // right
                *pucDatalen = 1;
                pucData[0] = 0x08;
            } else if (tmp == 0x02 || tmp == 0x80) {
                // up
                *pucDatalen = 1;
                pucData[0] = 0x05;
            } else if (tmp == 0x98) {
                // down
                *pucDatalen = 1;
                pucData[0] = 0x06;
            } else if (tmp == 0x30 || tmp == 0xb0 || tmp == 0x40) {			// 0x40用于享车派红外遥控器
                pucData[0] = 0x04;                  // return
                *pucDatalen = 1;
            } else if (tmp == 0x7a || tmp == 0x70) {
                pucData[0] = 0x02;                  // menu
                *pucDatalen = 1;
            } else if (tmp == 0x90) {
				if (GiRemoteType == REMOTE_TYPE_XCP)
				{
					// down
					*pucDatalen = 1;
					pucData[0] = 0x06;
				}
				else
				{
					// right
					*pucDatalen = 1;
					pucData[0] = 0x08;
				}
			}/*else if (tmp == 0x40) {
                // tmp == 0x40 是车享红外遥控器的DEL键值，暂未启动。
            }*/
            else if (tmp == 0x99 || tmp == 0xee) {
                *pucDatalen = 3;
                pucData[0] = 0x30;
                pucData[1] = (tmp == 0xee) ? 0x1 : 0x0;     // 0xee:turn left; 0x99: turn right.
                pucData[2] = 0x01;
            }
            else if (tmp == 0x08) {
                *pucDatalen = 1;
                pucData[0] = BV_KEY_NUM_1;
            }
            else if (tmp == 0x88) {
                *pucDatalen = 1;
                pucData[0] = BV_KEY_NUM_2;
            }
            else if (tmp == 0x48) {
                *pucDatalen = 1;
                pucData[0] = BV_KEY_NUM_3;
            }
            else if (tmp == 0x28) {
                *pucDatalen = 1;
                pucData[0] = BV_KEY_NUM_4;
            }
            else if (tmp == 0x68) {
                *pucDatalen = 1;
                pucData[0] = BV_KEY_NUM_6;
            }
            else if (tmp == 0x18) {
                *pucDatalen = 1;
                pucData[0] = BV_KEY_NUM_7;
            }
            else if (tmp == 0x58) {
                *pucDatalen = 1;
                pucData[0] = BV_KEY_NUM_9;
            }

            else {
                return -1;
            }
            *pucCmd = CP_CMD_KEY;
            return 0;
        }
        else if ((irData & 0xff00ffff) == 0xfb00ff00) {		// 车享语音模块
            uint16_t tmp;
            tmp = irData >> 16;
            if (tmp == 0xfb03) {		// 车享语音模块，打开全景
                *pucDatalen = 2;
                pucData[0] = 0x32;
                pucData[1] = 0x00;
            }
            else if (tmp == 0xfb02) {	// 车享语音模块，关闭全景
                *pucDatalen = 2;
                pucData[0] = 0x32;
                pucData[1] = 0x01;
            }
            else if (tmp == 0xfb05) {	// 车享语音模块，打开前视
                *pucDatalen = 2;
                pucData[0] = 0x32;
                pucData[1] = 0x02;
            }
            else if (tmp == 0xfb06) {	// 车享语音模块，打开后视
                *pucDatalen = 2;
                pucData[0] = 0x32;
                pucData[1] = 0x03;
            }
            else if (tmp == 0xfb07) {	// 车享语音模块，打开左视
                *pucDatalen = 2;
                pucData[0] = 0x32;
                pucData[1] = 0x04;
            }
            else if (tmp == 0xfb08) {	// 车享语音模块，打开右视
                *pucDatalen = 2;
                pucData[0] = 0x32;
                pucData[1] = 0x05;
            }
            else if (tmp == 0xfb09) {	// 车享语音模块，前视放大
                *pucDatalen = 2;
                pucData[0] = 0x32;
                pucData[1] = 0x0E;
            }
            else if (tmp == 0xfb10) {	// 车享语音模块，后视放大
                *pucDatalen = 2;
                pucData[0] = 0x32;
                pucData[1] = 0x0F;
            }
            else if (tmp == 0xfb11) {	// 车享语音模块，左视放大
                //printf("Left zoom\n");
                return -1;
            }
            else if (tmp == 0xfb12) {	// 车享语音模块，右视放大
                //printf("Right zoom\n");
                return -1;
            }
            else if (tmp == 0xfb13) {	// 车享语音模块，限宽模式
                *pucDatalen = 2;
                pucData[0] = 0x32;
                pucData[1] = 0x07;
            }
            else if (tmp == 0xfb14) {	// 车享语音模块，前后左右
                //printf("Front+Rear+Left+Right\n");
                return -1;
            }
            else {
                return -1;
            }

            *pucCmd = CP_CMD_KEY;
            return 0;
        }
        else if ((irData & 0xffffff) == 0x40bf80) {
            uint8_t tmp = irData >> 24;
            //fprintf(stderr, "%s, %d, code=0x%04x\n", __func__, __LINE__, tmp);
            if (tmp == 0x50) {			// 车享语音模块，3D环绕一圈
                *pucDatalen = 2;
                pucData[0] = 0x32;
                pucData[1] = 0x1b;
            }
            else if (tmp == 0xA0) {		// 车享语音模块，切换3D模式
                *pucDatalen = 2;
                pucData[0] = 0x32;
                pucData[1] = 0x0b;
            }
            else if (tmp == 0x40) {		// 车享语音模块，3D左旋
                *pucDatalen = 2;
                pucData[0] = 0x32;
                pucData[1] = 0x0C;
            }
            else if (tmp == 0xC0) {		// 车享语音模块，3D右旋
                *pucDatalen = 2;
                pucData[0] = 0x32;
                pucData[1] = 0x0D;
            }
            else if (tmp == 0x60) {		// 车享语音模块，前视流媒体
                *pucDatalen = 2;
                pucData[0] = 0x32;
                pucData[1] = 0x08;
            }
            else if (tmp == 0xE0) {		// 车享语音模块，后视流媒体
                *pucDatalen = 2;
                pucData[0] = 0x32;
                pucData[1] = 0x09;
            }
            else if (tmp == 0x10) {		// 车享语音模块，打开第5路摄像头
                //printf("Open the fifth camera\n");
                return -1;
            }
            else if (tmp == 0x90) {		// 车享语音模块，打开第6路摄像头
                //printf("Open the sixth camera\n");
                return -1;
            }
            else {
                return -1;
            }

            *pucCmd = CP_CMD_KEY;
            return 0;
        }
    }
    return -1;
}

int32_t IR_Receive(IrRx& ExtCom, uint8_t* pucBuf, uint8_t ucLen, int32_t iTimeout)
{
	return ExtCom.Read(pucBuf, ucLen, iTimeout);
}

int32_t IRRecvCmd_NEC(IrRx& IrDev, uint8_t* pucCmd, uint8_t* pucData, uint8_t* pucDatalen, int32_t iTimeout) {
	uint32_t irData;
	int32_t iLen = 0;
	iLen = IR_Receive(IrDev, (uint8_t*)&irData, 4, iTimeout);
	if (iLen == 4) {
		if ((irData & 0xffff) == 0x55aa) {              // 鼎威、方易通车机IR触摸
			uint32_t X, Y;
			pucData[0] = 0x01;
			Y = irData >> 24;
			X = (irData >> 16) & 0xff;
			X = (X * 4095) / 255;
			Y = (Y * 4095) / 255;
			pucData[1] = (uint8_t)((X & 0xFFFF) >> 8);
			pucData[2] = (uint8_t)((X & 0xFFFF) >> 0);
			pucData[3] = (uint8_t)((Y & 0xFFFF) >> 8);
			pucData[4] = (uint8_t)((Y & 0xFFFF) >> 0);
			*pucCmd = CP_CMD_TOUCHCOOR;
			*pucDatalen = 5;
			return 0;
		}
		else if ((irData & 0xffff) == 0xff00) {                   // 诺维达、鼎威、方易通语音控制
			if ((irData & 0xff000000) == 0xfb000000) {
				uint8_t tmp;
				tmp = (irData >> 16) & 0xff;
				if (tmp == 0x03) {
					pucData[1] = 0;
				}
				else if (tmp == 0x02) {
					pucData[1] = 1;
				}
				else if (tmp > 0x04 && tmp < 0x09) {
					pucData[1] = tmp - 3;
				}
				else if (tmp == 0x13) {
					pucData[1] = 0x06;              // 打开窄道模式
				}
				else {
					return -1;
				}
				*pucCmd = CP_CMD_KEY;
				*pucDatalen = 2;
				pucData[0] = 0x32;
				return 0;
			}
		}
		else if ((irData & 0xffff) == 0xfd02) {
			uint16_t tmp;
			tmp = irData >> 16;
			if (tmp == 0xf906) {
				pucData[1] = 0x08;                  // 前流媒体
				*pucDatalen = 2;
				pucData[0] = 0x32;
			}
			else if (tmp == 0xf807) {
				pucData[1] = 0x09;                  // 后流媒体
				*pucDatalen = 2;
				pucData[0] = 0x32;
			}
			else if (tmp == 0xfc03) {
				*pucDatalen = 3;
				pucData[0] = 0x30;                  // 左旋
				pucData[1] = 0x1;
				pucData[2] = 0x01;
			}
			else if (tmp == 0xfd02) {
				*pucDatalen = 3;
				pucData[0] = 0x30;                  // clockwise // 右旋
				pucData[1] = 0x0;
				pucData[2] = 0x01;
			}
			else {
				return -1;
			}
			*pucCmd = CP_CMD_KEY;
			return 0;
		}
		else if ((irData & 0xffff0000) == 0xbb660000) {           // 诺维达车机IR触摸
			uint32_t X, Y;
			pucData[0] = 0x01;
			Y = irData & 0xff;
			X = (irData >> 16) & 0xff;
			X = (X * 4095) / 255;
			Y = (Y * 4095) / 255;
			pucData[1] = (uint8_t)((X & 0xFFFF) >> 8);
			pucData[2] = (uint8_t)((X & 0xFFFF) >> 0);
			pucData[3] = (uint8_t)((Y & 0xFFFF) >> 8);
			pucData[4] = (uint8_t)((Y & 0xFFFF) >> 0);
			*pucCmd = CP_CMD_TOUCHCOOR;
			*pucDatalen = 5;
			return 0;
		}
		else if ((irData & 0xffffff) == 0xffff80) {
			uint8_t tmp;
			tmp = irData >> 24;
			if (tmp == 0x80) {
				pucData[0] = 0x09;                  // power
				*pucDatalen = 1;
			}
			else if (tmp == 0x10) {
				pucData[0] = 0x03;                  // enter
				*pucDatalen = 1;
			}
			else if (tmp == 0xe0) {
				// left -> turn left
				*pucDatalen = 3;
				pucData[0] = 0x30;
				pucData[1] = 0x1;
				pucData[2] = 0x01;
			}
			else if (tmp == 0x90) {
				// right -> turn right
				*pucDatalen = 3;
				pucData[0] = 0x30;
				pucData[1] = 0x0;
				pucData[2] = 0x01;

			}
			else if (tmp == 0xa0) {
				// up -> turn left
				*pucDatalen = 3;
				pucData[0] = 0x30;
				pucData[1] = 0x1;
				pucData[2] = 0x01;
			}
			else if (tmp == 0xd0) {
				// down -> turn right
				*pucDatalen = 3;
				pucData[0] = 0x30;
				pucData[1] = 0x0;
				pucData[2] = 0x01;
			}
			else if (tmp == 0xb0) {
				pucData[0] = 0x04;                  // return
				*pucDatalen = 1;
			}
			else if (tmp == 0x7a) {
				pucData[0] = 0x02;                  // menu
				*pucDatalen = 1;
			}
			else {
				return -1;
			}
			*pucCmd = CP_CMD_KEY;
			return 0;
		}
		else if ((irData & 0xffffff) == 0x4fb80) {
			uint8_t tmp;
			tmp = irData >> 24;
			if (tmp == 0x30) {
				pucData[0] = 0x09;                  // power
				*pucDatalen = 1;
			}
			else if (tmp == 0xd0) {
				pucData[0] = 0x03;                  // enter
				*pucDatalen = 1;
			}
			else if (tmp == 0x08) {
				// left -> turn left
				*pucDatalen = 3;
				pucData[0] = 0x30;
				pucData[1] = 0x1;
				pucData[2] = 0x01;
			}
			else if (tmp == 0x28) {
				// right -> turn right
				*pucDatalen = 3;
				pucData[0] = 0x30;
				pucData[1] = 0x0;
				pucData[2] = 0x01;

			}
			else if (tmp == 0x90) {
				// up -> turn left
				*pucDatalen = 3;
				pucData[0] = 0x30;
				pucData[1] = 0x1;
				pucData[2] = 0x01;
			}
			else if (tmp == 0x80) {
				// down -> turn right
				*pucDatalen = 3;
				pucData[0] = 0x30;
				pucData[1] = 0x0;
				pucData[2] = 0x01;
			}
			else if (tmp == 0x10) {
				pucData[0] = 0x04;                  // return
				*pucDatalen = 1;
			}
			else {
				return -1;
			}
			*pucCmd = CP_CMD_KEY;
			return 0;
		}
		else if ((irData & 0xffffff) == 0xff80) {
			uint8_t tmp;
			tmp = irData >> 24;
			// 以下tmp的第二个值,如tmp == 0x00是车享的红外遥控器值
			if (tmp == 0xa2 || tmp == 0x00) {
				pucData[0] = 0x09;                  // power
				*pucDatalen = 1;
			}
			else if (tmp == 0xa8 || tmp == 0xa0) {
				pucData[0] = 0x03;                  // enter
				*pucDatalen = 1;
			}
			else if (tmp == 0xe0 || tmp == 0x20) {
				// left -> turn left
				*pucDatalen = 3;
				pucData[0] = 0x30;
				pucData[1] = 0x1;
				pucData[2] = 0x01;
			}
			else if (tmp == 0x90 || tmp == 0x60) {
				// right -> turn right
				*pucDatalen = 3;
				pucData[0] = 0x30;
				pucData[1] = 0x0;
				pucData[2] = 0x01;

			}
			else if (tmp == 0x02 || tmp == 0x80) {
				// up -> turn left
				*pucDatalen = 3;
				pucData[0] = 0x30;
				pucData[1] = 0x1;
				pucData[2] = 0x01;
			}
			else if (tmp == 0x98 || tmp == 0x90) {
				// down -> turn right
				*pucDatalen = 3;
				pucData[0] = 0x30;
				pucData[1] = 0x0;
				pucData[2] = 0x01;
			}
			else if (tmp == 0x30 || tmp == 0xb0 || tmp == 0x40) {			// 0x40用于享车派红外遥控器
				pucData[0] = 0x04;                  // return
				*pucDatalen = 1;
			}
			else if (tmp == 0x7a || tmp == 0x70) {
				pucData[0] = 0x02;                  // menu
				*pucDatalen = 1;
			}/*else if (tmp == 0x40) {
				// tmp == 0x40 是车享红外遥控器的DEL键值，暂未启动。
				printf("%s: DEL\n", __FUNCTION__);
			}*/
			else {
				return -1;
			}
			*pucCmd = CP_CMD_KEY;
			return 0;
		}
	}
	return -1;
}

int32_t CalCorner(uint8_t corner)
{
#define MID_CORNER      (0x80)
#define MAX_CORNER      (34000l)
	int32_t cornerData;
	uint8_t tmp;

	tmp = corner;
	if (tmp >= MID_CORNER) {
		cornerData = (tmp - MID_CORNER) * ((int32_t)MAX_CORNER / MID_CORNER);
	}
	else {
		cornerData = (int32_t)MAX_CORNER - (tmp * ((int32_t)MAX_CORNER / MID_CORNER));
		cornerData = 0 - cornerData;
	}

	return  cornerData;
}

int SetParamByIndex(int index, DispParam_t* pParam)
{
	printf("param cmd (0x0B) index = 0x%x\n", index);
	pParam->rotate = 0;
	switch (index) {
	case 0x03:
		pParam->dispmode = Mode_VGA;
		pParam->pclk = 74250000;
		pParam->x = 1280;
		pParam->y = 720;
		pParam->hbp = 220;
		pParam->hfp = 110;
		pParam->hspw = 40;
		pParam->ht = 1650;
		pParam->vbp = 20;
		pParam->vfp = 5;
		pParam->vspw = 5;
		pParam->vt = 750;
		pParam->hpolarity = 1;
		pParam->vpolarity = 1;
		pParam->clk_phase = 2;
		pParam->lvds_mode = 0;
		pParam->lvds_link = 0;
		break;

	case 0x85:
		pParam->dispmode = Mode_VGA;
		pParam->pclk = 40000000;
		pParam->x = 800;
		pParam->y = 480;
		pParam->hbp = 128;
		pParam->hfp = 40;
		pParam->hspw = 88;
		pParam->ht = 1056;
		pParam->vfp = 20;
		pParam->vbp = 124;
		pParam->vspw = 4;
		pParam->vt = 628;
		pParam->hpolarity = 1;
		pParam->vpolarity = 1;
		pParam->clk_phase = 2;
		pParam->lvds_mode = 0;
		pParam->lvds_link = 0;
		break;

	case 0x88:
		pParam->dispmode = Mode_VGA;
		pParam->pclk = 48000000;
		pParam->x = 1920;
		pParam->y = 720;
		pParam->hbp = 48;
		pParam->hfp = 41;
		pParam->hspw = 40;
		pParam->ht = 2049;
		pParam->vbp = 25;
		pParam->vfp = 25;
		pParam->vspw = 10;
		pParam->vt = 780;
		pParam->hpolarity = 1;
		pParam->vpolarity = 1;
		pParam->clk_phase = 2;
		pParam->lvds_mode = 0;
		pParam->lvds_link = 0;
		break;
	case 0x89:
		pParam->dispmode = Mode_VGA;
		pParam->pclk = 99000000;
		pParam->x = 1920;
		pParam->y = 720;
		pParam->hbp = 58;
		pParam->hfp = 31;
		pParam->hspw = 40;
		pParam->ht = 2049;
		pParam->vbp = 50;
		pParam->vfp = 24;
		pParam->vspw = 4;
		pParam->vt = 798;
		pParam->hpolarity = 1;
		pParam->vpolarity = 1;
		pParam->clk_phase = 2;
		pParam->lvds_mode = 0;
		pParam->lvds_link = 0;
		break;
	// OPRT RES H768 1024p60
	case 0x8A:
		pParam->dispmode = Mode_VGA;
		pParam->pclk = 60000000;
		pParam->x = 768;
		pParam->y = 1024;
		pParam->hbp = 100;
		pParam->hfp = 20;
		pParam->hspw = 55;
		pParam->ht = 943;
		pParam->vfp = 10;
		pParam->vbp = 22;
		pParam->vspw = 4;
		pParam->vt = 1060;
		pParam->hpolarity = 1;
		pParam->vpolarity = 1;
		pParam->clk_phase = 2;
		pParam->lvds_mode = 0;
		pParam->lvds_link = 0;
		break;

	// OPRT RES H1280 720P60
	case 0x8B:
		pParam->dispmode = Mode_VGA;
		pParam->pclk=74250000;
		pParam->x=1280;
		pParam->y=720;
		pParam->hbp=220;
		pParam->hfp=110;
		pParam->hspw=40;
		pParam->ht=1650;
		pParam->vfp=5;
		pParam->vbp=20;
		pParam->vspw=5;
		pParam->vt=750;
		pParam->hpolarity = 1;
		pParam->vpolarity = 1;
		pParam->clk_phase = 2;
		pParam->lvds_mode = 0;
		pParam->lvds_link = 0;
		break;

	case 0x8C:
		pParam->dispmode = Mode_VGA;
		pParam->pclk = 66000000;
		pParam->x = 1280;
		pParam->y = 640;
		pParam->hbp = 220;
		pParam->hfp = 110;
		pParam->hspw = 40;
		pParam->ht = 1650;
		pParam->vfp = 5;
		pParam->vbp = 16;
		pParam->vspw = 5;
		pParam->vt = 666;
		pParam->hpolarity = 1;
		pParam->vpolarity = 1;
		pParam->clk_phase = 2;
		pParam->lvds_mode = 0;
		pParam->lvds_link = 0;
		break;

	case 0x8E:
		pParam->dispmode = Mode_VGA;
		pParam->pclk = 48000000;
		pParam->x = 1280;
		pParam->y = 480;
		pParam->hbp = 100;
		pParam->hfp = 60;
		pParam->hspw = 60;
		pParam->ht = 1500;
		pParam->vfp = 20;
		pParam->vbp = 30;
		pParam->vspw = 10;
		pParam->vt = 540;
		pParam->hpolarity = 1;
		pParam->vpolarity = 1;
		pParam->clk_phase = 2;
		pParam->lvds_mode = 0;
		pParam->lvds_link = 0;
		break;

	case 0x90:
		pParam->dispmode = Mode_VGA;
		pParam->pclk = 36000000;
		pParam->x = 1024;
		pParam->y = 480;
		pParam->hbp = 50;
		pParam->hfp = 30;
		pParam->hspw = 38;
		pParam->ht = 1142;
		pParam->vfp = 10;
		pParam->vbp = 25;
		pParam->vspw = 10;
		pParam->vt = 525;
		pParam->hpolarity = 1;
		pParam->vpolarity = 1;
		pParam->clk_phase = 2;
		pParam->lvds_mode = 0;
		pParam->lvds_link = 0;
		break;

	case 0x91:
		pParam->dispmode = Mode_VGA;
		pParam->pclk = 60000000;
		pParam->x = 1440;
		pParam->y = 540;
		pParam->hbp = 120;
		pParam->hfp = 60;
		pParam->hspw = 80;
		pParam->ht = 1700;
		pParam->vbp = 30;
		pParam->vfp = 10;
		pParam->vspw = 8;
		pParam->vt = 588;
		pParam->hpolarity = 1;
		pParam->vpolarity = 1;
		pParam->clk_phase = 2;
		pParam->lvds_mode = 0;
		pParam->lvds_link = 0;
		break;

	case 0x92:
		pParam->dispmode = Mode_VGA;
		pParam->pclk = 36000000;
		pParam->x = 960;
		pParam->y = 540;
		pParam->hbp = 25;
		pParam->hfp = 25;
		pParam->hspw = 15;
		pParam->ht = 1025;
		pParam->vbp = 35;
		pParam->vfp = 8;
		pParam->vspw = 2;
		pParam->vt = 585;
		pParam->hpolarity = 1;
		pParam->vpolarity = 1;
		pParam->clk_phase = 2;
		pParam->lvds_mode = 0;
		pParam->lvds_link = 0;
		break;

	case 0x96:
		pParam->dispmode = Mode_VGA;
		pParam->pclk = 79200000;
		pParam->x = 1280;
		pParam->y = 768;
		pParam->hbp = 220;
		pParam->hfp = 110;
		pParam->hspw = 40;
		pParam->ht = 1650;
		pParam->vfp = 6;
		pParam->vbp = 20;
		pParam->vspw = 6;
		pParam->vt = 800;
		pParam->hpolarity = 1;
		pParam->vpolarity = 1;
		pParam->clk_phase = 2;
		pParam->lvds_mode = 0;
		pParam->lvds_link = 0;
		break;

	case 0x98:
		pParam->dispmode = Mode_VGA;
		pParam->pclk = 75350000;
		pParam->x = 1540;
		pParam->y = 720;
		pParam->hbp = 80;
		pParam->hfp = 48;
		pParam->hspw = 32;
		pParam->ht = 1700;
		pParam->vfp = 2;
		pParam->vbp = 5;
		pParam->vspw = 2;
		pParam->vt = 729;
		pParam->hpolarity = 1;
		pParam->vpolarity = 1;
		pParam->clk_phase = 2;
		pParam->lvds_mode = 0;
		pParam->lvds_link = 0;
		break;
	// OPRT RES H1280 720p50
	case 0xA0:
		pParam->dispmode = Mode_VGA;
		pParam->pclk = 74250000;
		pParam->x = 1280;
		pParam->y = 720;
		pParam->hbp = 220;
		pParam->hfp = 440;
		pParam->hspw = 40;
		pParam->ht = 1980;
		pParam->vfp = 5;
		pParam->vbp = 20;
		pParam->vspw = 5;
		pParam->vt = 750;
		pParam->hpolarity = 1;
		pParam->vpolarity = 1;
		pParam->clk_phase = 2;
		pParam->lvds_mode = 0;
		pParam->lvds_link = 0;
		break;
	// OPRT RES H1280 720p40
	case 0xA1:
		pParam->dispmode = Mode_VGA;
		pParam->pclk = 59400000;
		pParam->x = 1280;
		pParam->y = 720;
		pParam->hbp = 220;
		pParam->hfp = 440;
		pParam->hspw = 40;
		pParam->ht = 1980;
		pParam->vfp = 5;
		pParam->vbp = 20;
		pParam->vspw = 5;
		pParam->vt = 750;
		pParam->hpolarity = 1;
		pParam->vpolarity = 1;
		pParam->clk_phase = 2;
		pParam->lvds_mode = 0;
		pParam->lvds_link = 0;
		break;

	// OPRT RES H768 1024p40
	case 0xA2:
		pParam->dispmode = Mode_VGA;
		pParam->pclk = 40000000;
		pParam->x = 768;
		pParam->y = 1024;
		pParam->hbp = 100;
		pParam->hfp = 20;
		pParam->hspw = 55;
		pParam->ht = 943;
		pParam->vfp = 10;
		pParam->vbp = 22;
		pParam->vspw = 4;
		pParam->vt = 1060;
		pParam->hpolarity = 1;
		pParam->vpolarity = 1;
		pParam->clk_phase = 2;
		pParam->lvds_mode = 0;
		pParam->lvds_link = 0;
		break;
	// OPRT RES H1920 720 BENZ 5 0
	case 0xA3:
	// OPRT RES H1920 720 BENZ 5 5
	case 0xA4:
		pParam->dispmode = Mode_VGA;
		pParam->pclk = 48000000;
		pParam->x = 1920;
		pParam->y = 720;
		pParam->hbp = 48;
		pParam->hfp = 41;
		pParam->hspw = 40;
		pParam->ht = 2049;
		pParam->vfp = 25;
		pParam->vbp = 25;
		pParam->vspw = 10;
		pParam->vt = 780;
		pParam->hpolarity = 1;
		pParam->vpolarity = 1;
		pParam->clk_phase = 2;
		pParam->lvds_mode = 0;
		pParam->lvds_link = 0;
		break;
	// OPRT RES H1920 1080p40
	case 0xA5:
		pParam->dispmode = Mode_VGA;
		pParam->pclk = 99000000;
		pParam->x = 1920;
		pParam->y = 1080;
		pParam->hbp = 148;
		pParam->hfp = 88;
		pParam->hspw = 44;
		pParam->ht = 2200;
		pParam->vfp = 4;
		pParam->vbp = 36;
		pParam->vspw = 5;
		pParam->vt = 1125;
		pParam->hpolarity = 1;
		pParam->vpolarity = 1;
		pParam->clk_phase = 2;
		pParam->lvds_mode = 0;
		pParam->lvds_link = 0;
		break;

	case 0xC0:
		pParam->dispmode = Mode_VGA;
		pParam->pclk = 140580000;
		pParam->x = 1920;
		pParam->y = 1020;
		pParam->hbp = 148;
		pParam->hfp = 88;
		pParam->hspw = 44;
		pParam->ht = 2200;
		pParam->vbp = 25;
		pParam->vfp = 12;
		pParam->vspw = 5;
		pParam->vt = 1062;
		pParam->hpolarity = 1;
		pParam->vpolarity = 1;
		pParam->clk_phase = 2;
		pParam->lvds_mode = 0;
		pParam->lvds_link = 0;
		break;

	case 0xC1:
		pParam->dispmode = Mode_VGA;
		pParam->pclk = 76325000;
		pParam->x = 1560;
		pParam->y = 700;
		pParam->hbp = 82;
		pParam->hfp = 48;
		pParam->hspw = 32;
		pParam->ht = 1722;
		pParam->vfp = 2;
		pParam->vbp = 5;
		pParam->vspw = 2;
		pParam->vt = 709;
		pParam->hpolarity = 1;
		pParam->vpolarity = 1;
		pParam->clk_phase = 2;
		pParam->lvds_mode = 0;
		pParam->lvds_link = 0;
		break;

	default:
		printf("param cmd (0x0B) index = 0x%x unsupported\n", index);
		return -1;
		break;
	}
	return 0;
}

int BC_BMW_ID8_ID8p5_ID9_ParamModify(DispParam_t* pParam)
{
	if (!pParam) {
		return -1;
	}

	if (pParam->dispmode == Mode_VGA) {
		if (pParam->x == 2880 && pParam->y == 960 && pParam->pclk == 188140000) {
			printf("%s Guohang ID8 VGA to HDMI, vh_polarity 0x%02x to 0x03\n", __FUNCTION__, (pParam->vpolarity << 1) | (pParam->hpolarity));
			pParam->dispmode = Mode_HDMI;
			pParam->hpolarity = 1;
			pParam->vpolarity = 1;
			return 0;
		}
		return -1;
	}

	if (pParam->x == 1920 && pParam->y == 960) {
		printf("%s ID9 vh_polarity 0x%02x to 0x00\n", __FUNCTION__, (pParam->vpolarity << 1) | (pParam->hpolarity));
		pParam->hpolarity = 0;
		pParam->vpolarity = 0;
		return 0;
	}

	if (pParam->x == 2880 && pParam->y == 960) {
		printf("pParam->pclk = %d\n", pParam->pclk);
		if (pParam->pclk == 177282000 && pParam->ht == 2940) {
			printf("%s ID8\n", __FUNCTION__);
			pParam->pclk = 180000000;
			pParam->hpolarity = 0;
			pParam->vpolarity = 0;
			pParam->ht = 2986;
			return 0;
		}
		else if (pParam->pclk == 188140000)
		{
			printf("%s ID8p5 vh_polarity 0x%02x to 0x00\n", __FUNCTION__, (pParam->vpolarity << 1) | (pParam->hpolarity));
			pParam->hpolarity = 0;
			pParam->vpolarity = 0;
			return 0;
		}
	}
	return -1;
}

int32_t ComRecvCmd_XCP(Uart& ExtCom, uint8_t* pucCmd, uint8_t* pucData, uint8_t* pucDatalen, int32_t iTimeout, bool bDebug, uint8_t* pBufRaw) {
	uint8_t ucBuf[256] = {0};
	int32_t iLen = 0;
	static float wRate = 4095.0f / GCSystemConf.GetDispWidth();
	static float hRate = 4095.0f / GCSystemConf.GetDispHeight();

	iLen = ComProtocolRecvXCP(ExtCom, pBufRaw? pBufRaw: ucBuf, 128, iTimeout);
	if (iLen > 4) {
		XCPFrame_t* pXCPFrame = pBufRaw? (XCPFrame_t*)pBufRaw : (XCPFrame_t*)ucBuf;
		// if (bDebug)
		// 	GPortConf.PushData(pXCPFrame->cmd, pBufRaw ? pBufRaw : ucBuf, iLen);
		//printf("pXCPFrame->cmd=0x%02x, pXCPFrame->len=%d\n", pXCPFrame->cmd, pXCPFrame->len);
		switch (pXCPFrame->cmd) {
			case 0x10:
				//printf("pXCPFrame->len=%d\n", pXCPFrame->len);
				if (pXCPFrame->buf[0] == 0xA5) {
					// 实测最多有5个点的触摸，本软件暂且支持一个点触摸
					int count = pXCPFrame->buf[1];
					if (pXCPFrame->buf[1] > 0) {
						//printf("%02x %02x %02x %02x %02x\n", pXCPFrame->buf[1], pXCPFrame->buf[2], pXCPFrame->buf[3], pXCPFrame->buf[4], pXCPFrame->buf[5]);
						pucData[0] = 1;
						unsigned short x = (pXCPFrame->buf[3] << 8) + pXCPFrame->buf[2];
						unsigned short y = (pXCPFrame->buf[5] << 8) + pXCPFrame->buf[4];
						x = x * wRate;
						y = y * hRate;
						pucData[1] = (x >> 8) & 0xFF;
						pucData[2] = x & 0xFF;
						pucData[3] = (y >> 8) & 0xFF;
						pucData[4] = y & 0xFF;
					}
					else {
						memset(&pucData[0], 0, 5);
					}
					*pucCmd = CP_CMD_TOUCHCOOR;
					*pucDatalen = 5;
				}
				else {
					return -1;
				}
			break;
		}
		return 0;
	}

	return -1;

}

// 通过UART通信接收GP2协议的命令，并根据接收到的数据类型进行处理，返回命令标识和数据。
int32_t ComRecvCmd_GP2(Uart& ExtCom, uint8_t* pucCmd, uint8_t* pucData, uint8_t* pucDatalen, int32_t iTimeout, bool bDebug, uint8_t* pBufRaw) {
	uint8_t ucBuf[256] = {0};
	int32_t iLen = 0;
	CarInfo_t* pCarInfo = (CarInfo_t*)pucData;
	Radar6644_t* pRadar6644 = (Radar6644_t*)pucData;
	DispParam_t* pParam = NULL;
	DispParam_t dispParam;
	unsigned char* pBuf = NULL;
	static int isYueJiaBoard = 0;
	static DWORD voiceTick = GetTickCount();
	static uint8_t voiceCmd = 0x00;
	// static uint8_t lastCmd = 0x00;
	DWORD tick, tickDelta;
	iLen = ComProtocolRecv_GP2(ExtCom, pBufRaw? pBufRaw: ucBuf, 128, iTimeout); // 调用ComProtocolRecv_GP2函数接收数据，数据存储在pBufRaw(pBufRaw不为空)或ucBuf中，最大长度为128，超时时间为iTimeout
	/*
	iLen = 11
	0x2e 0x03 0x08 0x13 0x00 0x00 0x9e 0xaa 0xee 0x00 0x00 
	0x2e 0x03 0x08 0x13 0x00 0x00 0x80 0x55 0x55 0x00 0x00 
	0x2e 0x03 0x08 0x13 0x00 0x00 0xa1 0xff 0x33 0x00 0x00 */
	if (iLen > 3) {
	// 	printf("[%s - %d], iLen = %d\r\n",__FUNCTION__,__LINE__, iLen);
	// 	for (size_t i = 0; i < iLen; i++) {
	// 		printf("0x%02x ", ucBuf[i]);
	// 	}
	// 	printf("\n");
		
		GP2Frame_t* pGP2Frame = pBufRaw? (GP2Frame_t*)pBufRaw : (GP2Frame_t*)ucBuf;
		// if (bDebug)
		// 	GPortConf.PushData(pGP2Frame->cmd, pBufRaw ? pBufRaw : ucBuf, iLen);
		// if (lastCmd != pGP2Frame->cmd) {
		// 	printf("pGP2Frame->cmd=0x%02x\n", pGP2Frame->cmd);
		// 	lastCmd = pGP2Frame->cmd;
		// }
		switch (pGP2Frame->cmd) {
		case 0x03:                          // car infomation
		{
			if ((pGP2Frame->len != 0x08) && (pGP2Frame->len != 0x10)) {
				return -1;
			}
			bool carDataExt = (pGP2Frame->len == 0x10);
			uint8_t uctmp;
			uint32_t corner;
			memset(pCarInfo, 0, sizeof(CarInfo_t));
			pCarInfo->accValid = 1;
			if ((pGP2Frame->buf[0] & 0x03) >= 0x02) {
				pCarInfo->acc = 1;
			}
			else {
				pCarInfo->acc = 0;
			}
			pCarInfo->gear = (pGP2Frame->buf[0] >> 4) & 0x0f;	// 取出buf[0] 这一字节的高 4 位 右移到低 4 位
			//                if (pCarInfo->gear != 0x01)
			//                    pCarInfo->gear = 0x03;                  // except R gear, set D gear.

			pCarInfo->lightstatus = pGP2Frame->buf[1] & 0x07;
			if (carDataExt) {
				pCarInfo->lightstatus = (pCarInfo->lightstatus & (~0x08)) | ((pGP2Frame->buf[1] >> 1) & 0x08);  // 示宽灯
			}
			pCarInfo->lightstatus = (pCarInfo->lightstatus & (~0x40)) | (pGP2Frame->buf[6] & 0x40);     // P键
			if ((pGP2Frame->buf[6] & 0x80)) {         // Auto Park
				pCarInfo->autopark = 1;
			}
			else {
				pCarInfo->autopark = 0;
			}
			pCarInfo->speed = pGP2Frame->buf[2];
			//if (pCarInfo->speed > 1 && pCarInfo->speed < 20) {
			//    if (CarInfoValidFlagsGet(ComSpeedFlag) == 0) {
			//        CarInfoValidFlagsSet(ComSpeedFlag);
			//    }
			//}

			//if (CarInfoValidFlagsGet(ComConnerFlag) == 0 && preConner != ucBuf[3]) {
			//    if ((preConner != 0x80) && (preConner != 0x00)) {
			//        CarInfoValidFlagsSet(ComConnerFlag);
			//    }
			//    preConner = ucBuf[3];
			//}
			corner = CalCorner(pGP2Frame->buf[3]);
			pCarInfo->corner = ((corner) << 24) | ((corner & 0x0000ff00) << 8) | ((corner & 0x00ff0000) >> 8) | ((corner) >> 24);
			pCarInfo->doorstatus = pGP2Frame->buf[6] & 0x1f;

			if (carDataExt == false) {
				pCarInfo->Radar.RadarMode = 0xB2;
				uctmp = pGP2Frame->buf[4] & 0x03;
				if (uctmp == 0 || uctmp == 2) {
					pCarInfo->Radar.RadarFront_1 = uctmp;
				}
				else {
					pCarInfo->Radar.RadarFront_1 = uctmp ^ 0x02;
				}
				uctmp = (pGP2Frame->buf[4] >> 2) & 0x03;
				if (uctmp == 0 || uctmp == 2) {
					pCarInfo->Radar.RadarFront_2 = uctmp;
				}
				else {
					pCarInfo->Radar.RadarFront_2 = uctmp ^ 0x02;
				}
				uctmp = (pGP2Frame->buf[4] >> 4) & 0x03;
				if (uctmp == 0 || uctmp == 2) {
					pCarInfo->Radar.RadarFront_4 = uctmp;
				}
				else {
					pCarInfo->Radar.RadarFront_4 = uctmp ^ 0x02;
				}
				uctmp = (pGP2Frame->buf[4] >> 6) & 0x03;
				if (uctmp == 0 || uctmp == 2) {
					pCarInfo->Radar.RadarFront_3 = uctmp;
				}
				else {
					pCarInfo->Radar.RadarFront_3 = uctmp ^ 0x02;
				}
				uctmp = pGP2Frame->buf[5] & 0x03;
				if (uctmp == 0 || uctmp == 2) {
					pCarInfo->Radar.RadarRear_1 = uctmp;
				}
				else {
					pCarInfo->Radar.RadarRear_1 = uctmp ^ 0x02;
				}
				uctmp = (pGP2Frame->buf[5] >> 2) & 0x03;
				if (uctmp == 0 || uctmp == 2) {
					pCarInfo->Radar.RadarRear_2 = uctmp;
				}
				else {
					pCarInfo->Radar.RadarRear_2 = uctmp ^ 0x02;
				}
				uctmp = (pGP2Frame->buf[5] >> 4) & 0x03;
				if (uctmp == 0 || uctmp == 2) {
					pCarInfo->Radar.RadarRear_4 = uctmp;
				}
				else {
					pCarInfo->Radar.RadarRear_4 = uctmp ^ 0x02;
				}
				uctmp = (pGP2Frame->buf[5] >> 6) & 0x03;
				if (uctmp == 0 || uctmp == 2) {
					pCarInfo->Radar.RadarRear_3 = uctmp;
				}
				else {
					pCarInfo->Radar.RadarRear_3 = uctmp ^ 0x02;
				}
			}
			else {
				pCarInfo->Radar.RadarMode = pGP2Frame->buf[7];  // 0xB7
				memcpy((uint8_t*)&pCarInfo->Radar + sizeof(pCarInfo->Radar.RadarMode), &pGP2Frame->buf[8], sizeof(pCarInfo->Radar) - sizeof(pCarInfo->Radar.RadarMode));
			}

			*pucCmd = CP_CMD_CARDATA;
			if (carDataExt) {
				*pucDatalen = sizeof(CarInfo_t);
			}
			else {
				*pucDatalen = 13;
			}
		}
		break;
		case 0x02:                          // key  
			if (pGP2Frame->len != 2) {
				return -1;
			}
			if (pGP2Frame->buf[0] == 0xF0 && pGP2Frame->buf[1] == 0x00) {
				pucData[0] = 0x32;
				pucData[1] = 0x01;
				*pucDatalen = 2;
			}
			else if ((pGP2Frame->buf[0] == 0xF1 || pGP2Frame->buf[0] == 0xF2) && pGP2Frame->buf[1] == 0x00) {
				pucData[0] = 0x32;
				pucData[1] = 0x00;
				*pucDatalen = 2;
			}
			else if (pGP2Frame->buf[0] == 0x10 && pGP2Frame->buf[1] == 0x01) {
				pucData[0] = BV_KEY_ENTER2;
				*pucDatalen = 1;
			}
			else if (((pGP2Frame->buf[0] == 0x19) && pGP2Frame->buf[1] == 0x00) ||
					((pGP2Frame->buf[0] == 0x0E) && pGP2Frame->buf[1] == 0x01)){		// 巴谷PREV
				pucData[0] = 0x30;
				pucData[1] = 0x01;
				pucData[2] = 0x01;
				*pucDatalen = 3;
			}
			else if (((pGP2Frame->buf[0] == 0x1a) && pGP2Frame->buf[1] == 0x00) ||
					((pGP2Frame->buf[0] == 0x0F) && pGP2Frame->buf[1] == 0x01)){		// 巴谷NEXT
				pucData[0] = 0x30;
				pucData[1] = 0x00;
				pucData[2] = 0x01;
				*pucDatalen = 3;
			}
			else if (pGP2Frame->buf[0] == 0x11 && pGP2Frame->buf[1] == 0x01) {
				pucData[0] = 0x05;
				*pucDatalen = 1;
			}
			else if (pGP2Frame->buf[0] == 0x12 && pGP2Frame->buf[1] == 0x01) {
				pucData[0] = 0x06;
				*pucDatalen = 1;
			}
			else if (pGP2Frame->buf[0] == 0x13 && pGP2Frame->buf[1] == 0x01) {
				pucData[0] = 0x07;
				*pucDatalen = 1;
			}
			else if (pGP2Frame->buf[0] == 0x14 && pGP2Frame->buf[1] == 0x01) {
				pucData[0] = 0x08;
				*pucDatalen = 1;
			}
			else if (pGP2Frame->buf[0] == 0x04 && pGP2Frame->buf[1] == 0x01) {
				pucData[0] = 0x02;
				*pucDatalen = 1;
			}
			else if (pGP2Frame->buf[0] == 0x05 && pGP2Frame->buf[1] == 0x01) {
				pucData[0] = 0x04;
				*pucDatalen = 1;
			}
			else if (pGP2Frame->buf[0] == 0x40 && pGP2Frame->buf[1] == 0x01) {
				pucData[0] = 0x32;
				pucData[1] = 0x20;
				*pucDatalen = 2;
			}
			else if (pGP2Frame->buf[0] == 0x26 && pGP2Frame->buf[1] == 0x01) {	// 巴谷VOLOV用于切换显示
				pucData[0] = 0x32;
				pucData[1] = 0xCA;
				*pucDatalen = 2;
			}
			else {
				return -1;
			}
			*pucCmd = CP_CMD_KEY;
			break;
		case 0x04:                       // time
			if (pGP2Frame->len != 0x07) {
				return -1;
			}
			memcpy(pucData, &pGP2Frame->buf[1], 6);
			*pucCmd = CP_CMD_SYNCTIME;
			*pucDatalen = 6;
			break;
		case 0x05:                          // touchscreen
			if (pGP2Frame->len != 0x05) {
				return -1;
			}
			/*            if (pGP2Frame->buf[0] == 0x00) {
							pucData[0] = 0x11;
						} else if (pGP2Frame->buf[0] == 0x01 || pGP2Frame->buf[0] == 0x10) {
							pucData[0] = 0x10;
						} */
			if (pGP2Frame->buf[0] == 0x02 || pGP2Frame->buf[0] == 0x04) {
				*pucCmd = CP_CMD_KEY;
				*pucDatalen = 3;
				pucData[0] = 0x30;                     // clockwise
				pucData[1] = 0x0;
				pucData[2] = 2;
				break;
			}
			else if (pGP2Frame->buf[0] == 0x03 || pGP2Frame->buf[0] == 0x05) {
				*pucCmd = CP_CMD_KEY;
				*pucDatalen = 3;
				pucData[0] = 0x30;                     // clockwise
				pucData[1] = 0x1;
				pucData[2] = 2;
				break;
			}
			memcpy(&pucData[0], pGP2Frame->buf, 5);
			*pucCmd = CP_CMD_TOUCHCOOR;
			*pucDatalen = 5;
			break;

		case 0x07:
			*pucCmd = CP_CMD_UPDATA_PROGRAM;
			memcpy(&pucData[0], pGP2Frame->buf, pGP2Frame->len);
			*pucDatalen = pGP2Frame->len;
			break;

		case 0x18:
			memcpy(&pucData[0], pGP2Frame->buf, 5);
			if (pucData[0] != 0x20 && pucData[0] != 0x21) {
				pucData[0] = pucData[0] ? 0x20 : 0x21;
			}
			*pucCmd = CP_CMD_TOUCHCOOR;
			*pucDatalen = 5;
			break;

		case 0x37:                          // touchscreen
			uint32_t X, Y;
			if (pGP2Frame->len != 0x05) {
				return -1;
			}
			pucData[0] = pGP2Frame->buf[4];
			X = pGP2Frame->buf[0] * 256 + pGP2Frame->buf[1];
			Y = pGP2Frame->buf[2] * 256 + pGP2Frame->buf[3];
			X = (X * 4095) / 1280;
			Y = (Y * 4095) / 720;
			pucData[1] = (uint8_t)((X & 0xFFFF) >> 8);
			pucData[2] = (uint8_t)((X & 0xFFFF) >> 0);
			pucData[3] = (uint8_t)((Y & 0xFFFF) >> 8);
			pucData[4] = (uint8_t)((Y & 0xFFFF) >> 0);
			*pucCmd = CP_CMD_TOUCHCOOR;
			*pucDatalen = 5;
			break;
		case 0x09:
			if (pGP2Frame->len != 1) {
				return -1;
			}
			if (pGP2Frame->buf[0] < 5) {
				pucData[1] = pGP2Frame->buf[0] + 0x1;
			}
			else if (pGP2Frame->buf[0] == 0x16 || pGP2Frame->buf[0] == 0x05) {
				pucData[1] = 0x06;              // 打开窄道模式
			}
			else if (pGP2Frame->buf[0] == 0x18 || pGP2Frame->buf[0] == 0x06) {
				pucData[1] = 0x07;              // 打开路崖模式
			}
			else if (pGP2Frame->buf[0] == 0x25 || pGP2Frame->buf[0] == 0x34) {
				pucData[1] = 0x08;              // 打开前流媒体
			}
			else if (pGP2Frame->buf[0] == 0x26 || pGP2Frame->buf[0] == 0x35) {
				pucData[1] = 0x09;              // 打开后流媒体
			}
			else if (pGP2Frame->buf[0] == 0x40 || pGP2Frame->buf[0] == 0x33) {
				pucData[1] = 0x0A;              // 打开2D
			}
			else if (pGP2Frame->buf[0] == 0x41 || pGP2Frame->buf[0] == 0x31) {
				pucData[1] = 0x0B;              // 打开3D
			}
			else if (pGP2Frame->buf[0] == 0x30 || pGP2Frame->buf[0] == 0x42) {
				pucData[1] = 0x0C;              // 打开左3D
			}
			else if (pGP2Frame->buf[0] == 0x32 || pGP2Frame->buf[0] == 0x43) {
				pucData[1] = 0x0D;              // 打开右3D
			}
			else if (pGP2Frame->buf[0] == 0x50) {
				pucData[1] = 0xA1;              // 关闭540（车底透视关闭）
			}
			else if (pGP2Frame->buf[0] == 0x51) {
				pucData[1] = 0xA2;              // 打开540（车底透视打开）
			}
			else if (pGP2Frame->buf[0] == 0x52) {
				pucData[1] = 0xA3;              // 环视一周（满屏车模半透明）
			}
			else if (pGP2Frame->buf[0] == 0x80) {
				pucData[1] = 0xA4;              // 关闭氛围灯UI
			}
			else if (pGP2Frame->buf[0] == 0x81) {
				pucData[1] = 0xA5;              // 打开氛围灯UI
			}
			else if (pGP2Frame->buf[0] == 0x60) {
				pucData[1] = 0xA6;              // 关闭香氛UI
			}
			else if (pGP2Frame->buf[0] == 0x61) {
				pucData[1] = 0xA7;              // 打开香氛UI
			}
			else if (pGP2Frame->buf[0] == 0x71) {
				pucData[1] = 0xA8;              // 打开互联
			}
			else if (pGP2Frame->buf[0] == 0x70) {
				pucData[1] = 0xA9;              // 关闭互联
			}
			else {
				return -1;
			}

			// 过滤掉在在一定时间内重发的声控(如翼畅解码板)，避免在重发期间内反复重新切换视图
			tick = GetTickCount();
			tickDelta = tick - voiceTick;
			voiceTick = tick;
			if (pucData[1] == voiceCmd && tickDelta > 0 && tickDelta < 2000) {
				return -1;
			}
			voiceCmd = pucData[1];

			pucData[0] = 0x32;
			*pucCmd = CP_CMD_KEY;
			*pucDatalen = 2;
			break;
			//        case 0x71:
			//            if (iLen == 9) {
			//            } else
			//                return -1;
			//            break;
		case 0x0B:
			if (pGP2Frame->len != 0x20 && pGP2Frame->len != 0x21 && pGP2Frame->len != 0x1) {
				printf("cmd=0x%x, pGP2Frame->len(0x%x) != 0x20 or 0x21 or 0x01!\n", pGP2Frame->cmd, pGP2Frame->len);
				return -1;
			}
			*pucCmd = CP_CMD_GP_SET2;
			pucData[0] = 0x03;              // 设置屏参
			pParam = (DispParam_t*)&pucData[1];
			pBuf = pGP2Frame->buf;
			if (pGP2Frame->len == 0x1)
			{
				if (0 != SetParamByIndex(pBuf[0], pParam)) {
					return -1;
				}
			} else {
				pParam->pclk = (pBuf[1] << 24) | (pBuf[2] << 16) | (pBuf[3] << 8) | pBuf[4];
				pParam->x = (pBuf[5] << 8) + pBuf[6];
				pParam->y = (pBuf[7] << 8) + pBuf[8];
				pParam->hbp = (pBuf[13] << 8) + pBuf[14];
				pParam->hfp = (pBuf[9] << 8) + pBuf[10];
				pParam->hspw = (pBuf[11] << 8) + pBuf[12];
				pParam->ht = pParam->x + pParam->hbp + pParam->hfp + pParam->hspw;
				pParam->vbp = (pBuf[20] << 8) + pBuf[21];
				pParam->vfp = (pBuf[16] << 8) + pBuf[17];
				pParam->vspw = (pBuf[18] << 8) + pBuf[19];
				pParam->vt = pParam->y + pParam->vbp + pParam->vfp + pParam->vspw;
				pParam->hpolarity = !pBuf[15];
				pParam->vpolarity = !pBuf[22];
				pParam->clk_phase = pBuf[26];
				pParam->rotate = (pBuf[23] << 8) + pBuf[24];
				switch (pBuf[25])
				{
					case Mode_BAGOO_HDMI:
						pParam->dispmode = Mode_HDMI;
						BC_BMW_ID8_ID8p5_ID9_ParamModify(pParam);
						break;
					case Mode_BAGOO_VGA:
					case Mode_BAGOO_RGB888:
						pParam->dispmode = Mode_VGA;
						BC_BMW_ID8_ID8p5_ID9_ParamModify(pParam);
						break;
					case Mode_BAGOO_AHD:
						pParam->dispmode = Mode_AHD;
						break;
					case Mode_BAGOO_LVDS:
						pParam->dispmode = Mode_LVDS;
						pParam->lvds_mode = pGP2Frame->len == 0x21 ? pBuf[31] : 0;
						pParam->lvds_mode = !!pParam->lvds_mode;
						pParam->lvds_link = 0;
						break;

					case Mode_Preset_CVBS_NTSC:
					case Mode_Preset_CVBS_PAL:
					case Mode_Preset_AHD720P25Hz:
					case Mode_Preset_AHD720P30Hz:
					case Mode_Preset_AHD1080P25Hz:
					case Mode_Preset_AHD1080P30Hz:
					case Mode_Preset_TVI720P25Hz:
					case Mode_Preset_TVI720P30Hz:
					case Mode_Preset_TVI1080P25Hz:
					case Mode_Preset_TVI1080P30Hz:
						pParam->dispmode = pBuf[25];
						break;

					default:
						pParam->dispmode = Mode_AV;
						break;
				}
			}
			*pucDatalen = sizeof(DispParam_t) + 1;;
			break;
		case 0xFD:                          // disp param
			if ((pGP2Frame->len < 0x1A) || (pGP2Frame->len > 0x1F)) {
				printf("cmd=0x%x, pGP2Frame->len(0x%x) is invalid, not in [0x1A,0x1F]\n", pGP2Frame->cmd, pGP2Frame->len);
				return -1;
			}
			*pucCmd = CP_CMD_GP_SET2;
			pucData[0] = 0x03;              // 设置屏参
			pParam = (DispParam_t*)&pucData[1];
			GCSystemConf.GetDisplayinfo(pParam);
			pBuf = pGP2Frame->buf;

			pParam->pclk = (pBuf[0] << 24) | (pBuf[1] << 16) | (pBuf[2] << 8) | pBuf[3];
			pParam->x = (pBuf[5] << 8) + pBuf[6];
			pParam->y = (pBuf[7] << 8) + pBuf[8];
			pParam->hbp = (pBuf[11] << 8) + pBuf[12];
			pParam->hfp = (pBuf[13] << 8) + pBuf[14];
			pParam->hspw = (pBuf[15] << 8) + pBuf[16];
			pParam->ht = pParam->x + pParam->hbp + pParam->hfp + pParam->hspw;
			pParam->vbp = (pBuf[19] << 8) + pBuf[20];
			pParam->vfp = (pBuf[21] << 8) + pBuf[22];
			pParam->vspw = pBuf[23];
			pParam->vt = pParam->y + pParam->vbp + pParam->vfp + pParam->vspw;
			pParam->hpolarity = !pBuf[24];
			pParam->vpolarity = !pBuf[25];

			if (pGP2Frame->len >= 0x1B) {
				if (pBuf[26] == 0) {
					pParam->dispmode = Mode_VGA;
				}
				else if (pBuf[26] == 1) {
					pParam->dispmode = Mode_HDMI;
				}
				else if (pBuf[26] == 2) {
					pParam->dispmode = Mode_AHD;
				}
				else if (pBuf[26] == 4) {
					pParam->dispmode = Mode_LVDS;
					pParam->lvds_mode = 0;  // 享车派所配解码器的LVDS模式信息无效，固定为4，故此处取相反值为0，注意：仅限于此客户，其它客户按协议此处为1
					pParam->lvds_link = 0;
				}
				else if (pBuf[26] == 5) {
					pParam->dispmode = Mode_LVDS;
					pParam->lvds_mode = 1;  // 享车派所配解码器的LVDS模式信息无效，固定为4，故此处取相反值为1，注意：仅限于此客户，其它客户按协议此处为0
					pParam->lvds_link = 0;
				}
				else if (pBuf[26] == 0x11) {
					pParam->dispmode = Mode_Preset_HDMI_CADILLAC;
				}
				else if (pBuf[26] == 0x20) {
					pParam->dispmode = Mode_Preset_BT656_CADILLAC;
				}
				else if (pBuf[26] == 0xF4) {
					pParam->dispmode = Mode_LVDS;
					pParam->lvds_mode = 0;
					pParam->lvds_link = 2;	// 双通道
				}
				else if (pBuf[26] == 0xF5) {
					pParam->dispmode = Mode_LVDS;
					pParam->lvds_mode = 1;
					pParam->lvds_link = 2;	// 双通道
				}
				else {
					pParam->dispmode = Mode_AV;
				}
				pParam->clk_phase = (pGP2Frame->len == 0x1C) ? pBuf[27] : 0;
			}
			pParam->rotate = 0;
			*pucDatalen = sizeof(DispParam_t) + 1;
			break;

		case 0x71:                          // Decorder version
			if ((pGP2Frame->len == 0) || (pGP2Frame->len % 0x09)) {
				return -1;
			}
			*pucCmd = CP_CMD_MB_VERSION;
			pucData[0] = 0x07;              // 解码器版本
			memcpy(&pucData[1], pGP2Frame->buf, pGP2Frame->len);
			*pucDatalen = pGP2Frame->len + 1;
			break;

		case 0x30:                          // disp param
			//printf("=========>request disp param,len=%d.\r\n", pGP2Frame->len);
			// len == 0x26和0x27: 悦驾协议
			if (pGP2Frame->len != 0x2b && pGP2Frame->len != 0x26 && pGP2Frame->len != 0x27) {
				printf("cmd=0x%x, pGP2Frame->len(0x%x) != 0x2b or 0x26 or 0x27!\n", pGP2Frame->cmd, pGP2Frame->len);
				return -1;
			}
			*pucCmd = CP_CMD_GP_SET2;
			pucData[0] = 0x03;              // 设置屏参
			pParam = (DispParam_t*)&pucData[1];
			pBuf = pGP2Frame->buf;

			pParam->pclk = (pBuf[3] << 24) | (pBuf[2] << 16) | (pBuf[1] << 8) | pBuf[0];
			pParam->x = (pBuf[7] << 24) | (pBuf[6] << 16) | (pBuf[5] << 8) | pBuf[4];
			pParam->y = (pBuf[11] << 24) | (pBuf[10] << 16) | (pBuf[9] << 8) | pBuf[8];
			//pParam->ht = (pBuf[3] << 24) | (pBuf[2] << 16) | (pBuf[1] << 8) | pBuf[0];
			pParam->hfp = (pBuf[15] << 24) | (pBuf[14] << 16) | (pBuf[13] << 8) | pBuf[12];
			pParam->hbp = (pBuf[19] << 24) | (pBuf[18] << 16) | (pBuf[17] << 8) | pBuf[16];
			pParam->vfp = (pBuf[23] << 24) | (pBuf[22] << 16) | (pBuf[21] << 8) | pBuf[20];
			pParam->vbp = (pBuf[27] << 24) | (pBuf[26] << 16) | (pBuf[25] << 8) | pBuf[24];
			pParam->hspw = (pBuf[31] << 24) | (pBuf[30] << 16) | (pBuf[29] << 8) | pBuf[28];
			pParam->vspw = (pBuf[35] << 24) | (pBuf[34] << 16) | (pBuf[33] << 8) | pBuf[32];
			if (isYueJiaBoard) {	// 悦驾的协议中，同步脉宽已经加入后肩，所以此处减去同步脉宽
				pParam->hbp = pParam->hbp - pParam->hspw;
				pParam->vbp = pParam->vbp - pParam->vspw;
			}
			pParam->ht = pParam->x + pParam->hfp + pParam->hbp + pParam->hspw;
			pParam->vt = pParam->y + pParam->vfp + pParam->vbp + pParam->vspw;
			pParam->hpolarity = pBuf[36];
			pParam->vpolarity = pBuf[37];
			if (isYueJiaBoard) {
				pParam->hpolarity = !pParam->hpolarity;
				pParam->vpolarity = !pParam->vpolarity;
			}

			if (pGP2Frame->len == 0x2b) {
				pParam->clk_phase = pBuf[38];
				switch (pBuf[39])
				{
				case Mode_OTHER_HDMI:
					pParam->dispmode = Mode_HDMI;
					pParam->lvds_mode = !!pBuf[42];		// 享车派
					pParam->lvds_link = (pBuf[39] == Mode_OTHER_LVDS_SPLIT) ? 2 : 0;
					break;
				case Mode_OTHER_VGA:
				case Mode_OTHER_RGB:
					pParam->dispmode = Mode_VGA;
					break;
				case Mode_OTHER_AHD:
					pParam->dispmode = Mode_AHD;
					break;
				case Mode_OTHER_LVDS:
				case Mode_OTHER_LVDS_SPLIT:
					pParam->dispmode = Mode_LVDS;
					pParam->lvds_mode = !!pBuf[42];		// 享车派
					pParam->lvds_link = (pBuf[39] == Mode_OTHER_LVDS_SPLIT) ? 2 : 0;
					break;

				case Mode_Preset_CVBS_NTSC:
				case Mode_Preset_CVBS_PAL:
				case Mode_Preset_AHD720P25Hz:
				case Mode_Preset_AHD720P30Hz:
				case Mode_Preset_AHD1080P25Hz:
				case Mode_Preset_AHD1080P30Hz:
				case Mode_Preset_TVI720P25Hz:
				case Mode_Preset_TVI720P30Hz:
				case Mode_Preset_TVI1080P25Hz:
				case Mode_Preset_TVI1080P30Hz:
					pParam->dispmode = pBuf[39];
					break;

				default:
					pParam->dispmode = Mode_AV;
					break;
				}
				pParam->rotate = (pBuf[41] << 8) | pBuf[40];
			}
			else if (pGP2Frame->len == 0x26 || pGP2Frame->len == 0x27) {
				pParam->clk_phase = (pGP2Frame->len == 0x27) ? pBuf[38] : 2;		// default 2
				pParam->dispmode = Mode_VGA;
				pParam->lvds_mode = 0;
				pParam->lvds_link = 0;
				pParam->rotate = 0;
			}
			if (pParam->clk_phase > 3) {
				printf("pParam->clk_phase = %d cast to 0.\n", pParam->clk_phase);
				pParam->clk_phase = 0;
			}
			*pucDatalen = sizeof(DispParam_t) + 1;
			break;

		case 0x68:
			if (pGP2Frame->len != 21) {
				return -1;
			}
			pRadar6644->RadarFront[0] = pGP2Frame->buf[9];
			memcpy(&pRadar6644->RadarFront[1], &pGP2Frame->buf[1], 4);
			pRadar6644->RadarFront[5] = pGP2Frame->buf[15];

			pRadar6644->RadarRear[0] = pGP2Frame->buf[14];
			memcpy(&pRadar6644->RadarRear[1], &pGP2Frame->buf[5], 4);
			pRadar6644->RadarRear[5] = pGP2Frame->buf[20];

			memcpy(&pRadar6644->RadarLeft, &pGP2Frame->buf[10], 4);
			memcpy(&pRadar6644->RadarRight, &pGP2Frame->buf[16], 4);

			{
				// 将[0x00, 0xFD）距离映射到红3黄3绿4共10段
				unsigned char mapTabs[] = { 15, 30, 45, 75, 105, 135, 165, 195, 225, 252 };
				unsigned char* pRadarVal = (unsigned char*)pRadar6644;
				for (int i = 0; i < sizeof(Radar6644_t); i++) {
					unsigned char val = 0;
					for (int j = 0; j < sizeof(mapTabs) / sizeof(mapTabs[0]); j++) {
						if (pRadarVal[i] < mapTabs[j]) {
							val = j + 1;
							break;
						}
					}
					pRadarVal[i] = val;
				}
			}

			*pucDatalen = sizeof(Radar6644_t);
			*pucCmd = CP_CMD_RADAR_6644;
			break;

		case 0xFA:
			if (pGP2Frame->len != 3) {
				return -1;
			}
			if (pGP2Frame->buf[0] == 0xAA &&
				pGP2Frame->buf[1] == 0x55 &&
				pGP2Frame->buf[2] == 0x01 ) {
				isYueJiaBoard = 1;
				printf("It's YUEJIA decoder.\n");
			}
			else {
				isYueJiaBoard = 0;
				printf("It isn't YUEJIA decoder.\n");
			}
			return -1;
			break;

		case 0xFB:
			if (pGP2Frame->len != 3) {
				return -1;
			}
			if (pGP2Frame->buf[0] == 0xAA &&
				pGP2Frame->buf[1] == 0x55) {
				printf("%s screen\n", (pGP2Frame->buf[2] ? "vertical":"wide"));
			}
			else {
				printf("cmd=0x%x, pGP2Frame->buf unsupported !\n", pGP2Frame->cmd);
				return -1;
			}
			*pucCmd = CP_CMD_DISP_ROTATE;
			pucData[0] = pGP2Frame->buf[2];
			*pucDatalen = pGP2Frame->len;
			break;

		case 0xEF:                          // 生产测试命令
			if (pGP2Frame->len == 12) {
				*pucCmd = CP_CMD_IN_TEST_MODE;
				memcpy(&pucData[0], pGP2Frame->buf, pGP2Frame->len);
				*pucDatalen = pGP2Frame->len;
			}
			else if (pGP2Frame->len == 1) {
				*pucCmd = CP_CMD_YUANCHANG_DEVICE_CHECK;
				memcpy(&pucData[0], pGP2Frame->buf, pGP2Frame->len);
				*pucDatalen = pGP2Frame->len;
			}

			break;
		default:
			return -1;
		}
		return 0;
	}

	return -1;
}

int32_t ComRecvCmd_GP1(Uart& ExtCom, uint8_t* pCmd, uint8_t* pData, uint8_t* pLen, int32_t iTimeout) {
	uint8_t ucBuf[256];
	int32_t iLen = 0;

	iLen = ComProtocolRecv_GP1(ExtCom, ucBuf, 128, iTimeout);
	if (iLen > 6) {
		GP1Frame_t* pGP1Frame = (GP1Frame_t*)ucBuf;

		*pCmd = pGP1Frame->cmd;
		*pLen = pGP1Frame->len - 1;
		memcpy(pData, pGP1Frame->buf, *pLen);

		return 0;
	}

	return -1;
}

int32_t ComRecvCmd_KoreaTouch(Uart& ExtCom, uint8_t* pucCmd, uint8_t* pucData, uint8_t* pucDatalen, int32_t iTimeout) {
	int32_t iLen;
	uint8_t ucBuf[256];
	static uint32_t X = 0;
	static uint32_t Y = 0;

	iLen = ExtCom.Read(ucBuf, 4, iTimeout);
	if (iLen < 4) {
		return -1;
	}

	if ((ucBuf[0] == 0x55 && ucBuf[1] == 0xff) || (ucBuf[0] == 0xff && ucBuf[1] == 0x55 && ucBuf[2] == 0x00 && ucBuf[3] == 0x00)) {
		uint32_t x, y;

		if ((ucBuf[0] == 0x55 && ucBuf[1] == 0xff)) {
			pucData[0] = 0x10;
			x = ucBuf[2];
			y = ucBuf[3];
			x = (x * 4095) / 0xff;
			y = (y * 4095) / 0xff;
			X = x;
			Y = y;
		}
		else {
			pucData[0] = 0x11;
		}
		pucData[1] = (uint8_t)((X & 0xFFFF) >> 8);
		pucData[2] = (uint8_t)((X & 0xFFFF) >> 0);
		pucData[3] = (uint8_t)((Y & 0xFFFF) >> 8);
		pucData[4] = (uint8_t)((Y & 0xFFFF) >> 0);
		*pucCmd = CP_CMD_TOUCHCOOR;
		*pucDatalen = 5;

		return 0;
	}
	return -1;
}

int32_t ComRecvCmd(Uart& ExtCom, uint8_t* pCmd, uint8_t* pData, uint8_t* pLen, int32_t iTimeout) {
	uint8_t ucBuf[256];
	int32_t iLen = 0;

	iLen = ComProtocolRecv_SW(ExtCom, ucBuf, 128, iTimeout);
	if (iLen > 6) {
		SW_CmdFrame_t* pSwCmdFrame = (SW_CmdFrame_t*)ucBuf;

		*pCmd = pSwCmdFrame->cmd;
		*pLen = pSwCmdFrame->len - 1;
		memcpy(pData, pSwCmdFrame->buf, *pLen);


		return 0;
	}

	return -1;
}

int32_t ComRecvCmd(CMyQueue& msgQueueRecv, uint8_t* pCmd, uint8_t* pData, uint8_t* pLen, int32_t iTimeout) {
	uint8_t ucBuf[256];
	int32_t iLen = 0;

	iLen = msgQueueRecv.RecvMsg(ucBuf, iTimeout);
	if (iLen != 0) {
		return -1;
	}
	SW_CmdFrame_t* pSwCmdFrame = (SW_CmdFrame_t*)ucBuf;

	*pCmd = pSwCmdFrame->cmd;
	*pLen = pSwCmdFrame->len - 1;
	memcpy(pData, pSwCmdFrame->buf, *pLen);

	return 0;
}

int32_t ComRecvAsk(CMyQueue& msgQueueRecv, uint8_t* pCmd, uint8_t* pData, uint8_t* pucLen, uint8_t* pucStatus, int32_t iTimeout) {
	uint8_t ucBuf[256];
	int32_t iLen = 0;

	iLen = msgQueueRecv.RecvMsg(ucBuf, iTimeout);
	if (iLen != 0) {
		return -1;
	}
	SW_AskFrame_t* pSwAskFrame = (SW_AskFrame_t*)ucBuf;

	*pCmd = pSwAskFrame->cmd;
	*pucLen = pSwAskFrame->len - 2;     // 减去cmd和status
	*pucStatus = pSwAskFrame->status;
	memcpy(pData, pSwAskFrame->buf, *pucLen);

	//printf("cmd=%02x,len=%d,status=%d.\r\n", *pCmd, *pucLen, *pucStatus);
	//for (int i = 0; i < *pucLen; i++) {
	//    printf("%02x ", pData[i]);
	//}
	//printf("\r\n");

	return 0;
}

#endif              // end of WIN32
