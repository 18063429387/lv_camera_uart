#ifndef __CAYENNECARDATA_H
#define __CAYENNECARDATA_H
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "../ref_inc/osport.h"

enum tappos {
	tappos_P = 0x00,
	tappos_R,
	tappos_N,
	tappos_D,
	tappos_unkown = 0x0f
};

enum ledstate {
	ledstate_alloff = 0,
	ledstate_lefton = 1,
	ledstate_righton = 2,
	ledstate_leftright = 3,
	ledstate_marker = 4,
	ledstate_near = 5,
	ledstate_far = 6
};

typedef struct carrundata {
	int         iAcc;
	tappos      Tappos;
	int         iLittleLight;       //  小灯
	int         iLowBeam;           //  近光灯
	int         iHighBeam;          //  远光灯
	ledstate    LedState;
	int         iDoorState;
	int         iSpeed;
	float       fAngle;
	int         iStartTime;          // 发动机启动时间（ms）
	int         iAutoPark;
	int         iCAN;
	int         iPKey;
}CARRUNDATA;


class CayenneCarData {
public:
	CayenneCarData() {
		m_iWheelbase = 2690;
		memset(m_ucBuf, 0, sizeof(m_ucBuf));
		Parse();
	}

	~CayenneCarData() {

	}

	void SetCarData(unsigned char* pucBuf, int iLen) {
		if (iLen != 8) {
			printf("<WARNING> %s: iLen = %d, not 8\r\n", __FUNCTION__, iLen);
			return;
		}
		m_Mutex.Enter();
		memcpy(&m_ucBuf[0], pucBuf, 8);
		m_iFlag = 1;
		m_Mutex.Exit();
	}

	void Sync() {
		m_Mutex.Enter();
		if (m_iFlag) {
			Parse();
			m_iFlag = 0;
		}
		m_Mutex.Exit();
	}

	tappos GetTaps() const {
		return m_Tappos;
	}

	ledstate GetLedState() const {
		return m_LedState;
	}

	int GetDoorState() const {
		return m_iDoorState;
	}

	int GetSpeed() const {
		return m_iSpeed;
	}

	float GetTripAngle() {
		return m_fAngle;
	}

	unsigned char LedsGet()
	{
		return m_Leds;
	}

	void SetWheelbase(int val)
	{
		m_iWheelbase = val;
	}

private:
	void Parse() {
		m_Tappos = (tappos)(m_ucBuf[0] & 0x0f);
		unsigned char ucTmp = m_ucBuf[1] & 0x3f;
		m_Leds = ucTmp;
		if (ucTmp == 0) {
			m_LedState = ledstate_alloff;
		}
		else if (ucTmp & 0x04) {
			m_LedState = ledstate_leftright;
		}
		else if (ucTmp & 0x01) {
			m_LedState = ledstate_lefton;
		}
		else if (ucTmp & 0x02) {
			m_LedState = ledstate_righton;
		}
		else {
			m_LedState = ledstate_alloff;
		}

		m_iDoorState = m_ucBuf[2] & 0x1f;
		m_iSpeed = m_ucBuf[3];

		if (m_ucBuf[4] != 0x80)
		{
			unsigned int uiTmp1 = (unsigned int)(m_ucBuf[4] << 24) | (m_ucBuf[5] << 16) | (m_ucBuf[6] << 8) | (m_ucBuf[7] << 0);
			m_fAngle = ((int)uiTmp1) / 1000.0f;
			m_fAngle = m_fAngle / 180.0f * 3.1415926f;
		}
		else {
			float fMaxAngle = asin(m_iWheelbase / 5070.0f) / 3.1415926f * 180.0f;
			signed short sTmp1 = (signed short)((m_ucBuf[6] << 8) | (m_ucBuf[7] << 0));
			m_fAngle = (sTmp1 / 4096.0f * fMaxAngle) / 180.0f * 3.1415926f;
		}
	}

private:
	int				m_iFlag;
	int				m_iDoorState;
	int				m_iSpeed;
	int				m_iWheelbase;
	tappos			m_Tappos;
	ledstate		m_LedState;
	unsigned char	m_Leds;
	unsigned char	m_ucBuf[8];
	float			m_fAngle;

	CMutex      m_Mutex;
};


#endif

