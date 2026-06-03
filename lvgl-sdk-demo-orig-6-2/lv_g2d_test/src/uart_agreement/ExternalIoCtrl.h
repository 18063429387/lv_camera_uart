#ifndef __EXT_IO_CTRL_H
#define __EXT_IO_CTRL_H
#include <stdint.h>
//#include "ComProtocols.h"
//#ifdef __cplusplus
//extern "C" {
//#endif   
typedef struct __IOCarInfo
{
	uint8_t gear : 4;
	uint8_t acc : 1;
	uint8_t reserveBits : 1;
	uint8_t can : 1;
	uint8_t accValid : 1;
	uint8_t lightstatus;
	uint8_t doorstatus;
	uint8_t speed;
} IOCarInfo_t;

extern IOCarInfo_t IOCtlCarInfo;

void* threadExtIoCtrl(void* p);
void ClearCarInfoValidFlag();

//#ifdef __cplusplus
//}
//#endif

#endif      // end of __EXT_IO_CTRL_H


