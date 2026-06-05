#ifndef __CONTROL_H
#define __CONTROL_H
#include <stdio.h>
#include "../../ref_inc/osport.h"
#include "com_defs.h"

#define APP_MODE_NORMAL                 0x01        //  ?????
#define APP_MODE_CALIB                  0x02        //  ????
#define APP_MODE_PLAY                   0x03        //  ??????
#define APP_MODE_CARLIFE                0x04        //  CarLife
#define APP_MODE_BVTMP_FROM_PLAY        0x05        //  ?????????????????§Ý?????
#define APP_MODE_BVTMP_FROM_CARLIFE     0x06        //  ????????????CarLife???§Ý?????

#define APP_MODE_NORMAL_SUB_NORMAL      0
#define APP_MODE_NORMAL_SUB_MENU        1
#define APP_MODE_NORMAL_SUB_SYSTEM      2

#define APP_MODE_CALIB_SUB_FREERUN      0
#define APP_MODE_CALIB_SUB_START        1
#define APP_MODE_CALIB_SUB_DO           2
#define APP_MODE_CALIB_SUB_END          3

#define APP_MODE_CALIB_SUB_SUCESSED     4
#define APP_MODE_CALIB_SUB_FAILED       5

enum appstate {
    asNothing = 0,
    asReCreateEnv,
    asSaveImg,
    asCalibMode,
    asStartCalib,
    asKey,

    asLumAdj,
    asDisplay,
    asCarLifeMode,
    asPlayMode,
    asAppMode,
    asAppSubMode,
};

class AppControl {
public:
    AppControl() {
        init();
    }

    ~AppControl() {

    }

    void Set (appstate state, int num = 1) {
        m_Mutex.Enter();
        switch (state)
        {
        case asNothing:
            break;

        case asReCreateEnv:
            m_iReCreateEnv = num;
            break;

        case asSaveImg:
            m_iSaveImg = num;
            break;

        case asCalibMode:
            m_iCalibMode = num;
            break;

        case asStartCalib:
            m_iStartCalib = num;
            break;

        case asKey:
            m_iKey = num;
            printf("SetKey: 0x%x\r\n", num);
            break;

        case asLumAdj:
            m_iLumAdj = num;
            break;

        case asDisplay:
            m_iDisplay = num;
            if ( 1 || num == 0) {
                m_iClearDisplayDelay = 0;
            }
            break;

        case asCarLifeMode:
            m_iCarLifeMode = num;
            break;

        case asPlayMode:
            m_iPlayMode = num;
            break;

        case asAppMode:
            //  ??app????????????????
            if (m_iAppMode != num) {
                m_iAppSubMode = 0;
            }
            m_iAppMode = num;
            break;

        case asAppSubMode:
            m_iAppSubMode = num;
            break;

        default:
            break;
        }
        m_Mutex.Exit();
    }

    void Clear(appstate state) {
        Set(state, 0);
    }

    int Get (appstate state) {
        int num = 0;

        m_Mutex.Enter();
        switch (state)
        {
        case asNothing:
            break;

        case asReCreateEnv:
            num = m_iReCreateEnv;
            break;

        case asSaveImg:
            num = m_iSaveImg;
            break;

        case asCalibMode:
            num = m_iCalibMode;
            break;

        case asStartCalib:
            num = m_iStartCalib;
            break;

        case asKey:
            num = m_iKey;
            break;

        case asLumAdj:
            num = m_iLumAdj;
            break;

        case asDisplay:
            if (m_iDisplay) {
                if (m_iClearDisplayDelay) {
                    if ( GetTickCount() >= m_uiClearDisplayStamp + m_iClearDisplayDelay) {
                        m_iDisplay           = 0;
                        m_iClearDisplayDelay = 0;
                    }
                }

            } else {
                m_iClearDisplayDelay = 0;
            }
            num = m_iDisplay;
            break;

        case asCarLifeMode:
            num = m_iCarLifeMode;
            break;

        case asPlayMode:
            num = m_iPlayMode;
            break;

        case asAppMode:
            num = m_iAppMode;
            break;

        case asAppSubMode:
            num = m_iAppSubMode;
            break;

        default:
            break;
        }
        m_Mutex.Exit();

        return num;
    }
    
    void ClearDisplyDelay (int iDlyMs) {
        m_Mutex.Enter();
        if (iDlyMs > 0) {
            m_iClearDisplayDelay  = iDlyMs;
            m_uiClearDisplayStamp = GetTickCount();
        } else {
            m_iClearDisplayDelay = 0;
            m_iDisplay = 0;
        }
        m_Mutex.Exit();
    }


private:
    void init (void) {
        m_iReCreateEnv  = 0;
        
        m_iSaveImg      = 0;
        m_iCalibMode    = 0;
        m_iStartCalib   = 0;
        m_iKey          = 0;
                       
        m_iLumAdj       = 0;
        m_iDisplay      = 0;
         
        m_iCarLifeMode  = 0;
        m_iPlayMode     = 0;
                       
        m_iAppMode      = APP_MODE_NORMAL;
        m_iAppSubMode   = 0;

        m_iClearDisplayDelay = 0;
    }
    
private:
    int    m_iReCreateEnv;

    //  ??????????
    int    m_iSaveImg;
    int    m_iCalibMode;
    int    m_iStartCalib;
    int    m_iKey;

    //  ?????????????
    int    m_iLumAdj;

    //   ?????????
    int    m_iDisplay;

    //  CarLife
    int m_iCarLifeMode;

    //  ??????
    int    m_iPlayMode;

    int    m_iAppMode;
    int    m_iAppSubMode;

    int    m_iClearDisplayDelay;    //  ?????????
    unsigned int m_uiClearDisplayStamp;

private:
    CMutex m_Mutex;
};



#endif

