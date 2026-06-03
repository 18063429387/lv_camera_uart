#ifndef __CRC_H
#define __CRC_H

#ifdef __cplusplus
extern "C" {
#endif

	unsigned short CRC16(unsigned char* pucFrame, unsigned int usLen);

	/*********************************************************************************************************
	** Function name:       crc7Update
	** Descriptions:        计算CRC7
	** input parameters:    ucOri: 初始CRC值。若只计算pucData中的CRC值，此参数请传入0
	**                      pucData: 需要计算CRC的数据缓冲区
	**                      uiLen: 需要计算CRC的字节数
	** output parameters:   计算结果（CRC7值）
	** Returned value:      没有使用
	*********************************************************************************************************/
	unsigned char crc7Update(unsigned char ucOri, unsigned char* pucData, unsigned int uiLen);

	unsigned short mbGetCRC16(unsigned short crcInit, unsigned char* ptr, unsigned int len);


#ifdef __cplusplus
}
#endif

#endif

