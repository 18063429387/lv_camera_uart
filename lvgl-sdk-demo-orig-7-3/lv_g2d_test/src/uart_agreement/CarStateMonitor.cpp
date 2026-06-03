//  用于监控车身状态，开关屏等
#include <stdio.h>
#include <string.h>

#include "CarStateMonitor.h"
#include "CSetings.h"
#include "threadUart.h"
#include "tool/disk_manage.h"

#ifndef WIN32
#include "CMyFile.h"
#endif // !WIN32

void* threadRoutinueMain(void* p)
{
	while (1) {
		GCarMonitor.RoutinueMain();
	}
	return 0;
}

void* threadRoutineOnOff(void* p)
{
	while (1) {
		GCarMonitor.RoutineOnOff();
	}
	return 0;
}

#ifndef WIN32
static void RestartVideoRecord()
{
	// vrEnableRecord(0);
	// int i = 0;
	// while (vrIsDisEnableDone() == 0) {
	// 	i++;
	// 	if (i > 100 * 20)
	// 		break;
	// 	Sleep(10);
	// }
	// vrEnableRecord(1);
}
#endif

CCarStateMonitor::CCarStateMonitor()
{
	m_CloseAndQuit = false;
	m_iScreenOnOff = 0;
	m_iScreenOnType = screenontype_normal;

	memset(&m_CarDataCur, 0, sizeof(m_CarDataCur));
	memset(&m_CarDataNewest, 0, sizeof(m_CarDataCur));

	m_iSpeedThres = 15;           //  15km/h 时关闭 360

	memset(m_ucRadarSegMin, 0, sizeof(m_ucRadarSegMin));
	m_ucRadarTriggerLevel[0] = 255;
	m_ucRadarTriggerLevel[1] = 255;
	m_ucRadarTriggerLevel[2] = 255;
	m_ucRadarTriggerLevel[3] = 255;
	m_pfunOnOff = onoff_default;

	m_uiIdleTime[screenontype_boot] = 3000;
	m_uiIdleTime[screenontype_normal] = 7000;         //  无操作后 7 秒关闭 360
	m_uiIdleTime[screenontype_taps] = 3000;
	m_uiIdleTime[screenontype_led] = 100;
	m_uiIdleTime[screenontype_door] = 3000;
	m_uiIdleTime[screenontype_radar] = 3000;         //  雷达打开的 360，在雷达消失后 3 秒关闭 360
	m_uiIdleTime[screenontype_leddouble] = 0x7fffffff;
	m_uiCarStartTime = 12000;
	m_tLastCANOffTime = 0;
	m_ACCOffTime = 0;
	m_subTypeBoot = subtype_boot_rot_wait;

	//  车身数据
	memset(&m_ucCarData[0], 0, sizeof(m_ucCarData));
	m_iCarDataFlag = 0;

	m_QueueOnOff.Create("onoff", sizeof(ONOFF_EVT), 64);
#if defined(WIN32)
	m_CarDataCur.iAcc = 1;
	m_CarDataCur.iStartTime = 2;
#endif
	m_bLedDoubleEn = true;
	m_bBootRotateEn = true;
	m_AbnormalCnt = 0;
	m_ForcedExit = false;
	m_RadarPause = false;
	m_iVoltage = -1;
	m_parkMonitorNotifyTime = 0;
	m_iLowSpeed = 0;
}

CCarStateMonitor& CCarStateMonitor::instance()
{
	static CCarStateMonitor _instance;
	return _instance;
}

void CCarStateMonitor::Init(pfunOnOff fun)
{
	m_CloseAndQuit = false;
	m_pfunOnOff = fun;

	//                                           boot,  normal, taps,  led,  door,  radar, leddouble, end
	unsigned int uiIdleTime[screenontype_end] = { 5000,  10000,  10000, 1000, 5000,  5000,  999999999 };

	INIPanoramaExit udtExit;
	GCSetings.GetPanoramaExitTime(udtExit);

	uiIdleTime[screenontype_boot] = udtExit.tap;
	uiIdleTime[screenontype_taps] = udtExit.tap;
	uiIdleTime[screenontype_led] = udtExit.led;
	uiIdleTime[screenontype_remote] = udtExit.remote;

	SetTimeParam(0, uiIdleTime, 1);

	SetSpeedThres(GCSetings.GetLogicSpeed());
	SetLedDoubleSwitch(GCSetings.GetLedDoubleSwitchState());
	if (GCSetings.GetBootRotate() == 0)
		SetBootRotate(false);

	SetRearSwitch360(GCSetings.GetRearSwitch360());
	SetDoubleDelay(GCSetings.GetDoubleDelayEn());
	SetDoubleDelayTime(GCSetings.GetDoubleDelayTime());
	SetLowSpeed(GCSetings.GetLowSpeed());
	SetSteerAngle(GCSetings.GetSteerAngle());

	int nRadarVal = GCSetings.GetRadarTriggerDistance();
	SetRadarTriggerLevel(nRadarVal, nRadarVal, nRadarVal, nRadarVal);
	m_iSpeedSteer = -1;
}

void CCarStateMonitor::Start()
{
	m_CloseAndQuit = false;
	m_CarDataCur.iAcc = 1;
	m_CarDataCur.iAutoPark = 0;
	m_CarDataCur.iCAN = CAN_ACTIVED;
	m_uiAccOnTimeStamp = GetTickCount();
	m_CarState = carstate_accon_rot360;
	m_iScreenOnType = screenontype_boot;
	m_subTypeBoot = subtype_boot_rot360;
	m_uiTimeStampLastOp = GetTickCount();

	INILogic udtLogic;
	GCSetings.GetLogic(udtLogic);
	bool dispRoate = false;
#ifndef WIN32
	CMyFile file;
	file.open("/tmp/disp_rotate.txt", "rb");
	if (file.IsOpen()) {
		// 因旋转屏而引起的bvapp重启，不应该旋转车模
		dispRoate = true;
		file.close();
		system("rm /tmp/disp_rotate.txt");
	}
#endif

	if (m_bBootRotateEn) {
		if (m_CarDataNewest.Tappos == tappos_R) {
			// qMainFrame.SetFunction(car_taps_R);
		}
		else {
			// if (m_CarDataNewest.LedState == ledstate_lefton && udtLogic.left != bvlogic_none)
			// 	qMainFrame.SetFunction(car_led_left);
			// else if (m_CarDataNewest.LedState == ledstate_righton && udtLogic.right != bvlogic_none)
			// 	qMainFrame.SetFunction(car_led_right);
			// else if (m_CarDataNewest.LedState == ledstate_leftright && GCSetings.GetLedDoubleSwitchState())
			// 	qMainFrame.SetFunction(car_led_doubleflash);
			// else 
			// {
			// 	if (!dispRoate) 
			// 		qMainFrame.SetFunction(car_default);
			// }
		}
		OnOff(1);
	}
	else {
		if (m_CarDataNewest.Tappos == tappos_R) {
			// qMainFrame.SetFunction(car_taps_R);
		}
		else
		{
			// if (m_CarDataNewest.LedState == ledstate_lefton && udtLogic.left != bvlogic_none)
			// 	qMainFrame.SetFunction(car_led_left);
			// else if (m_CarDataNewest.LedState == ledstate_righton && udtLogic.right != bvlogic_none)
			// 	qMainFrame.SetFunction(car_led_right);
			// else if (m_CarDataNewest.LedState == ledstate_leftright && GCSetings.GetLedDoubleSwitchState())
			// 	qMainFrame.SetFunction(car_led_doubleflash);
			// else
			// 	qMainFrame.SetFunction(car_taps_D);
		}
		m_subTypeBoot = subtype_boot_front;
		OnOff(0);
	}

	m_AbnormalCnt = 0;
	m_ForcedExit = false;
	m_RadarPause = false;
#ifndef WIN32
	if (GCSetings.GetRecord()) {
		// ParkMonitorSetFrameRate(-1);
		if (!DiskManage::IsMount()) {
			DiskManage::DoMount(NULL);
		}
		// if (asGet(asAppMode) != APP_MODE_PLAY) {
		// 	RestartVideoRecord();
		// }
	}
#endif
}
//转一圈所需的时间，用于预估启动旋转后，什么时候切换至前视 
//操作空闲多久后关闭 360 显示
//车机从发动机启动到完成Log显示所需时间

void CCarStateMonitor::SetTimeParam(unsigned int uiRot360Time, unsigned int uiIdleTime[screenontype_end], unsigned int uiCarStartTime)
{
	uiIdleTime[screenontype_steering] = uiIdleTime[screenontype_led];
	uiIdleTime[screenontype_low_speed] = uiIdleTime[screenontype_led];
	memcpy(&m_uiIdleTime[0], &uiIdleTime[0], sizeof(m_uiIdleTime));
	m_uiCarStartTime = uiCarStartTime;

	printf("screenontype_boot=%d\n", uiIdleTime[screenontype_boot]);
	printf("screenontype_normal=%d\n", uiIdleTime[screenontype_normal]);
	printf("screenontype_taps=%d\n", uiIdleTime[screenontype_taps]);
	printf("screenontype_led=%d\n", uiIdleTime[screenontype_led]);
	printf("screenontype_leddouble=%d\n", uiIdleTime[screenontype_leddouble]);
	printf("screenontype_remote=%d\n", uiIdleTime[screenontype_remote]);
	printf("screenontype_steering=%d\n", uiIdleTime[screenontype_steering]);
	printf("screenontype_low_speed=%d\n", uiIdleTime[screenontype_low_speed]);
}

void CCarStateMonitor::SetSpeedThres(int iSpeed)
{
	m_iSpeedThres = iSpeed;
}

void CCarStateMonitor::SetLedDoubleSwitch(bool bEn)
{
	m_bLedDoubleEn = bEn;
}

void CCarStateMonitor::SetBootRotate(bool bEn)
{
	m_bBootRotateEn = bEn;
}

void CCarStateMonitor::SetRadarTriggerLevel(unsigned char front, unsigned char rear, unsigned char left, unsigned char right)
{
	m_ucRadarTriggerLevel[0] = front;
	m_ucRadarTriggerLevel[1] = rear;
	m_ucRadarTriggerLevel[2] = left;
	m_ucRadarTriggerLevel[3] = right;
}

void CCarStateMonitor::SetRadar(int iRadarPos, unsigned char* pucBuf, int iRadarNum, int iIsBufPack)
{
	if (iRadarPos < 0 || iRadarPos >= 4)
		return;

	CMutexAuto mutexAuto(&m_Mutex);

	//  提取雷达最近的段数（最危险的段），以该段来决定是否打开 360
	unsigned char ucSeg, ucSegMin = 255;
	for (int i = 0; i < iRadarNum; i++) {
		if (iIsBufPack == 0) {
			ucSeg = pucBuf[i * 2] & 0x0fu;              //  第1字节低4位为段数, 第2字节为距离
		}
		else {
			if ((i & 0x01) == 0) {
				ucSeg = (pucBuf[i >> 1] >> 4) & 0x0fu;  //  高4位雷达1，低4位雷达2
			}
			else {
				ucSeg = (pucBuf[i >> 1] >> 0) & 0x0fu;
			}
		}

		if (ucSeg >= 1 && ucSeg <= 0x0d) {
			if (ucSeg < ucSegMin) {
				ucSegMin = ucSeg;
			}
		}
	}
	m_ucRadarSegMin[iRadarPos] = (ucSegMin == 255) ? 0 : ucSegMin;

	//  雷达开屏模式下，需要更新最后一次雷达报警的时间
	if (m_iScreenOnType == screenontype_radar && m_iScreenOnOff) {
		if (m_ucRadarSegMin[iRadarPos] != 0 && m_ucRadarSegMin[iRadarPos] != 0x0e && m_ucRadarSegMin[iRadarPos] <= m_ucRadarTriggerLevel[iRadarPos]) {
			m_uiTimeStampLastOp = GetTickCount();
		}
	}
	if (m_RadarPause && m_CarDataNewest.iSpeed <= m_iSpeedThres)
	{
		// 如果雷达当前是暂停状态，那么当所有雷达均无报警信号时，将暂停状态复原成正常状态
		if (!((m_ucRadarSegMin[0] != 0 && m_ucRadarSegMin[0] != 0x0e && m_ucRadarSegMin[0] <= m_ucRadarTriggerLevel[0]) ||
			(m_ucRadarSegMin[1] != 0 && m_ucRadarSegMin[1] != 0x0e && m_ucRadarSegMin[1] <= m_ucRadarTriggerLevel[1]) ||
			(m_ucRadarSegMin[2] != 0 && m_ucRadarSegMin[2] != 0x0e && m_ucRadarSegMin[2] <= m_ucRadarTriggerLevel[2]) ||
			(m_ucRadarSegMin[3] != 0 && m_ucRadarSegMin[3] != 0x0e && m_ucRadarSegMin[3] <= m_ucRadarTriggerLevel[3]))) {

			m_RadarPause = false;
		}
	}
}

void CCarStateMonitor::SetCarData(unsigned char* pucData, int iLen)
{
	if (iLen > sizeof(m_ucCarData) / sizeof(m_ucCarData[0])) {
		return;
	}
	//  此处不用 CMutexAuto mutexAuto(&m_Mutex), 因希望能在最后 m_Sem.Post() 之前 Exit()
	m_Mutex.Enter();

	if (pucData[0] & 0x80) {
		m_CarDataNewest.iAcc = !!(pucData[0] & (1 << 4));
		m_CarDataNewest.iAutoPark = !!(pucData[0] & (1 << 5));
		m_CarDataNewest.iCAN = !!(pucData[0] & (1 << 6));
		if (iLen == 9)
			m_CarDataNewest.iStartTime = pucData[8] * 200;
		else if (iLen == 8)
			m_CarDataNewest.iStartTime = m_uiCarStartTime + 1;
	}
	else {
		m_CarDataNewest.iAcc = 1;
		m_CarDataNewest.iAutoPark = !!(pucData[0] & (1 << 5));
		m_CarDataNewest.iCAN = CAN_ACTIVED;
		m_CarDataNewest.iStartTime = m_uiCarStartTime + 1;
	}

	if (m_bRearSwitch360 || ((pucData[0] & 0x0f) != tappos_R))
		m_CarDataNewest.Tappos = (tappos)(pucData[0] & 0x0f); //开启R档触发或当前不为R档时，则传入当前档位信息
	else if (m_CarDataNewest.Tappos == tappos_R)
		m_CarDataNewest.Tappos = tappos_P; // 关闭R档触发且当前为R档时，如果已传入R档，则将其清为P档

	if (pucData[1] & 0x01) {
		m_CarDataNewest.LedState = ledstate_lefton;
	}
	else if (pucData[1] & 0x02) {
		m_CarDataNewest.LedState = ledstate_righton;
	}
	else if (pucData[1] & 0x04) {
		m_CarDataNewest.LedState = ledstate_leftright;
	}
	else
		m_CarDataNewest.LedState = ledstate_alloff;

	m_CarDataNewest.iLittleLight = !!(pucData[1] & 0x08);
	m_CarDataNewest.iLowBeam = !!(pucData[1] & 0x10);
	m_CarDataNewest.iHighBeam = !!(pucData[1] & 0x20);
	m_CarDataNewest.iPKey = !!(pucData[1] & 0x40);
	m_CarDataNewest.iDoorState = pucData[2] & 0x1f;
	m_CarDataNewest.iSpeed = pucData[3]; // pucData[3] == 255 ? 0 : pucData[3];  //  根据协议文档，255 为底板无法采集到车身数据时的无效数据

	unsigned int uiTmp1 = (unsigned int)(pucData[4] << 24) | (pucData[5] << 16) | (pucData[6] << 8) | (pucData[7] << 0);
	m_CarDataNewest.fAngle = ((int)uiTmp1) / 1000.0f;
	m_CarDataNewest.fAngle = m_CarDataNewest.fAngle / 180.0f * 3.1415926f;

	m_iCarDataFlag = 1;

	m_Mutex.Exit();
	m_Sem.Post();
}

void CCarStateMonitor::SetOpType(caroptype opType)
{
	unsigned int uiTimeStampCur = GetTickCount();
	// int iAppMode = asGet(asAppMode);
	CMutexAuto mutexAuto(&m_Mutex);
	if (m_iScreenOnOff == 0) {
		if (opType == caroptype_open && m_CarDataNewest.iAcc != 0) {
			m_iScreenOnType = screenontype_leddouble;
			m_uiTimeStampLastOp = GetTickCount();
			OnOff(1);
		}
		else if (opType == caroptype_close_quit) {
			// printf("caroptype_close_quit!\n");
			m_CloseAndQuit = true;
			OnOff(0);
		}
	}
	else {
		if (opType == caroptype_touch) {
			if (m_iScreenOnType != screenontype_leddouble &&
				m_iScreenOnType != screenontype_led &&
				m_iScreenOnType != screenontype_taps &&
				m_iScreenOnType != screenontype_remote) {
				m_iScreenOnType = screenontype_normal;
			}
			m_uiTimeStampLastOp = GetTickCount();
		}
		else if (opType == caroptype_close) {
			m_iScreenOnType = screenontype_normal;
			m_uiTimeStampLastOp = GetTickCount();
			// if (!(SystemIsUpating() ||      // 正在升级
			// 	iAppMode == APP_MODE_CALIB ||      //  标定状态
			// 	iAppMode == APP_MODE_PLAY ||      //  播放状态
			// 	(qApp.getViewMode() == viewmode_setting && m_CarDataNewest.iSpeed < m_iSpeedThres))
			// 	&& m_CarDataNewest.Tappos != tappos_R
			// 	&& checkRadar(m_CarDataNewest, m_CarDataCur) != 1
			// 	) {
			// 	m_iScreenOnType = screenontype_normal;
			// 	m_uiTimeStampLastOp = uiTimeStampCur;
			// 	OnOff(0);
			// }
		}
		else if (opType == caroptype_close_force) {
			printf("caroptype_close_force!\n");
			OnOff(0);
			m_iScreenOnType = screenontype_normal;
			m_ForcedExit = true;
			if (checkRadar(m_CarDataNewest, m_CarDataCur)) {
				m_RadarPause = true;
			}
		}
		else if (opType == caroptype_close_quit) {
			// printf("caroptype_close_quit!\n");
			m_CloseAndQuit = true;
			OnOff(0);
		}
	}
}

void CCarStateMonitor::SetVoltage(int iVoltage)
{
	m_iVoltage = iVoltage;
}

void CCarStateMonitor::SetKey(unsigned char* pucKeyData, int iLen)
{
	if (!pucKeyData || iLen <= 0) return;

	int key = pucKeyData[0];
	int key2 = 0;
	if (iLen >= 2) {
		key2 = pucKeyData[1];
	}

	unsigned int uiCurTime = GetTickCount();
	// int iAppMode = asGet(asAppMode);

	//CMutexAuto mutexAuto(&m_Mutex);
	m_Mutex.Enter();
	if (m_iScreenOnOff == 0) {
		if (key == 0x03 /* 确认键 */ || key == 0x09 /* 电源键, 遥控器长按确定时会发此值 */ || key == 0x39) {
			//if (key == 0x09 /* 电源键, 遥控器长按确定时会发此值 */) {
			if (m_uiLastKeyOffScreenTime + SCREENOFF_KEY_INTERAL_TIME > uiCurTime) {    //  距离上一次用按键关屏的时间太短，忽略
				m_Mutex.Exit();
				return;
			}
			m_iScreenOnType = screenontype_remote;

			// if (key == 0x39)
			// 	qMainFrame.SetFunction(car_limit);
			// else
			// 	qMainFrame.SetFunction(car_remote);

			m_uiTimeStampLastOp = uiCurTime;
			m_ForcedExit = false;
			OnOff(1);
		}
		else if (iLen >= 2 && key == 0x32 && (IsOpenKey(key2) || key2 == 0x20 /* 取反360状态 */)) {
			/*
			 *  有些客户需要在默认到某种视图，在此处添加。或者根据当前档位和灯状态决定？
			 */
			 // TODO: 切换到某个视图
			 // 以双闪的方式来处理，只有双击双闪或者发“关闭360”都会关闭
			 // 这里处理的都是语音控制命令，在threadUart已经处理了视图，这里就不做处理了
			m_iScreenOnType = screenontype_leddouble;
			m_uiTimeStampLastOp = uiCurTime;
			m_ForcedExit = false;
			//qMainFrame.SetFunction(car_led_doubleflash);
			OnOff(1);
		}
	}
	else {
		if (key == 0x09 /* 电源键, 遥控器长按确定时会发此值 */)
		{
			m_iScreenOnType = screenontype_normal;
			m_uiLastKeyOffScreenTime = uiCurTime;

			// qMainFrame.SetFunction(car_360_exit);

			// if (!(SystemIsUpating() || iAppMode == APP_MODE_CALIB || iAppMode == APP_MODE_PLAY))
			// {
			// 	m_Mutex.Exit();
			// 	SetOpType(caroptype_close_force);
			// 	return;
			// }
		}
		else if (key == 0x04 /* 锟斤拷锟截硷拷 */)
		{
			// if (!(SystemIsUpating() ||      // 正在升级
			// 	iAppMode == APP_MODE_CALIB ||      //  标定状态
			// 	iAppMode == APP_MODE_PLAY ||      //  播放状态
			// 	qApp.getViewMode() == viewmode_setting))
			// {
			// 	m_Mutex.Exit();
			// 	SetOpType(caroptype_close_force);
			// 	return;
			// }
		}
		else if (iLen >= 2 && key == 0x32 && (key2 == 1 /* 关360 */ || key2 == 0x20 /* 取反360状态 */))
		{
			//  关闭360按键值，如声控时会发此值，如果车上有确定的按键，也可以发此值
			//  以双闪的方式来处理，只有双击双闪或者发“关闭360”都会关闭
			m_iScreenOnType = screenontype_normal;
			// if (!(SystemIsUpating() ||      // 正在升级
			// 	iAppMode == APP_MODE_CALIB ||      //  标定状态
			// 	iAppMode == APP_MODE_PLAY ||      //  播放状态
			// 	qApp.getViewMode() == viewmode_setting))
			// {
			// 	m_Mutex.Exit();
			// 	SetOpType(caroptype_close_force);
			// 	return;
			// }
		}
		else {
			if (m_iScreenOnType != screenontype_leddouble &&
				m_iScreenOnType != screenontype_led &&
				m_iScreenOnType != screenontype_taps &&
				m_iScreenOnType != screenontype_remote) {
				m_iScreenOnType = screenontype_normal;
			}
			m_uiTimeStampLastOp = uiCurTime;
		}
	}
	m_Mutex.Exit();
}

bool CCarStateMonitor::IsOpenKey(unsigned char key)
{
#if 0
	const unsigned char openKeys[] = {
		0x00, 0x02, 0x03, 0x04, 0x05, 	//开360、开前视、开后视、开左视、开右视
		0x06, 0x07, 0x08, 0x09, 		//打开窄道、打开路崖、前流媒体、后流媒体
		0x0A, 0x0B, 0x0C, 0x0D, 		//2D、3D模式、左3D、右3D
		0x0E, 0x0F, 0x1B, 				//前视放大、后视放大、3D环绕
		0xA3, 0xA5, 0xA7,				//环视一周（满屏车模半透明）、氛围灯、香氛
	};
#else
	const unsigned char openKeys[] = { 0x00 };
#endif

	for (int i = 0; i < sizeof(openKeys) / sizeof(openKeys[0]); ++i)
	{
		if (key == openKeys[i])
			return true;
	}
	return false;
}

bool CCarStateMonitor::IsSteeringTrigger(CARRUNDATA curData, CARRUNDATA lastData, TriggerStatus status)
{
	if (!m_bSpeedSteerEn)
		return false;

	auto SteeringTrigger = [&](int steerSpeed, float angleIn, int speedIn)->bool {

		if (speedIn > steerSpeed && (angleIn > m_fSteerAngle || angleIn < -m_fSteerAngle))
			return true;

		return false;
	};

	if (status == TriggerStatusHigh)
		return SteeringTrigger(m_iSpeedSteer, curData.fAngle, curData.iSpeed);
	else if (status == TriggerStatusLow)
		return !SteeringTrigger(m_iSpeedSteer, curData.fAngle, curData.iSpeed);
	else if (status == TriggerStatusRising)
		return (!SteeringTrigger(m_iSpeedSteer, lastData.fAngle, lastData.iSpeed) && SteeringTrigger(m_iSpeedSteer, curData.fAngle, curData.iSpeed));
	else if (status == TriggerStatusFalling)
		return (SteeringTrigger(m_iSpeedSteer, lastData.fAngle, lastData.iSpeed) && !SteeringTrigger(m_iSpeedSteer, curData.fAngle, curData.iSpeed));

	return false;
}

bool CCarStateMonitor::IsLowSpeedTrigger(CARRUNDATA curData, CARRUNDATA lastData, TriggerStatus status)
{
	if (m_iLowSpeed == 0)
		return false;

	if (status == TriggerStatusHigh)
		return curData.iSpeed < m_iLowSpeed;
	else if (status == TriggerStatusLow)
		return !(curData.iSpeed < m_iLowSpeed);
	else if (status == TriggerStatusRising)
	{
		if (lastData.iSpeed == 0 && curData.iSpeed != 0 && curData.iSpeed < m_iLowSpeed)
			return true;

		if (!(lastData.iSpeed < m_iLowSpeed) && (curData.iSpeed < m_iLowSpeed))
			return true;
	}
	else if (status == TriggerStatusFalling)
		return (lastData.iSpeed < m_iLowSpeed) && !(curData.iSpeed < m_iLowSpeed);

	return false;
}

int CCarStateMonitor::IsScreenOn()
{
	CMutexAuto mutexAuto(&m_Mutex);
	return m_iScreenOnOff;
}

void CCarStateMonitor::RoutinueMain()
{
	static screenontype subTypeBootLast = screenontype_end;

	m_Sem.Pend(50);

	int iRt;
	int iSyncCarDataFlag = 0;

	CARRUNDATA carDataOld;
	CARRUNDATA carDataTmpNewest;

	CMutexAuto mutexAuto(&m_Mutex);

	carDataOld = m_CarDataCur;
	if (m_iCarDataFlag) {
		m_iCarDataFlag = 0;
		iSyncCarDataFlag = 1;
		m_CarDataCur = m_CarDataNewest;
		carDataTmpNewest = m_CarDataNewest;
		// 有双闪开关切换时才入队列，避免m_DoubleLedSeq.Clear()后，此处又将上一次状态重新入队列。
		if (((carDataTmpNewest.LedState == ledstate_leftright) && (carDataOld.LedState != ledstate_leftright))
			|| ((carDataTmpNewest.LedState != ledstate_leftright) && (carDataOld.LedState == ledstate_leftright))) {
			printf("m_DoubleLedSeq.Push\n");
			m_DoubleLedSeq.Push(carDataTmpNewest.LedState == ledstate_leftright ? 1 : 0);
		}
	}
	else {
		carDataTmpNewest = m_CarDataCur;
	}

	unsigned int uiTimeStampCur = GetTickCount();
	INIParkMonitor udtPark;
	GCSetings.GetParkMonitor(udtPark);

	if (subTypeBootLast != m_iScreenOnType) {
		printf("m_iScreenOnType from  %d, to %d, m_iSpeedThres=%d\n", subTypeBootLast, m_iScreenOnType, m_iSpeedThres);
		subTypeBootLast = m_iScreenOnType;
	}

	// 当前处停车监控状态
	if (m_CarState == carstate_park_monitor) {
		if (udtPark.power == pm_power_core) {
			long long parkMonitorTime = GetTickCount() - m_ACCOffTime;
			bool voltageIsLower = false;
			// 当前电压无效(低于3.3V认为无效)或不低于阈值时，清除记录
			if ((m_iVoltage < 3.3f * 10) || (m_iVoltage >= udtPark.volt * 10)) {
				m_tLastVoltageLower = 0;
			}
			else {
				if (m_tLastVoltageLower > 0) {
					// 前期已经低于阈值，则检测是否超过一定时间
					int voltageIsLowerTime = GetTickCount() - m_tLastVoltageLower;
					if (voltageIsLowerTime > 30000) {
						voltageIsLower = true;
						//printf("found voltageIsLowerTime > 30!\n");
					}
				}
				else {
					// 此时开始低于阈值，则记录当前时间
					//printf("m_iVoltage < parkMonitor.iVolt start!");
					m_tLastVoltageLower = GetTickCount();
				}
			}

			// 1. 核心板电源管理时，停车监控时间已到或电压低于阈值则关机
			if ((parkMonitorTime / 1000) > udtPark.hours * 60 * 60 || voltageIsLower == true) {
				printf("park monitor time=%lld(s), setting time=%d(s), voltageIsLower is %s\n",
					parkMonitorTime / 1000, udtPark.hours * 60 * 60, voltageIsLower ? "true" : "false");
				if (voltageIsLower) {
					printf("current voltage is %f v(< 3.3f is invalid)\n", m_iVoltage / 10.0f);
				}
				printf("park monitor is finished, let's poweroff!\n");
#ifndef WIN32
				PowerOffRequest();
#endif
				Sleep(20);
				return;

			}
			else {
				if (GetTickCount() - m_parkMonitorNotifyTime >= 3000) {
					NotifyParkMonitorState();
					m_parkMonitorNotifyTime = GetTickCount();
				}
			}

		}
		else {
			// 2. 停车监控电源管理由解码板提供时, 停车监控时间已到则关机
			long long parkMonitorTime = GetTickCount() - m_ACCOffTime;
			if ((parkMonitorTime / 1000) > udtPark.hours * 60 * 60) {
				printf("park monitor time=%lld(s), setting time=%d(s)\n", parkMonitorTime / 1000, udtPark.hours * 60 * 60);
				printf("park monitor is finished, let's poweroff!\n");
#ifndef WIN32
				PowerOffRequest();
#endif
				Sleep(20);
				return;
			}
			else {
				if (GetTickCount() - m_parkMonitorNotifyTime >= 3000) {
					NotifyParkMonitorState();
					m_parkMonitorNotifyTime = GetTickCount();
				}
			}
		}

		if (carDataTmpNewest.iAcc) {	// 停车监控还未结束，ACC又从OFF切换到ON，则关闭本次停车监控。
			// qMainFrame.ClearEvent();
			if (udtPark.enable != pm_on_always) {
				udtPark.enable = pm_off;
			}
			m_CarState = carstate_boot;
			GCSetings.SetParkMonitor(udtPark);
#ifndef WIN32
			if (GCSetings.GetRecord()) {
				// ParkMonitorSetFrameRate(-1);
				if (!DiskManage::IsMount()) {
					DiskManage::DoMount(NULL);
				}
				// if (asGet(asAppMode) != APP_MODE_PLAY) {
				// 	RestartVideoRecord();
				// }
			}
			else {
				// vrEnableRecord(0);
			}
#endif
		}
		else {
#ifndef WIN32
			if (GCSetings.GetRecord() &&
				DiskManage::IsMount() &&
				udtPark.enable != pm_off) {
				Sleep(40);
			}
			else {
				printf("Record()=%d or IsMount()=%d or pm.enable=%d in park monitor mode, so let's power off\n",
					GCSetings.GetRecord(), DiskManage::IsMount(), udtPark.enable);
#ifndef WIN32
				PowerOffRequest();
#endif
				Sleep(20);
				return;
			}
#endif
		}
	}
	// int iAppMode = asGet(asAppMode);

	if (m_iScreenOnOff == 0) {
		INILogic udtLogic;
		GCSetings.GetLogic(udtLogic);


		// if (SystemIsUpating()) {
		// 	m_iScreenOnType = screenontype_normal;
		// 	m_subTypeBoot = subtype_boot_front;
		// 	OnOff(1);
		// 	return;
		// }

		if (carDataTmpNewest.iCAN == CAN_SUSPENED) {
			m_subTypeBoot = subtype_boot_rot_wait;
			if (m_tLastCANOffTime == 0) {
				// 记录 CAN 由 ON 到 OFF的时间
				m_tLastCANOffTime = GetTickCount();
			}
			else {
				// CAN休眠长达1秒
				/* 对于核心板，未匹配到正确波特率时或者匹配的是11520波特率但可能底板处于复位升级状态（MCU不发送CAN状态），会导致CAN是休眠状态，此时不应该立即关机，
				 * 而改为之前的超过20秒才关机的方式，以便于等待波特率匹配或正确地检测到底板升级状态
				 */
				bool wait = (!McuPortConnected()) || (McuPortConnected() && (GCSetings.GetMucPortBaudRate() == 115200));
				if (GetTickCount() - m_tLastCANOffTime > 1000 + (wait ? 19000 : 0)) {
					if (udtPark.power == pm_power_core) {
						// 1. 停车监控电源管理由核心板提供时,未启动停车监控时，则立即关机
						if (!((udtPark.used == pm_func_open) && udtPark.enable)) {
							printf("CAN OFF 1s later, not enable parkmonitor, let's poweroff!\n");
							PowerOffRequest();
							Sleep(20);
						}
						else {
							if (m_CarState != carstate_park_monitor) {
								// 启动停车监控
								printf("ACC OFF 1s later, let's start park monitor!\n");
								m_CarState = carstate_park_monitor;
								m_tLastVoltageLower = 0;
#ifndef WIN32
								// ParkMonitorSetFrameRate(udtPark.fps);
								RestartVideoRecord();
								// unsigned char ledStatus = LED_FLASH_ON1S_OFF2S;
								// RedLedCtrlQueue.PostMsg((void*)&ledStatus);
#endif
							}
						}
					}
					else {
						// 2. 停车监控电源管理由解码板提供时, 立即关机
						printf("CAN OFF 1s later, let's poweroff!\n");
						PowerOffRequest();
						Sleep(20);
					}
				}
			}
			return;
		}
		else {
			m_tLastCANOffTime = 0;
		}

		if (carDataTmpNewest.iAcc == 0) {

			if (m_ACCOffTime == 0) {
				// 记录 ACC 由 ON 到 OFF 的时间
				m_ACCOffTime = GetTickCount();
			}
			else {
				m_subTypeBoot = subtype_boot_rot_wait;
				// 1. ACC 关闭1秒以上，如果开启了停车，则进入停车监控
				if (GetTickCount() - m_ACCOffTime > 1000) {
					if ((udtPark.used == pm_func_open) && udtPark.enable) {
						if (m_CarState != carstate_park_monitor) {
							// 启动停车监控
							printf("ACC OFF 1s later, let's start park monitor!\n");
							m_CarState = carstate_park_monitor;
							m_tLastVoltageLower = 0;
#ifndef WIN32
							// ParkMonitorSetFrameRate(udtPark.fps);
							RestartVideoRecord();
							// unsigned char ledStatus = LED_FLASH_ON1S_OFF2S;
							// RedLedCtrlQueue.PostMsg((void*)&ledStatus);
#endif
						}
					}
					else {
#ifndef WIN32
						// vrEnableRecord(0);		// ACC OFF 且未开启停车监控时，关闭录像
#endif
					}
				}
			}
			Sleep(5);
			return;
		}
		else {
			m_ACCOffTime = 0;
		}

		if (m_ForcedExit) {
			bool recover = false;
			if (/*SystemIsUpating() ||*/
				(carDataOld.iAcc == 0 && carDataTmpNewest.iAcc != 0) ||
				(carDataOld.Tappos != tappos_D && carDataTmpNewest.Tappos == tappos_D) ||
				(carDataOld.Tappos != tappos_R && carDataTmpNewest.Tappos == tappos_R) ||
				(carDataOld.iDoorState == 0 && carDataTmpNewest.iDoorState != 0) ||
				(carDataOld.iPKey == 0 && carDataTmpNewest.iPKey != 0) ||
			// 	(iAppMode == APP_MODE_CALIB) ||
			// 	(iAppMode == APP_MODE_PLAY) ||
				(m_bLedDoubleEn && m_DoubleLedSeq.GetChangeCnt(m_iDoubleDelayTime) >= 2) ||
				(checkRadar(carDataTmpNewest, carDataOld)) /*||
				IsSteeringTrigger(carDataTmpNewest, carDataOld, TriggerStatusRising) ||
				IsLowSpeedTrigger(carDataTmpNewest, carDataOld, TriggerStatusRising)*/) {
				recover = true;
			}

			if (carDataOld.LedState != ledstate_lefton &&
				carDataTmpNewest.LedState == ledstate_lefton &&
				udtLogic.left != bvlogic_none) {
				recover = true;
			}
			if (carDataOld.LedState != ledstate_righton &&
				carDataTmpNewest.LedState == ledstate_righton &&
				udtLogic.right != bvlogic_none) {
				recover = true;
			}

			if (recover == false) {
				return;
			}
			m_ForcedExit = false;
		}

		// 自动泊车模式下，应该保持原车画面
		if (carDataTmpNewest.iAutoPark == 1) {
			return;
		}

		// if (turnRightOpsAction(carDataTmpNewest, iAppMode, m_iScreenOnOff)) {
		// 	return;
		// }

		//  360等待旋转，且车机开机Log显示完成，则开始旋转并开屏
		if ((m_subTypeBoot == subtype_boot_rot_wait) && (carDataTmpNewest.iStartTime > m_uiCarStartTime)) {
			Start();
			return;
		}

		//  刚上电时处理. 事实上只要ACC OFF了，底板必然会对核心板断电，因此 ACC ON 的时候都是重新上电，因此实际上不会进入此状态
		if (carDataOld.iAcc == 0 && carDataTmpNewest.iAcc != 0) {
			//printf("<WARNING> %s, %d: Why at here???\r\n", __FUNCTION__, __LINE__);
			printf("ACC from OFF to ON, m_uiCarStartTime=%d\n", m_uiCarStartTime);
			//  标记为等待旋转状态。
			m_subTypeBoot = subtype_boot_rot_wait;
			return;
		}

		if (carDataTmpNewest.iPKey == 1) {
			m_iScreenOnType = screenontype_pkey;
			m_subTypeBoot = subtype_boot_front;
			OnOff(1);
			return;
		}

		//  先判断是否连续双闪
		/*
		1、m_bLedDoubleEn == true
		2、3s内按下双闪两次*/
		if (m_bLedDoubleEn && m_DoubleLedSeq.GetChangeCnt(m_iDoubleDelayTime) >= 2) {                   //  3 秒内，2次双闪
			m_DoubleLedSeq.Clear();// 清空双闪状态序列，避免本次双击事件被重复处理

			m_subTypeBoot = subtype_boot_front; // 将启动子状态设置为“前视模式”。表示不需要旋转车模动画，
			OnOff(1);
			printf("ScreenOn: screenontype_leddouble\n");
			/*
			设置当前屏幕开启的类型为“双闪触发类型”（screenontype_leddouble）。
			这个类型会影响后续关屏逻辑：
			- 通常双闪触发的屏幕会使用一个较长的空闲时间，
			  除非用户再次双击双闪或执行其他强制关闭操作。*/
			m_iScreenOnType = screenontype_leddouble;
			return;
		}

		//  当处于标定时，一直保持开屏. 设置 和 播放界面是否也要一直保持开屏 ??
		// if (SystemIsUpating() ||      // 正在升级
		// 	iAppMode == APP_MODE_CALIB ||      //  标定状态
		// 	iAppMode == APP_MODE_PLAY ||      //  播放状态
		// 	 //  设置界面
		// 	(qApp.getViewMode() == viewmode_setting && m_CarDataNewest.iSpeed <= m_iSpeedThres)) {

		// 	m_iScreenOnType = screenontype_normal;
		// 	m_subTypeBoot = subtype_boot_front;
		// 	OnOff(1);
		// 	return;
		// }
		if (carDataTmpNewest.Tappos == tappos_R) {// 判断 R 档
			m_subTypeBoot = subtype_boot_front;		// 前视状态
			OnOff(1);// 1 表示开启屏幕
			printf("ScreenOn: screenontype_taps\n");
			m_iScreenOnType = screenontype_taps;	// 开屏类型，倒车触发
			return;
		}

		//  开门触发
		if (iSyncCarDataFlag && carDataTmpNewest.iDoorState && carDataTmpNewest.iDoorState != carDataOld.iDoorState) {
			m_subTypeBoot = subtype_boot_front;
			OnOff(1);
			printf("ScreenOn: screenontype_door\n");
			m_iScreenOnType = screenontype_door;
			m_uiTimeStampLastOp = uiTimeStampCur;
			return;
		}

		//  非倒档，且速度大于限值
		if (carDataTmpNewest.Tappos != tappos_R && carDataTmpNewest.iSpeed >= m_iSpeedThres * 0.98) {  //  滞回比较, 2% 的滞回量
			//printf("Screen Not On: speed too fastly\n");
			return;
		}

		//  D 档或 N 档
		/*  在360启动之前挂入D则满足“carDataTmpNewest.Tappos == tappos_D && carDataOld.Tappos”，
		 *  从而使得m_iScreenOnType由subtype_boot_rot360变为screenontype_taps，即不会再执行“旋转完成切换到前视”。
		 *  为了避免这种情况，将初始档位设置为tappos_unkown，并添加carDataOld.Tappos != tappos_unkown条件限制。
		 */
		/*
		判断是否为“挂入D档”
		条件1：当前档位是 D 档
		条件2：上一次记录的档位不是 D 档（即从其他档位切换到 D 档）
		条件3：上一次记录的档位也不是“未知档位”（tappos_unkown）
		这样做的目的是：避免系统刚启动时，初始档位为 unknown 而误触发开屏。
		只有档位真正从某个确定的有效档位（如 P、N、R）切换到 D 档时，才认为是有效触发。			*/
		if (carDataTmpNewest.Tappos == tappos_D && carDataOld.Tappos != tappos_D && carDataOld.Tappos != tappos_unkown) {
			m_subTypeBoot = subtype_boot_front;	// 前视状态
			OnOff(1);
			printf("ScreenOn: screenontype_taps\n");
			m_iScreenOnType = screenontype_taps;	// 开屏类型，档位触发
			/*    
			更新“最后一次用户/系统操作的时间戳”为当前时间。
			在屏幕已经打开的情况下，只要处于 R 档，就不断刷新这个时间戳。
			这样做的目的是：避免因“空闲超时”而自动关闭屏幕。
			因为倒车过程中必须持续显示画面，不能因为几秒钟无操作就关屏。*/
			m_uiTimeStampLastOp = uiTimeStampCur;
			return;
		}

		/*
		低速触发开屏
		1、开机或开机旋转车模 2、车速不为0 
		条件3：调用 IsLowSpeedTrigger 函数检测是否处于“低速状态”的上升沿或持续高电平。
          参数 TriggerStatusHigh 表示检测“当前是否处于低速状态”（高于阈值则false，低于阈值则true）。
          该函数内部会根据配置的低速阈值（m_iLowSpeed，例如10 km/h）判断：
          - 若当前车速 < 阈值，返回 true（表示处于低速区域）
          - 否则返回 false
		*/
		if ((m_iScreenOnType != screenontype_boot || m_subTypeBoot != subtype_boot_rot360)
			&& carDataTmpNewest.iSpeed != 0
			&& IsLowSpeedTrigger(carDataTmpNewest, carDataOld, TriggerStatusHigh)) {
			OnOff(1);	// 满足以上条件开屏
			printf("ScreenOn: screenontype_low_speed\n");
			m_iScreenOnType = screenontype_low_speed;
			m_uiTimeStampLastOp = uiTimeStampCur;
			return;
		}

		//  雷达触发
		if (carDataTmpNewest.Tappos == tappos_D && carDataTmpNewest.iSpeed < m_iSpeedThres) {
			if (checkRadar(m_CarDataNewest, carDataOld) &&
				(!(m_iScreenOnType == screenontype_boot && m_subTypeBoot == subtype_boot_rot360))) {
				m_subTypeBoot = subtype_boot_front;
				OnOff(1);
				printf("ScreenOn: screenontype_radar\n");
				m_iScreenOnType = screenontype_radar;
				m_uiTimeStampLastOp = uiTimeStampCur;
				return;
			}
		}

		//  速度降下来，并且打了转向灯
		if ((carDataTmpNewest.LedState == ledstate_lefton && udtLogic.left != bvlogic_none) ||
			(carDataTmpNewest.LedState == ledstate_righton && udtLogic.right != bvlogic_none)) {
			if (carDataTmpNewest.iSpeed < m_iSpeedThres * 0.98)
			{
				m_subTypeBoot = subtype_boot_front;
				OnOff(1);
				printf("ScreenOn: screenontype_led\n");
				m_iScreenOnType = screenontype_led;
				return;
			}
		}

		if (IsSteeringTrigger(carDataTmpNewest, carDataOld, TriggerStatusHigh) &&
			(carDataTmpNewest.iSpeed < m_iSpeedThres * 0.98)) {
			m_subTypeBoot = subtype_boot_front;
			OnOff(1);
			printf("ScreenOn: screenontype_steering\n");
			m_iScreenOnType = screenontype_steering;
			return;
		}
	}
	else {
		/*
		 *  当前是打开的
		 */
		if (m_ForcedExit) {
			OnOff(0);
			return;
		}
		// if (SystemIsUpating()) {
		// 	return;
		// }
		if (udtPark.power == pm_power_core) {	// 核心板电源管理依然按原来的方式控制
			if (carDataTmpNewest.iCAN == CAN_SUSPENED) {
				OnOff(0);
				return;
			}
			m_tLastCANOffTime = 0;
		}
		else {
			if (carDataTmpNewest.iAcc == 0) {
				OnOff(0);
			}
			else {
				m_ACCOffTime = 0;
			}

			if (carDataTmpNewest.iCAN == CAN_SUSPENED) {
				OnOff(0);
				return;
			}
			else {
				m_tLastCANOffTime = 0;
			}
		}

		if (carDataTmpNewest.iAcc == 0) {
			printf("ScreenOff(Current status=ON): iAcc == 0, m_uiCarStartTime=%d\n", m_uiCarStartTime);
			OnOff(0);
			if (carDataOld.iAcc) {

				/*
					*  处理关机相关事宜 ( 这部分工作是在 threadUart()中做，还是此处做? )：
					*  1、若录像已开启, 则先停止录像，并等待确保停止成功
					*  2、若已插入U盘，则 umount U盘
					*  3、通知底板准备关机（ 底板收到通知后，延时一定时间再断核心板电 ）
					*  4、shut down
					*/

			}
			return;
		}

		// 自动泊车模式下，应该切原车画面
		if (carDataTmpNewest.iAutoPark == 1) {
			//printf("Auto Parking!\n");
			OnOff(0);
			return;
		}

		// if (turnRightOpsAction(carDataTmpNewest, iAppMode, m_iScreenOnOff)) {
		// 	return;
		// }

		if (m_iScreenOnType == screenontype_pkey) {
			if (carDataTmpNewest.iPKey == 1) {
				return;
			}
			else {
				TryToOtherOpenState(m_CarDataNewest, carDataOld);
			}
		}

		// 避免在设置界面超过关屏时速时，用双闪开屏后又会被设置界面超过关屏时速时关屏的问题，于是将双闪移动到前面
		if (m_iScreenOnType == screenontype_leddouble) {
			if (m_bLedDoubleEn && m_DoubleLedSeq.GetChangeCnt(m_iDoubleDelayTime) >= 2) {
				m_DoubleLedSeq.Clear();
				printf("OFF: screenontype_leddouble\r\n");
				OnOff(0);
				m_iScreenOnType = screenontype_normal;
				m_ForcedExit = true;
				if (checkRadar(m_CarDataNewest, m_CarDataCur)) {
					m_RadarPause = true;
				}
			}
			return;                                     //  双闪打开时，只有双闪才能关闭
		}
		else {
			// 不是双闪打开时，也能用双闪关闭
			if (m_bLedDoubleEn && m_DoubleLedSeq.GetChangeCnt(m_iDoubleDelayTime) >= 2) {
				m_DoubleLedSeq.Clear();
				printf("OFF: screenontype_leddouble\r\n");
				OnOff(0);
				m_iScreenOnType = screenontype_normal;
				m_ForcedExit = true;
				if (checkRadar(m_CarDataNewest, m_CarDataCur)) {
					m_RadarPause = true;
				}
				return;
			}
		}

		//  当处于标定时，一直保持开屏. 设置 和 播放界面是否也要一直保持开屏 ??
		// if (SystemIsUpating() ||      // 正在升级
		// 	iAppMode == APP_MODE_CALIB ||      //  标定状态
		// 	iAppMode == APP_MODE_PLAY ||      //  播放状态
		// 	 //  设置界面
		// 	(qApp.getViewMode() == viewmode_setting)) {

		// 	// 设置界面不关屏，避免此处关屏后，若有其它开屏条件时（如R档），导致反复开、关切换
		// 	m_uiTimeStampLastOp = uiTimeStampCur;

		// 	return;
		// }

		if (carDataTmpNewest.Tappos != tappos_P && carDataTmpNewest.Tappos != carDataOld.Tappos && m_iScreenOnType != screenontype_boot) {
			//  非 P 档，并且档位在变化，排除在启动旋转车模时从P(=0)到D且中间不发R档信息
			m_iScreenOnType = screenontype_taps;
			m_uiTimeStampLastOp = uiTimeStampCur;
			//OnOff(1);                                   //  即便现在是开的，也通知一次底板开屏，避免由于中间某种异常情况导致关屏后不再开启
			return;
		}

		if (carDataTmpNewest.Tappos == tappos_R) {      //  R 档时一直开启
			m_subTypeBoot = subtype_boot_front;			// 还是前视
			/*    
			更新“最后一次用户/系统操作的时间戳”为当前时间。
			在屏幕已经打开的情况下，只要处于 R 档，就不断刷新这个时间戳。
			这样做的目的是：避免因“空闲超时”而自动关闭屏幕。
			因为倒车过程中必须持续显示画面，不能因为几秒钟无操作就关屏。*/
			m_uiTimeStampLastOp = uiTimeStampCur;
			return;
		}

		if (m_iScreenOnType == screenontype_door) {
			if (iSyncCarDataFlag && carDataTmpNewest.iDoorState && carDataTmpNewest.iDoorState != carDataOld.iDoorState) {
				m_uiTimeStampLastOp = uiTimeStampCur;
				return;
			}
			if (m_uiTimeStampLastOp + m_uiIdleTime[screenontype_door] < uiTimeStampCur) { //  开门后一段时间内没有其它门变化，也关闭
				TryToOtherOpenState(m_CarDataNewest, carDataOld);
			}
			return;         //  开门时强制提醒一段时间
		}

		//  不是 R 档 且 速度高于阈值，强制关屏
		if (m_CarDataNewest.Tappos != tappos_R && m_CarDataNewest.iSpeed > m_iSpeedThres) {
			printf("OFF: Speed > %d\r\n", m_iSpeedThres);
			OnOff(0);
			return;
		}

		//  条件1：当前最新的档位是 P 档，条件2：上一次记录的档位不是 P 档
		if (m_CarDataNewest.Tappos == tappos_P && carDataOld.Tappos != tappos_P) {
			/*
			传入当前最新车身数据（m_CarDataNewest）和上一次的数据（carDataOld） 根据条件决定 仍然开屏 / 关屏 */
			TryToOtherOpenState(m_CarDataNewest, carDataOld);
			return;
		}

		// 切档后，几秒内没有操作的话关闭
		// 是档位触发 且 当前不是R档
		if (m_iScreenOnType == screenontype_taps && m_CarDataNewest.Tappos != tappos_R) {
			if (m_CarDataNewest.Tappos != carDataOld.Tappos) {      //  档位有变化
				m_uiTimeStampLastOp = uiTimeStampCur;	// 档位有变化更新时间戳，避免刚换挡就因空闲超时而关屏。
			}
			if ((carDataOld.LedState == ledstate_lefton || carDataOld.LedState == ledstate_righton) &&/* 检测转向灯状态的变化（左/右转灯亮 -> 左右转向灯都熄灭） */
				(carDataTmpNewest.LedState != ledstate_lefton && carDataTmpNewest.LedState != ledstate_righton)) {
				m_uiTimeStampLastOp = GetTickCount();	// 档位有变化更新时间戳，避免刚换挡就因空闲超时而关屏。
				printf("m_iScreenOnType = screenontype_taps, update last op time by LED (line %d）.\r\n", __LINE__);
			}
			if (m_uiTimeStampLastOp + m_uiIdleTime[screenontype_taps] < uiTimeStampCur) {
				//printf("+screenontype_taps, + 0 TryToOtherOpenState.\r\n");
				TryToOtherOpenState(m_CarDataNewest, carDataOld);
				//Sleep(100);
				//printf("+screenontype_taps, - 0 TryToOtherOpenState.\r\n");
				return;
			}
		}

		if (m_iScreenOnType == screenontype_low_speed)
		{
			if (carDataTmpNewest.iSpeed == 0 && (m_uiTimeStampLastOp + m_uiIdleTime[screenontype_low_speed] < uiTimeStampCur))
			{
				printf("OFF: screenontype_low_speed\r\n");
				TryToOtherOpenState(m_CarDataNewest, carDataOld);
				return;
			}

			if (!IsLowSpeedTrigger(carDataTmpNewest, carDataOld, TriggerStatusHigh))
			{
				TryToOtherOpenState(m_CarDataNewest, carDataOld);
				return;
			}
		}

		if (m_iScreenOnType == screenontype_boot) {

			INILogic udtLogic;
			GCSetings.GetLogic(udtLogic);
			if ((m_CarDataNewest.LedState == ledstate_lefton && udtLogic.left != bvlogic_none) ||
				m_CarDataNewest.LedState == ledstate_righton && udtLogic.right != bvlogic_none) {
				m_iScreenOnType = screenontype_led;
				m_subTypeBoot = subtype_boot_rot360_endstay;
				return;
			}

			if (carDataTmpNewest.iDoorState && (!(m_iScreenOnType == screenontype_boot && m_subTypeBoot == subtype_boot_rot360))) {
				m_iScreenOnType = screenontype_door;
				m_subTypeBoot = subtype_boot_rot360_endstay;
				return;
			}

			switch (m_subTypeBoot)
			{
			case subtype_boot_rot360:
				int iIsRotViewId;
				int iIsAutoRot;
				int iIsRotEnd;
				float fAngleCur;
				iIsRotViewId = 0;
				iIsRotEnd = 0;
				iRt = 0; //viewGetRotState(&iIsRotViewId, &iIsAutoRot, &iIsRotEnd, &fAngleCur);
				if (iRt >= 0) {
					if (iIsRotViewId && iIsRotEnd) {
						m_subTypeBoot = subtype_boot_rot360_endstay;
						m_uiTimeStampLastOp = uiTimeStampCur;
					}
#if 1
					// 此条件不会满足，因为如果iIsRotViewId != 0，viewGetRotState返回-1。
					if (!iIsRotViewId) {
						// 实测转车模时，某一时刻可能会满足此条件，导致旋转中途关屏，于是此处对单次进行过滤，超过5次才更新状态
						m_AbnormalCnt++;
						if ((m_AbnormalCnt % 5) == 0)
						{
							m_subTypeBoot = subtype_boot_front;
							m_uiTimeStampLastOp = uiTimeStampCur;
						}
					}
#endif
				}
				else {
					// 如果当前不在旋转车模视图（在开机旋转时可能会被其它操作中断，如倒档、开门等），则viewGetRotState会返回-1，此处应次m_subTypeBoot置为非subtype_boot_rot360状态，否则会出现不关屏的问题
					m_subTypeBoot = subtype_boot_front;
					m_uiTimeStampLastOp = uiTimeStampCur;
				}
				break;
			case subtype_boot_rot360_endstay:
				if (m_uiTimeStampLastOp + 3000 < uiTimeStampCur) {

					if (carDataTmpNewest.LedState == ledstate_leftright && GCSetings.GetLedDoubleSwitchState()) {
						// qMainFrame.SetFunction(car_led_doubleflash);
					}
					else {
						// qMainFrame.SetFunction(car_taps_D);
					}

					m_uiTimeStampLastOp = uiTimeStampCur;
					m_subTypeBoot = subtype_boot_front;
				}
				break;
			case subtype_boot_front:
				if (m_uiTimeStampLastOp + m_uiIdleTime[screenontype_boot] < uiTimeStampCur) {
					TryToOtherOpenState(carDataTmpNewest, carDataOld);
					return;
				}
				break;
			default:
				printf("%s, %d: state seq bad??? state seq = %d\r\n", __FUNCTION__, __LINE__, m_subTypeBoot);
				break;
			}
		}

		if (m_iScreenOnType == screenontype_radar) {
			if (checkRadar(m_CarDataNewest, carDataOld) && GCSetings.GetRadarState()) {
				return;
			}
			if (m_uiTimeStampLastOp + m_uiIdleTime[screenontype_radar] < uiTimeStampCur) {
				TryToOtherOpenState(carDataTmpNewest, carDataOld);
				return;
			}
		}

		if (m_iScreenOnType == screenontype_led) {
			//  灯已消失
			if ((carDataOld.LedState == ledstate_lefton || carDataOld.LedState == ledstate_righton) &&
				(m_CarDataNewest.LedState != ledstate_lefton && m_CarDataNewest.LedState != ledstate_righton)) {
				m_uiTimeStampLastOp = uiTimeStampCur;
			}
			else if (m_CarDataNewest.LedState != ledstate_lefton && m_CarDataNewest.LedState != ledstate_righton) {
				if (m_uiTimeStampLastOp + m_uiIdleTime[screenontype_led] < uiTimeStampCur) {
					TryToOtherOpenState(carDataTmpNewest, carDataOld);
				}
				return;
			}
		}
		if (m_iScreenOnType == screenontype_steering) {
			if (IsSteeringTrigger(carDataTmpNewest, carDataOld, TriggerStatusFalling)) {
				m_uiTimeStampLastOp = uiTimeStampCur;
			}
			else if (!IsSteeringTrigger(carDataTmpNewest, carDataOld, TriggerStatusHigh)) {
				if (m_uiTimeStampLastOp + m_uiIdleTime[screenontype_steering] < uiTimeStampCur) {
					TryToOtherOpenState(carDataTmpNewest, carDataOld);
				}
				return;
			}
		}

		if (m_iScreenOnType == screenontype_remote) {
			if (m_uiTimeStampLastOp + m_uiIdleTime[screenontype_remote] < uiTimeStampCur) {
				TryToOtherOpenState(carDataTmpNewest, carDataOld);
			}
			return;
		}

		//  普通模式下
		if (m_iScreenOnType == screenontype_normal) {
			if (m_uiTimeStampLastOp + m_uiIdleTime[screenontype_normal] < uiTimeStampCur) {
				TryToOtherOpenState(carDataTmpNewest, carDataOld);
				return;
			}
		}
	}

}

void CCarStateMonitor::RoutineOnOff()
{
	ONOFF_EVT evt;

	int iRt = m_QueueOnOff.RecvNewestMsgAndClear(&evt);
	if (iRt < 0)
		return;

	if (evt.iIsOn) {
		// asSet(asDisplay);
		if (m_pfunOnOff) {
			if (m_iScreenOnOff == 0) {
				// viewFlushRenderTempState();
				Sleep(2);
			}
			m_pfunOnOff(1);
		}
	}
	else {

		if (m_pfunOnOff) {
			m_pfunOnOff(0);
		}

		// 当时速小关屏阀值时，不关屏，避免频繁开关屏而可能显示“屏保中”的界面；关屏操作交由RoutinueMain中判断速度大于阀值时强制关屏。
		//if (m_CarDataNewest.iSpeed < m_iSpeedThres * 0.98 && m_CarDataCur.iAcc == 1)
		//return ;
		// asClearDisplayDelay(100000);

	}
	m_iScreenOnOff = !!evt.iIsOn;
	if (m_CloseAndQuit) {
		// FeedDog();
		exit(0);
	}
}

bool CCarStateMonitor::AccOn()
{
	CMutexAuto mutexAuto(&m_Mutex);
	return m_CarDataNewest.iAcc;
}

bool CCarStateMonitor::IsStartedParkMonitor()
{
	CMutexAuto mutexAuto(&m_Mutex);
	return (m_CarState == carstate_park_monitor);
}

void CCarStateMonitor::SetRearSwitch360(bool en)
{
	m_bRearSwitch360 = en;
}

void CCarStateMonitor::SetDoubleDelay(bool en)
{
	m_DoubleLedSeq.setEnable(en);
}

void CCarStateMonitor::SetDoubleDelayTime(int val)
{
	m_iDoubleDelayTime = val;
}

void CCarStateMonitor::SetLowSpeed(int iSpeed)
{
	m_iLowSpeed = iSpeed;
}

void CCarStateMonitor::SetSteerAngle(int val)
{
	//-35 ~ 35,打满左边是一圈半,一圈半是360°+180°
	if (val == 0)
		m_bSpeedSteerEn = false;
	else
	{
		m_bSpeedSteerEn = true;
		m_fSteerAngle = val * 3.0f / 180.0f * 3.14159f;
	}
}

void CCarStateMonitor::OnOff(int iIsOn, int iOnOffDlyMs /*= -1 /* 关屏时，延时多长时间打开 */)
{
	//printf("OnOff Next = %d, Cur screen type: %d\r\n", iIsOn, m_iScreenOnType);
	ONOFF_EVT evt;
	evt.iIsOn = (m_CloseAndQuit ? 0 : iIsOn);
	evt.uiTimeStamp = GetTickCount();
	evt.uiTimeDlyTime = iOnOffDlyMs < 0 ? (iIsOn ? 0 : 1500) : iOnOffDlyMs;
	m_QueueOnOff.PostMsg(&evt);
}

int CCarStateMonitor::checkRadar(CARRUNDATA newest, CARRUNDATA old)
{
	if (m_RadarPause) {
		return 0;
	}

	if (GCSetings.GetRadarState() && newest.Tappos == tappos_D && newest.iSpeed <= m_iSpeedThres) {
		if (
			(m_ucRadarSegMin[0] != 0 && m_ucRadarSegMin[0] != 0x0e && m_ucRadarSegMin[0] <= m_ucRadarTriggerLevel[0]) ||  //  前雷达
			(m_ucRadarSegMin[2] != 0 && m_ucRadarSegMin[2] != 0x0e && m_ucRadarSegMin[2] <= m_ucRadarTriggerLevel[2]) ||  //  左雷达
			(m_ucRadarSegMin[3] != 0 && m_ucRadarSegMin[3] != 0x0e && m_ucRadarSegMin[3] <= m_ucRadarTriggerLevel[3]) ||  //  右雷达
			0) {
			return 1;
		}
	}
	return 0;
}

int CCarStateMonitor::TryToOtherOpenState(CARRUNDATA newest, CARRUNDATA old)
{
	unsigned int uiTimeStampCur = GetTickCount();
	int iIsOn = 0;
	INILogic udtLogic;
	GCSetings.GetLogic(udtLogic);

	if (m_iScreenOnType != screenontype_door && newest.iDoorState != 0) {
		m_iScreenOnType = screenontype_door;
		m_uiTimeStampLastOp = uiTimeStampCur;
		iIsOn = 1;
	}
	else if (m_iScreenOnType != screenontype_radar && checkRadar(newest, old) &&
		(!(m_iScreenOnType == screenontype_boot && m_subTypeBoot == subtype_boot_rot360))) {
		m_iScreenOnType = screenontype_radar;
		m_uiTimeStampLastOp = uiTimeStampCur;
		iIsOn = 1;
	}
	else if (m_iScreenOnType != screenontype_led && newest.iSpeed < m_iSpeedThres &&      //  不判断是否是R档 ( R档时速度可能超15km/h )
		((m_CarDataNewest.LedState == ledstate_lefton && udtLogic.left != bvlogic_none) ||
			m_CarDataNewest.LedState == ledstate_righton && udtLogic.right != bvlogic_none)) {
		iIsOn = 1;                                                                          //  不需要更改 m_iScreenOnType 为 led
	}
	else if (m_iScreenOnType != screenontype_steering &&
		IsSteeringTrigger(newest, old, TriggerStatusHigh)) {
		m_iScreenOnType = screenontype_steering;
		iIsOn = 1;
	}
	else if (m_iScreenOnType != screenontype_pkey && newest.iPKey) {
		m_iScreenOnType = screenontype_pkey;
		iIsOn = 1;
	}
	else if (m_iScreenOnType != screenontype_low_speed && newest.iSpeed < m_iLowSpeed) {
		m_iScreenOnType = screenontype_low_speed;
		iIsOn = 1;
	}
// 	else if (qApp.getViewMode() == viewmode_menu && m_iScreenOnType != screenontype_door) {
// 		//  如果当前3D车模正在自动旋转，则需要等转完之后再计算空闲时间, 但要排除开门触发的情况，否则会在normal和door开屏间反复切换，不再关屏。
// 		int iIsRotViewId = 0;
// 		int iIsAutoRot = 0;
// 		int iIsRotEnd = 0;
// 		float fAngleCur = 0;
// 		viewGetRotState(&iIsRotViewId, &iIsAutoRot, &iIsRotEnd, &fAngleCur);

// #if WIN32
// 		iIsOn = 1;
// #endif
// 		if (iIsRotViewId && iIsAutoRot && iIsRotEnd == 0) {
// 			//m_iScreenOnType = screenontype_normal;
// 			m_uiTimeStampLastOp = uiTimeStampCur;
// 			iIsOn = 1;
// 		}
// 	}

	if (iIsOn) {
		//OnOff(1, 0);    //  之前已经是打开状态，因此不需要延时，直接打开即可
	}
	else {
		OnOff(0);
	}

	return iIsOn;
}

bool CCarStateMonitor::turnRightOpsAction(CARRUNDATA carData, int appMode, int screenOn)
{
	if (screenOn) {
		if (GCSetings.GetRightTriggerOriginal() &&
			(carData.LedState == ledstate_righton)) {
			if (m_turnRightOps.isTriger) {
				OnOff(0);
			}
			else {
				m_turnRightOps.isTriger = true;
				// if (appMode == APP_MODE_CARLIFE) {
				// 	m_turnRightOps.interfaceLast = InterfaceLink;
				// 	OnOff(0);
				// }
				// else {
					m_turnRightOps.interfaceLast = InterfaceAvm;
					OnOff(0);
				// }
				m_turnRightOps.screenontypeLast = m_iScreenOnType;
			}
			return true;
		}
		else {
			if (m_turnRightOps.isTriger) {
				m_turnRightOps.isTriger = false;
			}
			return false;
		}
	}
	else {
		if (GCSetings.GetRightTriggerOriginal() &&
			(carData.LedState == ledstate_righton)) {
			if (!m_turnRightOps.isTriger) {
				m_turnRightOps.isTriger = true;
				// if (appMode == APP_MODE_CARLIFE) {
				// 	m_turnRightOps.interfaceLast = InterfaceLink;
				// }
				// else {
					m_turnRightOps.interfaceLast = InterfaceCar;
				// }
				m_turnRightOps.screenontypeLast = m_iScreenOnType;
			}
			return true;
		}
		else {
			if (m_turnRightOps.isTriger) {
				m_turnRightOps.isTriger = false;
				if (m_turnRightOps.interfaceLast == InterfaceAvm) {
					m_iScreenOnType = m_turnRightOps.screenontypeLast;
					//HideCarlife();
					m_subTypeBoot = subtype_boot_front;
					OnOff(1);
				}
				else if (m_turnRightOps.interfaceLast == InterfaceLink) {
					m_iScreenOnType = m_turnRightOps.screenontypeLast;
					//ShowCarlife();
					m_subTypeBoot = subtype_boot_front;
					OnOff(1);
				}
				return false;
			}
			return false;
		}
		return false;
	}
}

CMySeq::CMySeq()
{
	m_bEnable = true;
	memset(&m_StateSeq, 0, sizeof(m_StateSeq));
	CreateSeq(32);
}

CMySeq::CMySeq(int iMaxSeq)
{
	memset(&m_StateSeq, 0, sizeof(m_StateSeq));
	CreateSeq(iMaxSeq);
}

CMySeq::~CMySeq()
{
	if (m_StateSeq.seq) {
		delete[] m_StateSeq.seq;
	}
}

void CMySeq::CreateSeq(int iMaxSeq)
{
	if (m_StateSeq.seq) delete[] m_StateSeq.seq;

	m_iMaxSeq = iMaxSeq;
	memset(&m_StateSeq, 0, sizeof(m_StateSeq));
	m_StateSeq.seq = new STATE_PER[iMaxSeq]();
	m_StateSeq.iCur = -1;
}

void CMySeq::PrintSeq(const char* who, const int where)
{
	state_seq* pStateSeq = &m_StateSeq;
	for (int i = pStateSeq->iCur - 1; i >= 0; i--) {
		printf("%s(%d): [%d]. uiTime=%d, uiState=%d\n", who, where, i, pStateSeq->seq[GetIndex(i)].uiTime, pStateSeq->seq[GetIndex(i)].uiState);
	}
}

// 每发生一次有效的状态变化（比如从关变开，或开变关），就会调用 Push(iCurState)，然后 iCur++
void CMySeq::Push(int iCurState)
{
	state_seq* pStateSeq = &m_StateSeq;
	if (pStateSeq->iCur > 0) {
		//  仅有变化时才压入栈
		if (iCurState != pStateSeq->seq[GetIndex(pStateSeq->iCur - 1)].uiState) {
			// 计算与前一次翻转的时间间隔
			unsigned int uiTimeInterval = GetTickCount() - pStateSeq->seq[GetIndex(pStateSeq->iCur - 1)].uiTime;
			if ((!m_bEnable) || (uiTimeInterval > 500)) {
				// 如果关闭了双闪间隔太短过滤开关 或 距离前一次翻转时间大于500ms，则正常入栈
				pStateSeq->seq[GetIndex(pStateSeq->iCur)].uiState = iCurState;
				pStateSeq->seq[GetIndex(pStateSeq->iCur)].uiTime = GetTickCount();
				pStateSeq->iCur++;
			}
			else {
				// 如果距离前一次翻转时间不大于500ms，则不入栈，且将上一次状态和时间进行更新。
				pStateSeq->seq[GetIndex(pStateSeq->iCur - 1)].uiState = iCurState;
				pStateSeq->seq[GetIndex(pStateSeq->iCur - 1)].uiTime = GetTickCount();
			}
		}

	}
	else {
		pStateSeq->seq[0].uiState = iCurState;
		pStateSeq->seq[0].uiTime = GetTickCount();
		pStateSeq->iCur = 1;
	}
}

int CMySeq::GetChangeCnt(unsigned int uiTimeLimit /*= 2000*/)
{
	state_seq* pStateSeq = &m_StateSeq;
	if (pStateSeq->iCur < 2) {          //  没有变化
		return 0;
	}
	unsigned int uiTimeCur = GetTickCount();// 获取当前系统时间（毫秒）

	int iCnt = 0;
	//printf("pStateSeq->iCur - 1 = %d\n", pStateSeq->iCur - 1);
	//PrintSeq(__func__, __LINE__);

	//  统计离当前时间小于 uiTimeLimit 的事件个数
	for (int i = pStateSeq->iCur - 1; i >= 0; i--) {
		if (pStateSeq->seq[GetIndex(i)].uiTime + uiTimeLimit >= uiTimeCur) {
			iCnt++;
		}
		else {
			break;
		}
	}

	return iCnt;
}

void CMySeq::setEnable(bool en)
{
	m_bEnable = en;
}

void CMySeq::Clear(void)
{
	m_StateSeq.iCur = 0;
}

int CMySeq::GetIndex(int iIndex)
{
	return iIndex % m_iMaxSeq;
}
