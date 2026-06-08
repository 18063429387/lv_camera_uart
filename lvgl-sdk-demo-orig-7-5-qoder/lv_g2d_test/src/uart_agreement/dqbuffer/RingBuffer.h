// =======================================================================================
//  Copyright (c) 2011 xxx All rights reserved. 
//  http://www.xxx.com
//! @file       RingBuffer.h
//! @author     张宏亮
//! @date       2011/10/01
//! @version    1.00
//! @brief      通用环形缓冲区相关代码头文件。
// =======================================================================================
#ifndef _RING_BUFFER_H_
#define _RING_BUFFER_H_

#include <stdint.h>

#ifdef __cplusplus  
extern "C" {  
#endif  

//==================================================

typedef struct _RingBuffer
{
    uint32_t InputPos;
    uint32_t OutputPos;
    uint32_t Count;
    void     *pBuf;
    uint32_t MaxBufferSize;  

	void *pmutex;
} RingBuffer;


int32_t RingBufferInit(RingBuffer *pRingBuffer, void *pBuf, uint32_t size);

// release_buf_malloc： 内部pBuf对应内存是否需要释放，1--free（pBuf）, 0--不处理。
// 注意：当RingBufferInit时，第二个参数为NULL时，release_buf_malloc必须为1.
int32_t RingBufferUnInit(RingBuffer *pRingBuffer, int32_t release_buf_malloc);

uint32_t RingBufferRead(RingBuffer *pRingBuffer, void *pBuf, uint32_t maxSize);

uint32_t RingBufferTailRead(RingBuffer *pRingBuffer, void *pBuf, uint32_t maxSize);

uint32_t RingBufferWrite(RingBuffer *pRingBuffer, const void *pBuf, uint32_t maxSize);

uint32_t RingBufferWriteExt(RingBuffer* pRingBuffer, const void* pBuf, uint32_t maxSize);

uint32_t RingBufferOverWrite(RingBuffer *pRingBuffer, const void *pBuf, uint32_t maxSize);

uint32_t RingBufferGetCount(const RingBuffer *pRingBuffer);

void RingBufferClear(RingBuffer *pRingBuffer);

#ifdef __cplusplus  
}  
#endif

#endif		// end of file
