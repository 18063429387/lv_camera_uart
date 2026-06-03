#ifndef __REMOTE_CONTROL_H
#define __REMOTE_CONTROL_H
#include <stdint.h>
#include "../ref_inc/osport.h"
typedef struct _RemoteControlVal
{
	uint8_t Id[3];
	uint8_t Index : 5;
	uint8_t ModeBits : 3;

	uint8_t KeyValue : 4;
	uint8_t KeyMode : 4;
	uint8_t Reserve[3];
} RemoteControlVal_t;

typedef struct _HandRemoteControlVal
{
	uint8_t Id[3];
	uint8_t Index : 5;
	uint8_t ModeBits : 3;
	uint8_t KeyValue;
	uint8_t TickCnt;
} HandRemoteControlVal_t;

class RemoteControl {
public:
	RemoteControl();
	~RemoteControl();
	int32_t RmtCtlRecvDataDeal(RemoteControlVal_t keyVal, uint8_t* pucCmd, uint8_t* ucBuf, uint8_t* ucLen);
	int32_t RmtCtlNoRecvDataDeal(uint8_t* pucCmd, uint8_t* ucBuf, uint8_t* ucLen);
	int32_t SetRmtCtlId(uint8_t* pId);
	uint32_t GetRmtCtlId(void);
	void Enable(void);
	void Disable(void);
	uint32_t IsEnable(void);
private:
	int32_t UpdateRmtCtlId(void);
private:
	uint32_t RmtCtlID;
	uint32_t ReadySaveIdFlag;
	uint32_t SetDispParamCnt;
	uint32_t SetDispKeyCnt;
	uint32_t RecvKeyCnt;
	uint32_t RmtCtlDisable;
	DWORD SystemTickCnt;
	DWORD SaveIdTimeoutCnt;
	uint8_t SetDispParamStep1;
	uint8_t SetDispParamStep2;
	uint8_t SetDispParamStep3;  //
};

#ifdef __cplusplus
extern "C" {
#endif

	int32_t GetRmtCtlId(uint8_t* pId);
	int32_t SetRmtCtlId(uint8_t* pId);
	int32_t RmtCtlDataDeal(RemoteControlVal_t keyVal);

#ifdef __cplusplus
}
#endif

#endif      // end of __REMOTE_CONTROL_H



