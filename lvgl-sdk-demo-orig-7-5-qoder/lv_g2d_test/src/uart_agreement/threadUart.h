#ifndef __THREADUART_H
#define __THREADUART_H
#include "ProcessCom.h"
#include "ComProt.h"
#include "comdatadefs.h"
#include "RemoteControl.h"
#include "uart.h"


#define MB_UPDATE_STATE_ING                 (int)1                       //  正在升级
#define MB_UPDATE_STATE_END                 (int)2                       //  升级结束，并已成功
#define MB_UPDATE_STATE_ERR_TRANSFER        (int)3                       //  升级失败: 传输失败
#define MB_UPDATE_STATE_ERR_WRITEFLASH      (int)4                       //  升级失败：烧写 FLASH 失败
#define MB_UPDATE_STATE_ERR_UNKNOW          (int)0xffu                   //  未知错误

struct mbupdateinfo {
	MyCritialData<int> UpdateFlag;
	MyCritialData<int> EndTransferLen;

	MyCritialData<int> UpdateState;

	int                iTotalTransferLen;
	unsigned short     usCrc16;
	unsigned char* pucBuf;
};

#ifdef __cplusplus
extern "C" {
#endif
	extern CMyQueue AvmMsgQueueRx;
	extern CMyQueue AvmMsgQueueTx;
	/*****************************************************************************
	  UART 的控制线程: 全景模式下
	*****************************************************************************/
	int threadControl_Uart(void* p);
	int threadAvmCom1(void* p);
	int threadAvmCom2(void* p);
	int threadAvmMsgQueueTx(void* p);
	
	void InitComPort(void);
	void PowerOffRequest(int iIsReboot = 0);
	void NotifyParkMonitorState(void);
	bool McuPortConnected();

#ifdef __cplusplus
}
#endif


#endif
