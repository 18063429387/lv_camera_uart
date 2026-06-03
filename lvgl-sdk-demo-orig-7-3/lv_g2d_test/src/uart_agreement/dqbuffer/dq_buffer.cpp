
#include "dq_buffer.h"
#include "stdlib.h"
#include "string.h"
// #include "osport.h"

int32_t dq_buf_init(dq_buf *p, const int8_t *pname, uint32_t max_cnt, uint32_t size)
{
	if (p == NULL)
		return -1;
	
	if (pname != NULL)
		strcpy((char *)p->name, (const char *)pname);
	
#if 0
	RingBufferInit(&p->full_queue, NULL, max_cnt * size);
	RingBufferInit(&p->empty_queue, NULL, max_cnt * size);
#else
    if (RingBufferInit(&p->full_queue, NULL, max_cnt * size) != 0)
    {
        RingBufferUnInit(&p->full_queue, 1);
        return -1;
    }
    if (RingBufferInit(&p->empty_queue, NULL, max_cnt * size) != 0)
    {
        RingBufferUnInit(&p->full_queue, 1);
        RingBufferUnInit(&p->empty_queue, 1);

        return -1;
    }
#endif

    CSem *pfull_sem  = new CSem;
    CSem *pempty_sem = new CSem;
    
    pfull_sem->Create("full sem",   0, max_cnt);
    pempty_sem->Create("empty sem", 0, max_cnt);

    p->full_sem  = (void *)pfull_sem;
    p->empty_sem = (void *)pempty_sem;

	p->size = size;
	
    p->pMutex = new CMutex;

    p->iFullQueueCnt    = 0;
    p->iEmptyQueueCnt   = 0;

	return 0;
}

void dq_buf_uninit(dq_buf *p)
{
	if (p == NULL)
		return;

	RingBufferUnInit(&p->full_queue, 1);
	RingBufferUnInit(&p->empty_queue, 1);

    CSem * pfull_sem  = (CSem *)p->full_sem;
    CSem * pempty_sem = (CSem *)p->empty_sem;

    pfull_sem->Delete();
    pempty_sem->Delete();

    delete p->pMutex;
}

int32_t dq_buf_get_empty(dq_buf *p, void *pdata, int timeout /*  -1 is wait forever */)
{
	uint32_t size;
	
    ((CSem *)p->empty_sem)->Pend(timeout);
	size = RingBufferRead(&p->empty_queue, pdata, p->size);
	if (size == 0)
	{	
		//size = RingBufferTailRead(&p->full_queue, pdata, p->size);
        ((CSem *)p->empty_sem)->Post();
	}
	if (size == p->size) {
        p->pMutex->Enter();
        p->iEmptyQueueCnt--;
        p->pMutex->Exit();
		return 0;
    }
	else
		return -1;
}

int32_t dq_buf_put_full(dq_buf *p, void *pdata)
{
	uint32_t cnt;
	
    cnt = RingBufferWrite(&p->full_queue, pdata, p->size);
	
	if (cnt != p->size)
		return -1;
	else {
        p->pMutex->Enter();
        p->iFullQueueCnt++;
        p->pMutex->Exit();
        ((CSem *)p->full_sem)->Post();
		return 0;
    }
}

int32_t dq_buf_get_full(dq_buf *p, void *pdata, int timeout /*  -1 is wait forever */)
{
	uint32_t cnt;

	((CSem *)p->full_sem)->Pend(timeout);
	cnt = RingBufferRead(&p->full_queue, pdata, p->size);
	if (cnt != p->size) {	
        ((CSem *)p->full_sem)->Post();
		return -1;
	} else {
        p->pMutex->Enter();
        p->iFullQueueCnt--;
        p->pMutex->Exit();

		return 0;
	}
}

int32_t dq_buf_put_empty(dq_buf *p, void *pdata)
{
	uint32_t cnt;
	
	cnt = RingBufferWrite(&p->empty_queue, pdata, p->size);
	
	if (cnt != p->size)
		return -1;
	else {
        p->pMutex->Enter();
        p->iEmptyQueueCnt++;
        p->pMutex->Exit();

        ((CSem *)p->empty_sem)->Post();
		return 0;	
    }
}

int32_t dq_buf_get_empty_cnt(dq_buf *p)
{
    int cnt = 0;
    p->pMutex->Enter();
    cnt = p->iEmptyQueueCnt;
    p->pMutex->Exit();
    return cnt;
}

int32_t dq_buf_get_full_cnt(dq_buf *p)
{
    int cnt = 0;
    p->pMutex->Enter();
    cnt = p->iFullQueueCnt;
    p->pMutex->Exit();
    return cnt;
}

