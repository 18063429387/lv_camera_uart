#ifndef __OS_PORT_H
#define __OS_PORT_H
#include <stdio.h>
#include <string.h>
// #include "DbgPrint.h"

#ifdef WIN32
#include "windows.h"

class CThread
{
public:
    CThread() {
        m_h = INVALID_HANDLE_VALUE;
    }

    CThread(void *pfun, void *param) {
        Create(pfun, param);
    }

    int Create(void *pfun, void *param, int iStackSize = 0, int iPrior = 0) {
        m_h = CreateThread(NULL, iStackSize, (LPTHREAD_START_ROUTINE)pfun,    (void*)param, 0, NULL);
        if (m_h != INVALID_HANDLE_VALUE) {
            return 0;
        } else {
            return -1;
        }
    }

    int IsValid (void) {
        return m_h != INVALID_HANDLE_VALUE;
    }

private:
    HANDLE m_h;
};

class CSem 
{
public:
    CSem() {
        m_h = INVALID_HANDLE_VALUE;
    }

    CSem(char *pcName, int iInitCnt, int iMaxCnt) {
        Create(pcName, iInitCnt, iMaxCnt);
    }

    ~CSem() {
        Delete();
    }

    int Create(char *pcName, int iInitCnt, int iMaxCnt) {
        m_h = CreateSemaphore(NULL, iInitCnt, iMaxCnt, pcName);
        if (m_h != INVALID_HANDLE_VALUE) {
            return 0;
        } else {
            return -1;
        }
    }

    void Delete (void) {
        if (m_h != INVALID_HANDLE_VALUE) CloseHandle(m_h);
        m_h = INVALID_HANDLE_VALUE;
    }

    void Post (int iCnt = 1) {
        ReleaseSemaphore(m_h, iCnt, NULL);
    }

    int Pend (int iTimeOut = -1) {
        DWORD ret;
        if (iTimeOut < 0) {
            ret = WaitForSingleObject(m_h, INFINITE);
        } else {
            ret = WaitForSingleObject(m_h, iTimeOut);
        }

        if (ret == WAIT_OBJECT_0) {
            return 0;
        }
        return -1;
    }

    int IsValid (void) {
        return m_h != INVALID_HANDLE_VALUE;
    }

private:
    HANDLE m_h;

};

class CMutex {
public:
    CMutex() {
        m_Sem.Create(NULL, 1, 10);
    }

    void Enter (void) {
        m_Sem.Pend();
    }

    void Exit (void) {
        m_Sem.Post();
    }

private:
    CSem m_Sem;
};

#else           //  linux

#include <unistd.h>    
#include <pthread.h>    
#include <semaphore.h>    
#include <time.h>
#include <errno.h>    

class CThread
{
public:
    CThread() {
        m_h = -1;
    }

    CThread(void *pfun, void *param) {
        Create(pfun, param);
    }

    int Create(void *pfun, void *param, int iStackSize = 0, int iPrior = 0) {
        int res = 0;

        if (iStackSize < 0) iStackSize = 0;
        if (iPrior < 0) iPrior = 0;
        if (iStackSize || iPrior) {
            pthread_attr_t attr;
            pthread_attr_init(&attr);
            int iRt = 0;
            if (iStackSize) {
                iRt = pthread_attr_setstacksize(&attr, iStackSize);
                if (iRt != 0) {
                    // PRINTF_ERROR("<WARNING> %s: pthread_attr_setstacksize( %d ) failed!\r\n", __FUNCTION__, iStackSize);
                }
            }
            if (iPrior) {
                pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED); 
                struct sched_param sched_param;
                iRt = pthread_attr_setschedpolicy(&attr, SCHED_RR);
                if (iRt != 0) {
                    // PRINTF_ERROR("<WARNING> %s: pthread_attr_setschedpolicy( SCHED_RR ) failed!\r\n", __FUNCTION__);
                }
                pthread_attr_getschedparam(&attr, &sched_param);
                int iMaxPrior = sched_get_priority_max(SCHED_RR);
                //PRINTF("%s: max priority: %d\r\n", __FUNCTION__, iMaxPrior);
                if (iPrior > iMaxPrior) iPrior = iMaxPrior;
                sched_param.sched_priority = iPrior;
                iRt = pthread_attr_setschedparam(&attr, &sched_param);
                if (iRt != 0) {
                    // PRINTF_ERROR("<WARNING> %s: pthread_attr_setschedparam failed!\r\n", __FUNCTION__);
                }
            }
            res = pthread_create (&m_h, &attr, (void *(*)(void *))pfun, param); 
            pthread_attr_destroy(&attr);
        } else {
            res = pthread_create (&m_h, NULL, (void *(*)(void *))pfun, param);    
        }
        if (res == 0) {
            return 0;
        } else {
            return -1;
        }
    }

    int IsValid (void) {
        return m_h != -1;
    }
private:
    pthread_t m_h;
};

static void threadPrintState (const char *pcThreadName)
{
    int my_policy;
    struct sched_param my_param;

    pthread_getschedparam(pthread_self(), &my_policy, &my_param);
    // PRINTF("%s: thread_routine running at %s/%d\n", pcThreadName,
    //             (my_policy == SCHED_FIFO ? "FIFO"
    //             : (my_policy == SCHED_RR ? "RR"
    //             : (my_policy == SCHED_OTHER ? "OTHER"
    //             : "unknown"))),
    //             my_param.sched_priority);
}

class CSem 
{
public:
    CSem() {
        //m_h = 0;
    }

    ~CSem() {
        Delete();
    }

    CSem(char *pcName, int iInitCnt, int iMaxCnt) {
        Create(pcName, iInitCnt, iMaxCnt);
    }

    int Create(char *pcName, int iInitCnt, int iMaxCnt) {
        int res = sem_init(&m_h, 0, iInitCnt);
        if (res == 0) {
            return 0;
        } else {
            return -1;
        }
    }

    void Delete (void) {
        //if (m_h != 0) {
            sem_destroy(&m_h);
        //}
    }

    void Post (int iCnt = 1) {
        sem_post(&m_h);
    }

    int Pend (int iTimeOut = -1) {
        int ret;
        if (iTimeOut < 0) {
            ret = sem_wait(&m_h);
        } else if (iTimeOut == 0) {
            ret = sem_trywait(&m_h);
        } else {
            /* sem_timedwait?????????????? ????????????????????��?
               ?????????????????????????????????????????????????????
               ?????????????????sem_trywait + usleep??????????
            */
            //ret = sem_timedwait_millsecs(&m_h, iTimeOut);
            int i = 10;
            int j = iTimeOut * 1000 / i;  // ms to us
            if (j > 200000) {       // ??????????200ms??????????200ms??????????????
                j = 200000;
                //i = iTimeOut * 1000 / j;
            }
            i = (iTimeOut * 1000 + j / 2) / j;      // ????????
            while (i--) {
                ret = sem_trywait(&m_h);
                if (!ret) {
                    return ret;
                }
                usleep(j);
            }
        }
        return ret;
    }

private:
    sem_t m_h;

    int sem_timedwait_millsecs(sem_t *sem, long msecs)  
    {  
        struct timespec ts;  
        clock_gettime(CLOCK_REALTIME, &ts);  
        long secs = msecs/1000;  
        msecs = msecs%1000;  

        long add = 0;  
        msecs = msecs*1000*1000 + ts.tv_nsec;  
        add = msecs / (1000*1000*1000);  
        ts.tv_sec += (add + secs);  
        ts.tv_nsec = msecs%(1000*1000*1000);  

        return sem_timedwait(sem, &ts);  
    } 
};

class CMutex {
public:
    CMutex() {
        m_Sem.Create(NULL, 1, 10);
    }

    void Enter (void) {
        m_Sem.Pend();
    }

    void Exit (void) {
        m_Sem.Post();
    }

private:
    CSem m_Sem;
};

typedef long long DWORD;

static DWORD GetTickCount()
{
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts); // CLOCK_MONOTONIC单调递增地时间戳， ts包含秒和纳秒两部分

    return (DWORD)(ts.tv_sec * (DWORD)1000 + ts.tv_nsec / 1000000); // 转换为毫秒级时间戳
}

static void Sleep(DWORD dwMill)
{
    timeval timeOut;
    timeOut.tv_sec = dwMill / 1000;
    timeOut.tv_usec = (dwMill % 1000) * 1000;
    if (0 != select(0, NULL, NULL, NULL, &timeOut))
    {
        printf("Sleep failed!\r\n");
    }
}

static void SleepUs(DWORD dwMircro)
{
    timeval timeOut;
    timeOut.tv_sec = dwMircro / 1000000;
    timeOut.tv_usec = dwMircro * 1000;
    if (0 != select(0, NULL, NULL, NULL, &timeOut))
    {
        printf("SleepUs failed!\r\n");
        //  return -1;
    }
    //return 0;
}


#endif

struct myqueue {
    int iFront;
    int iRear;
    int iEleSize;
    int iQueueLength;
    char name[64];
    unsigned char *pucData;
};
typedef struct myqueue MYQUEUE;

class CMyQueue
{
public:
    CMyQueue() {
       memset(&m_Queue, 0, sizeof(m_Queue));
    }

    CMyQueue (char *name, int iEleSize, int iQueueLength) {
        Create(name, iEleSize, iQueueLength); 
    }

    ~CMyQueue() {
        Delete();
    }

    int Create (char *name, int iEleSize, int iQueueLength) {
        if (iEleSize <= 0) {
            return -1;
        }
        if (iQueueLength <= 0) {
            return -1;
        }
        m_Sem.Create(name, 0, iQueueLength);
        init(name, iEleSize, iQueueLength);

        return 0;
    }

    int RecvMsg (void *pvMsg, int iWaitTime = -1) {
        m_Sem.Pend(iWaitTime);
        int iRt = -1;
        //printf("%s ,befor Enter+++++++++++++++\n", __FUNCTION__ );
        m_Muetx.Enter();
        //printf("%s ,after Enter+++++++++++++++\n", __FUNCTION__ );
        if (m_Queue.iFront == m_Queue.iRear) {
            iRt = -1;
        } else {
            //printf("%s index=%d********\n", __FUNCTION__, GetIndex(m_Queue.iFront));
            memcpy(pvMsg, m_Queue.pucData + m_Queue.iEleSize * GetIndex(m_Queue.iFront), m_Queue.iEleSize);
            m_Queue.iFront++;
            iRt = 0;
        }
        m_Muetx.Exit();
        return iRt;
    }

    int RecvNewestMsgAndClear (void *pvMsg, int iWaitTime = -1) {
        m_Sem.Pend(iWaitTime);
        int iRt = -1;
        m_Muetx.Enter();

        if (m_Queue.iFront == m_Queue.iRear) {
            iRt = -1;
        } else {
            int iFrontBak = m_Queue.iFront;
            while (m_Queue.iFront != m_Queue.iRear) {
                iFrontBak = m_Queue.iFront;
                m_Queue.iFront++;
            }
            m_Queue.iFront = iFrontBak;
            memcpy(pvMsg, m_Queue.pucData + m_Queue.iEleSize * GetIndex(m_Queue.iFront), m_Queue.iEleSize);
            m_Queue.iRear  = 0;
            m_Queue.iFront = 0;
            iRt = 0;
        }
        m_Muetx.Exit();
        return iRt;
    }

    int PostMsg (void *pvMsg) {
        int iRt = -1;
        //printf("%s ,befor Enter+++++++++++++++\n", __FUNCTION__ );
        m_Muetx.Enter();
        //printf("%s ,after Enter+++++++++++++++\n", __FUNCTION__ );
        
        if (m_Queue.iRear > m_Queue.iFront && GetIndex(m_Queue.iRear) == GetIndex(m_Queue.iFront)) {
            //printf("%s, %d: %s queue full, not write data!\r\n", __FUNCTION__, __LINE__, m_Queue.name);
            iRt = -1;
        } else {
            //printf("%s index=%d********\n", __FUNCTION__, GetIndex(m_Queue.iRear));
            memcpy(m_Queue.pucData + m_Queue.iEleSize * GetIndex(m_Queue.iRear), pvMsg, m_Queue.iEleSize);
            m_Queue.iRear++;

            //  ??????????????
            if (m_Queue.iRear > m_Queue.iQueueLength && m_Queue.iFront > m_Queue.iQueueLength) {
                m_Queue.iRear  -= m_Queue.iQueueLength;
                m_Queue.iFront -= m_Queue.iQueueLength;
            }
            iRt = 0;
        }
        m_Muetx.Exit();

        if (iRt >= 0) {
            m_Sem.Post();
        }
        return iRt;
    }

    void Delete (void) {
        if (m_Queue.pucData) delete [] m_Queue.pucData;
        memset(&m_Queue, 0, sizeof(m_Queue));
    }

private:
    CSem    m_Sem;
    CMutex  m_Muetx;
    MYQUEUE m_Queue;

private:
    void init (char *name, int iEleSize, int iQueueLength) {
        memset(&m_Queue, 0, sizeof(m_Queue));
        if (name) {
            if (strlen(name) >= sizeof(m_Queue.name)) {
                memcpy(&m_Queue.name[0], name, sizeof(m_Queue.name) - 1);
                m_Queue.name[sizeof(m_Queue.name) - 1] = 0;
            } else {
                strcpy(m_Queue.name, name);
            }
        } else {
            strcpy(m_Queue.name, "default");
        }

        m_Queue.iEleSize     = iEleSize;
        m_Queue.iQueueLength = iQueueLength;
        m_Queue.pucData      = new unsigned char [iEleSize * iQueueLength]();

        m_Queue.iRear  = 0;
        m_Queue.iFront = 0;
    }

    int GetIndex (int index) {
        return index % m_Queue.iQueueLength;
    }
};

class CMutexAuto {
public:
    CMutexAuto() {
        m_pMutex = NULL;
    }

    CMutexAuto(CMutex *pMutex) {
        m_pMutex = pMutex;
        if (m_pMutex) {
            m_pMutex->Enter();
        }
    }

    ~CMutexAuto (void) {
        if (m_pMutex) {
            m_pMutex->Exit();
        }
    }

private:
    CMutex *m_pMutex;
};
#endif

