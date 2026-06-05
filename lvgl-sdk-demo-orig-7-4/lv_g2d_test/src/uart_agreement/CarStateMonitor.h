//  用于监控车身状态，开关屏等
#ifndef __CAR_STATE_MONITOR_H
#define __CAR_STATE_MONITOR_H
#include <stdio.h>
#include <string.h>

#include "CayenneCarData.h"

void ParkMonitorSetFrameRate(int iFrameRate);

typedef void (*pfunOnOff) (int iIsOn);

enum carstate {
	carstate_boot = 0,              //  系统刚上电时默认状态
	carstate_accon_rot360,
	carstate_accon_front,
	carstate_normal,
	carstate_park_monitor,          // 停车监控
};

enum screenontype {
	screenontype_boot = 0,
	screenontype_normal,            //  不管之前是以何种方式切换的360，只要出现用户手动介入（如点触摸屏，转动旋钮等），就将转入 normal 状态。双闪打开的除外
	screenontype_taps,
	screenontype_led,
	screenontype_door,
	screenontype_radar,

	screenontype_leddouble,         //  双闪打开的 360
	screenontype_remote,            //  遥控器开屏
	screenontype_pkey,
	screenontype_steering,          //  方向盘转向触发
	screenontype_low_speed,         //  低速触发
	screenontype_end,
};

enum subtype_boot {
	subtype_boot_rot360 = 0,			// 开机旋转车模中
	subtype_boot_rot360_endstay,		// 旋转结束后停留一段时间
	subtype_boot_front,					// 前视模式
	subtype_boot_rot_wait           // 等待车机启动Log显示完成
};

//  会导致界面变化的操作类型
enum caroptype {
	caroptype_touch = 0,            //  触摸
	caroptype_open,                 //  用户手动打开 360 （指通过P键或其它键打开，而非双闪。双闪打开内部会自动处理）
	caroptype_close,                //  用户手动关闭 360
	caroptype_close_force,          //  强制关闭 360
	caroptype_close_quit,			//  强制关屏并退出程序
};

#define SCREENOFF_KEY_INTERAL_TIME      2000    /*  关屏后，多长时间再操作才能开屏, 避免屏幕开关屏抖动 */
#define CAN_SUSPENED                    1
#define CAN_ACTIVED                     0

struct state_per {
	unsigned int uiState;
	unsigned int uiTime;
};
typedef struct state_per STATE_PER;

struct onoff_evt {
	int iIsOn;
	unsigned int uiState;
	unsigned int uiTimeStamp;
	unsigned int uiTimeDlyTime;
};
typedef struct onoff_evt ONOFF_EVT;

static void onoff_default(int iIsOn) {
	if (iIsOn) {
		printf("screen on.\r\n");
	}
	else {
		printf("screen off.\r\n");
	}
}

class CMySeq
{
public:
	CMySeq();
	CMySeq(int iMaxSeq);
	~CMySeq();

	void CreateSeq(int iMaxSeq);
	void PrintSeq(const char* who, const int where);
	void Push(int iCurState);
	void Clear(void);
	int GetIndex(int iIndex);

	//  uiTimeLimit 毫秒时间内，状态变化次数
	int GetChangeCnt(unsigned int uiTimeLimit = 2000);
	void setEnable(bool en);

private:
	int m_iMaxSeq;
	struct state_seq {
		int iCur; 		// 已经存储的有效状态变化事件的总个数
		STATE_PER* seq;	// 循环数组，存放每个事件的时间和状态值
	} m_StateSeq;
	bool m_bEnable;
};

class CCarStateMonitor;
#define GCarMonitor CCarStateMonitor::instance()

class CCarStateMonitor
{
public:
	enum Interface {
		InterfaceCar,
		InterfaceAvm,
		InterfaceLink,
		InterfaceUnknown,
	};

	enum TriggerStatus {
		TriggerStatusHigh,			// 有触发信号
		TriggerStatusLow,			// 无触发信号
		TriggerStatusRising,		// 触发信号从无到有
		TriggerStatusFalling,		// 触发信号从有到无
	};

	struct turnRightOps {
		Interface interfaceLast = { InterfaceUnknown };
		screenontype screenontypeLast = { screenontype_normal };
		bool isTriger = { false };
	};

public:
	static CCarStateMonitor& instance();

	virtual ~CCarStateMonitor() {};

protected:
	CCarStateMonitor();
	CCarStateMonitor(const CCarStateMonitor&) = delete;
	CCarStateMonitor& operator=(const CCarStateMonitor&) = delete;

public:
	void Init(pfunOnOff fun);

	void Start();

	void SetTimeParam(unsigned int uiRot360Time, unsigned int uiIdleTime[screenontype_end], unsigned int uiCarStartTime);

	void SetSpeedThres(int iSpeed);

	void SetLedDoubleSwitch(bool bEn);

	void SetBootRotate(bool bEn);

	void SetRadarTriggerLevel(unsigned char front = 255, unsigned char rear = 255, unsigned char left = 255, unsigned char right = 255);

	void SetRadar(int iRadarPos, unsigned char* pucBuf, int iRadarNum, int iIsBufPack);

	void SetCarData(unsigned char* pucData, int iLen);

	void SetOpType(caroptype opType);

	void SetVoltage(int iVoltage);

	void SetKey(unsigned char* pucKeyData, int iLen);

	bool IsOpenKey(unsigned char key);

	bool IsSteeringTrigger(CARRUNDATA curData, CARRUNDATA lastData, TriggerStatus status);
	bool IsLowSpeedTrigger(CARRUNDATA curData, CARRUNDATA lastData, TriggerStatus status);

	int IsScreenOn();

	//  需单独使用线程一直调用此函数，线程里无需额外 Sleep() 休眠, while(1) { RoutinueMain(); } 即可
	void RoutinueMain();

	//  需单独使用线程一直要调用此函数，线程里无需额外 Sleep() 休眠, while(1) { RoutineOnOff(); } 即可
	void RoutineOnOff();

	bool AccOn();

	bool IsStartedParkMonitor();

	void SetRearSwitch360(bool en);
	void SetDoubleDelay(bool en);
	void SetDoubleDelayTime(int val);
	void SetLowSpeed(int iSpeed);
	void SetSteerAngle(int val);

private:
	void OnOff(int iIsOn, int iOnOffDlyMs = -1 /*  关屏时，延时多长时间打开 */);

	int checkRadar(CARRUNDATA newest, CARRUNDATA old);

	int TryToOtherOpenState(CARRUNDATA newest, CARRUNDATA old);
	bool turnRightOpsAction(CARRUNDATA carData, int appMode, int screenOn);

private:
	int          m_iScreenOnOff;
	screenontype m_iScreenOnType;

	subtype_boot m_subTypeBoot;
	unsigned int m_uiTimeStampLastOp;

	unsigned int m_uiAccOnTimeStamp;
	unsigned int m_uiAccOffTimeStamp;

	int          m_iSpeedThres;

	CMySeq       m_DoubleLedSeq;

	unsigned char m_ucRadarSegMin[4];
	unsigned char m_ucRadarTriggerLevel[4];

	CARRUNDATA   m_CarDataCur;
	CARRUNDATA   m_CarDataNewest;

	carstate     m_CarState;
	caroptype    m_LastCarOpType;

	pfunOnOff    m_pfunOnOff;

	unsigned int m_uiCarStartTime;
	unsigned int m_uiIdleTime[screenontype_end];          //  操作空闲多久后关闭 360

	unsigned int m_uiLastKeyOffScreenTime;                     //  最后一次关屏时间，此后3秒后的按键信息会忽略，避免再次误触发
	long long    m_tLastCANOffTime;
	long long    m_ACCOffTime;
	long long    m_tLastVoltageLower;
	long long    m_parkMonitorNotifyTime;

	int         m_iVoltage;
	int         m_iDoubleDelayTime;
	int         m_iLowSpeed;

	CSem         m_Sem;                                   //  信号量
	CMutex       m_Mutex;
	CMyQueue     m_QueueOnOff;

	//  车身信号
	unsigned char m_ucCarData[10];
	unsigned char m_iCarDataFlag;

	bool        m_bLedDoubleEn;
	bool        m_bBootRotateEn;
	int         m_AbnormalCnt;
	bool        m_ForcedExit;
	bool        m_RadarPause;
	bool		m_CloseAndQuit;
	bool		m_bRearSwitch360;
	turnRightOps		m_turnRightOps;

	bool m_bSpeedSteerEn;
	int m_iSpeedSteer;
	float m_fSteerAngle;
};

void* threadRoutinueMain(void* p);
void* threadRoutineOnOff(void* p);

#endif
