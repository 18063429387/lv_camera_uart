// =======================================================================================
//  Copyright (c) 2011 xxx All rights reserved. 
//  http://www.xxx.com
//! @file       RingBuffer.c
//! @author     ?????
//! @date       2011/10/01
//! @version    1.00
//! @brief      ???¦Ë???????????.
// =======================================================================================
// ------------------------------?????-------------------------------------------------
// Data:	2012/03/13
// Version:	V 1.01
// Brief:	???????????C?????????§¹??
// =======================================================================================
// Data:	2016/08/31
// Version:	V 1.02
// Brief:	????????????????????§Ø?????????????????
// =======================================================================================
#include <string.h>
#include "RingBuffer.h"
#include <stdlib.h>

// ===================?????????????============================
// #if WIN32 || 1
#if 1
#include "../../ref_inc/osport.h"
#else
#include "pthread.h"
#endif

int32_t CriticalInit(void ** pData)
{
	if (pData == NULL)
		return -1;
	
#if 1 || defined(WIN32)
    CMutex *pMutex = new CMutex;
    *pData = (void *)pMutex;
#else
	pthread_mutex_t *pmutex = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
	
	pthread_mutex_init(pmutex, NULL);
	*pData = (void *)pmutex;
#endif

	return 0;
}

int32_t CriticalUnInit(void ** pData)
{
#if 1 || defined(WIN32)
    CMutex *pMutex = *(CMutex **)pData;
    delete pMutex;
    *pData = NULL;
#else
	pthread_mutex_t *pmutex = *(pthread_mutex_t **)pData;
	
	pthread_mutex_destroy(pmutex);
	
	free((void *)pmutex);
    *pData = NULL;
#endif

	return 0;
}
int32_t CriticalEnter(void *pData)
{
#if 1 || defined(WIN32)
    CMutex *pMutex = (CMutex *)pData;
    pMutex->Enter();
    return 0;
#else
    //uint32_t priMask;
    //
    ////priMask = __get_PRIMASK();
    ////__disable_irq();
    //return priMask;
	pthread_mutex_t *pmutex = (pthread_mutex_t *)pData;
	
	return pthread_mutex_lock(pmutex);
#endif
}

int32_t CriticalExit(void *pData)
{
#if 1 || defined(WIN32)
    CMutex *pMutex = (CMutex *)pData;
    pMutex->Exit();
    return 0;
#else
    //__set_PRIMASK(priMask);
	pthread_mutex_t *pmutex = (pthread_mutex_t *)pData;
	return pthread_mutex_unlock(pmutex);
	// return 0;
#endif
}



int32_t RingBufferInit(RingBuffer *pRingBuffer, void *pBuf, uint32_t bufferSize)
{
    if ((void *)0 == pRingBuffer)
    {
        return -1;
    }
    
	if ((void *)0 == pBuf)
	{
		pRingBuffer->pBuf = (void *)malloc(bufferSize);
        if (pRingBuffer->pBuf == NULL) {
            printf("<WARNING> %s: malloc (%d) failed!\r\n", __FUNCTION__, bufferSize);
            return -1;
        }
    } else
    {
        pRingBuffer->pBuf = pBuf;
    }

	
    pRingBuffer->InputPos = 0;
    pRingBuffer->OutputPos = 0;
    pRingBuffer->Count = 0;
    //pRingBuffer->pBuf = pBuf;
    pRingBuffer->MaxBufferSize = bufferSize;

    return CriticalInit(&(pRingBuffer->pmutex));      
}
// release_buf_malloc?? ???pBuf????????????????1--free??pBuf??, 0--????????
// ?????RingBufferInit?????????????NULL???release_buf_malloc?????1.
int32_t RingBufferUnInit(RingBuffer *pRingBuffer, int32_t release_buf_malloc)
{
	int32_t err;
	
	if ((void *)0 == pRingBuffer)
    {
        return -1;
    }
	
	err = CriticalUnInit(&(pRingBuffer->pmutex));
	
	if (release_buf_malloc)
		free(pRingBuffer->pBuf);
	pRingBuffer->pBuf = NULL;
	
    pRingBuffer->InputPos = 0;
    pRingBuffer->OutputPos = 0;
    pRingBuffer->Count = 0;   
    pRingBuffer->MaxBufferSize = 0;

    return err;
}
uint32_t RingBufferRead(RingBuffer *pRingBuffer, void *pBuf, uint32_t maxSize)
{
    uint32_t temp, temp2;
	uint32_t size;
	uint8_t *_pBuf = (uint8_t *)pBuf;
	uint8_t *_pRingBuffer = (uint8_t *)pRingBuffer->pBuf;
    
    if ((void *)0 == pRingBuffer || (void *)0 == pBuf)
    {
        return 0;
    }
    
    CriticalEnter(pRingBuffer->pmutex);   
	if (0 == pRingBuffer->Count || 0 == maxSize)            // ????????0?????????
    {
		CriticalExit(pRingBuffer->pmutex);
        return 0;
    }
    
    size =  (pRingBuffer->Count > maxSize) ? maxSize : pRingBuffer->Count;

	temp = pRingBuffer->OutputPos + size;
	if (temp < pRingBuffer->MaxBufferSize)
	{
		memcpy(_pBuf, &_pRingBuffer[pRingBuffer->OutputPos], size);
		pRingBuffer->OutputPos = temp;
	}
	else
	{
		temp2 = pRingBuffer->MaxBufferSize - pRingBuffer->OutputPos;
		memcpy(_pBuf, &_pRingBuffer[pRingBuffer->OutputPos], temp2);
		pRingBuffer->OutputPos = temp - pRingBuffer->MaxBufferSize;
		memcpy(&_pBuf[temp2], &_pRingBuffer[0], pRingBuffer->OutputPos);
	}
	

	pRingBuffer->Count -= size;
    CriticalExit(pRingBuffer->pmutex);
    
    return size; 
}


uint32_t RingBufferTailRead(RingBuffer *pRingBuffer, void *pBuf, uint32_t maxSize)
{
    uint32_t temp;
	uint32_t size;
	uint8_t *_pBuf = (uint8_t *)pBuf;
	uint8_t *_pRingBuffer = (uint8_t *)pRingBuffer->pBuf;
    
    if ((void *)0 == pRingBuffer || (void *)0 == pBuf)
    {
        return 0;
    }

    CriticalEnter(pRingBuffer->pmutex);    
    if (0 == pRingBuffer->Count || 0 == maxSize)            // ????????0?????????
    {
		CriticalExit(pRingBuffer->pmutex);
        return 0;
    }
    
	size =  (pRingBuffer->Count > maxSize) ? maxSize : pRingBuffer->Count;
	
	if (pRingBuffer->InputPos < size) {
		temp = size - pRingBuffer->InputPos;	
		memcpy(_pBuf, &_pRingBuffer[pRingBuffer->MaxBufferSize - temp], temp);
		memcpy(&_pBuf[temp], &_pRingBuffer[0], pRingBuffer->InputPos);
		pRingBuffer->InputPos = pRingBuffer->MaxBufferSize - temp;
		
	}else {
		memcpy(_pBuf, &_pRingBuffer[pRingBuffer->InputPos - size], size);
		pRingBuffer->InputPos = pRingBuffer->InputPos - size;
	}
	pRingBuffer->Count -= size;											// ??????????§Ö????????	
    
	CriticalExit(pRingBuffer->pmutex);
    
    return size;	
}


// ?????????????§Õ??
uint32_t RingBufferWrite(RingBuffer *pRingBuffer, const void *pBuf, uint32_t maxSize)
{
    uint32_t temp,temp2;
    uint32_t size;
    uint32_t count;
	const uint8_t *_pBuf  = (uint8_t *)pBuf;
	uint8_t *_pRingBuffer = (uint8_t *)pRingBuffer->pBuf;

    if ((void *)0 == pRingBuffer || (void *)0 == pBuf)
    {
        return 0;
    }  
    CriticalEnter(pRingBuffer->pmutex);
//    ASSERT(pRingBuffer->MaxBufferSize < pRingBuffer->Count);
    count =  pRingBuffer->MaxBufferSize - pRingBuffer->Count;
    size =  (count > maxSize) ? maxSize : count;
	
	temp = pRingBuffer->InputPos + size;		
	if (temp < pRingBuffer->MaxBufferSize)									// §Õ??????????????????????????
	{
		memcpy(&_pRingBuffer[pRingBuffer->InputPos], _pBuf, size);		// §Õ???????????????????????????????›¥
		pRingBuffer->InputPos = temp;										// ???????????
	}
	else																	// §Õ????????????????????????
	{
		temp2 = pRingBuffer->MaxBufferSize - pRingBuffer->InputPos;
		memcpy(&_pRingBuffer[pRingBuffer->InputPos], _pBuf, temp2);		// ????????????§Õ??
		pRingBuffer->InputPos = temp - pRingBuffer->MaxBufferSize;				
		memcpy(&_pRingBuffer[0], &_pBuf[temp2], pRingBuffer->InputPos);	// ????????????????????¡ä????¦Ë?????›¥
																			// ????????????§³??????????????¦Ë??›¥????????
	}
    
	pRingBuffer->Count += size;											// ??????????§Ö????????
	CriticalExit(pRingBuffer->pmutex);
    
    return size;
}

// ?????????????§Õ??
uint32_t RingBufferWriteExt(RingBuffer* pRingBuffer, const void* pBuf, uint32_t maxSize)
{
    uint32_t temp, temp2;
    uint32_t size;
    uint32_t count;
    const uint8_t* _pBuf = (uint8_t*)pBuf;
    uint8_t* _pRingBuffer = (uint8_t*)pRingBuffer->pBuf;

    if ((void*)0 == pRingBuffer || (void*)0 == pBuf)
    {
        return 0;
    }
    CriticalEnter(pRingBuffer->pmutex);
    count = pRingBuffer->MaxBufferSize - pRingBuffer->Count;
    if (count < maxSize) {
        CriticalExit(pRingBuffer->pmutex);
        return 0;
    }

    size = maxSize;

    temp = pRingBuffer->InputPos + size;
    if (temp < pRingBuffer->MaxBufferSize)									// §Õ??????????????????????????
    {
        memcpy(&_pRingBuffer[pRingBuffer->InputPos], _pBuf, size);		// §Õ???????????????????????????????›¥
        pRingBuffer->InputPos = temp;										// ???????????
    }
    else																	// §Õ????????????????????????
    {
        temp2 = pRingBuffer->MaxBufferSize - pRingBuffer->InputPos;
        memcpy(&_pRingBuffer[pRingBuffer->InputPos], _pBuf, temp2);		// ????????????§Õ??
        pRingBuffer->InputPos = temp - pRingBuffer->MaxBufferSize;
        memcpy(&_pRingBuffer[0], &_pBuf[temp2], pRingBuffer->InputPos);	// ????????????????????¡ä????¦Ë?????›¥
                                                                            // ????????????§³??????????????¦Ë??›¥????????
    }

    pRingBuffer->Count += size;											// ??????????§Ö????????
    CriticalExit(pRingBuffer->pmutex);

    return size;
}

// ?????????????????????????

uint32_t RingBufferOverWrite(RingBuffer *pRingBuffer, const void *pBuf, uint32_t size)
{
    uint32_t i;
	const uint8_t *_pBuf  = (uint8_t *)pBuf;
	uint8_t *_pRingBuffer = (uint8_t *)pRingBuffer->pBuf;

	if ((void *)0 == pRingBuffer || (void *)0 == pBuf)
    {
        return 0;
    }
    
	CriticalEnter(pRingBuffer->pmutex);
	
	for (i=0; i<size; i++)
    {
        _pRingBuffer[pRingBuffer->InputPos++] = _pBuf[i];
        if (pRingBuffer->InputPos >= pRingBuffer->MaxBufferSize)           // ????????????§³?????
        {
            pRingBuffer->InputPos = 0;
        }

        if (pRingBuffer->Count < pRingBuffer->MaxBufferSize)
        {
            pRingBuffer->Count++;
        }
        else                                                                 // ????????????????????
        {
            pRingBuffer->OutputPos = pRingBuffer->InputPos;
        }       
    }
	
	CriticalExit(pRingBuffer->pmutex);
	return i;
}


uint32_t RingBufferGetCount(const RingBuffer *pRingBuffer)
{
	uint32_t cnt;
    
    CriticalEnter(pRingBuffer->pmutex);
	cnt = pRingBuffer->Count;
    CriticalExit(pRingBuffer->pmutex);
	
    return  cnt;
}

void RingBufferClear(RingBuffer *pRingBuffer)
{
    //uint32_t level;
    
    CriticalEnter(pRingBuffer->pmutex);
    pRingBuffer->InputPos = 0;
    pRingBuffer->OutputPos = 0;
    pRingBuffer->Count = 0;
    CriticalExit(pRingBuffer->pmutex);
}
