// =======================================================================================
//  Copyright (c) 2011 xxx All rights reserved. 
//  http://www.xxx.com
//! @file       dq_buffer.h
//! @author     zhl
//! @date       2011/10/01
//! @version    1.00
//! @brief      double queue buffer
// =======================================================================================
#ifndef _DQ_BUFFER_H_
#define _DQ_BUFFER_H_
#include <stdio.h>

#include "../../ref_inc/osport.h"
#include "RingBuffer.h"

#ifdef __cplusplus  
extern "C" {  
#endif 

typedef struct double_queue_buffer {
	uint8_t name[32];
	
	uint32_t size;
	RingBuffer full_queue;
	RingBuffer empty_queue;	

    int        iFullQueueCnt;
    int        iEmptyQueueCnt;

    void      *full_sem;
    void      *empty_sem;
    CMutex    *pMutex;
} dq_buf;

int32_t dq_buf_init(dq_buf *p, const int8_t *pname, uint32_t max_cnt, uint32_t size);

void dq_buf_uninit(dq_buf *p);

int32_t dq_buf_get_empty(dq_buf *p, void *pdata, int timeout /*  -1 is wait forever */);

int32_t dq_buf_put_full(dq_buf *p, void *pdata);

int32_t dq_buf_get_full(dq_buf *p, void *pdata, int timeout /*  -1 is wait forever */);

int32_t dq_buf_put_empty(dq_buf *p, void *pdata);

int32_t dq_buf_get_empty_cnt(dq_buf *p);

int32_t dq_buf_get_full_cnt(dq_buf *p);

#ifdef __cplusplus  
}  
#endif

class CDqbuffer
{
public:
    CDqbuffer() {
        m_pDqBuf = NULL;
    }

    ~CDqbuffer() {
        if (m_pDqBuf) delete m_pDqBuf;
    }

    CDqbuffer(char *pcName, int iMaxCnt, int iEleSize) {
        Create(pcName, iMaxCnt, iEleSize);
    }

    int Create (char *pcName, int iMaxCnt, int iEleSize) {
        if (m_pDqBuf) {
            printf("<WARNING> CDqbuffer: dq buf had been create!\r\n");
            return -1;
        }
        m_iQueLength = iMaxCnt;
        m_pDqBuf = new dq_buf;
        return dq_buf_init(m_pDqBuf, (int8_t *)pcName, (uint32_t)iMaxCnt, iEleSize);
    }

    int Delete (void) {
        delete m_pDqBuf;
        m_pDqBuf = NULL;
    }

    int GetLength(void) {
        return m_iQueLength;
    }

    int PutFull (void *pvData) {
        return dq_buf_put_full(m_pDqBuf, pvData);
    }

    int PutEmpty (void *pvData) {
        return dq_buf_put_empty(m_pDqBuf, pvData);
    }

    int GetFull (void *pvData, int timeout = -1) {
        return dq_buf_get_full(m_pDqBuf, pvData, timeout);
    }

    int GetEmpty (void *pvData, int timeout = -1) {
        return dq_buf_get_empty(m_pDqBuf, pvData, timeout);
    }

    void *GetHandle(void) {
        return (void *)m_pDqBuf;
    }

    int GetFullCnt (void) {
        return dq_buf_get_full_cnt(m_pDqBuf);
    }

    int GetEmptyCnt (void) {
        return dq_buf_get_empty_cnt(m_pDqBuf);
    }

private:
    dq_buf *m_pDqBuf;
    int m_iQueLength;
};

#endif 	// _DQ_BUFFERS_H_