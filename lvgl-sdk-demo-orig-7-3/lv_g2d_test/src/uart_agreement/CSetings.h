#pragma once
#include <iostream>
#include <string.h>
#include <atomic>
#include <vector>
#include <map>

#include "core/bv_types.h"
// #include "core/bvrect.h"

typedef struct INILogic {
	bvtap_type		front;
	bvtap_type		rear;
	bvlogic_type   left;
	bvlogic_type   right;

	bool operator != (const INILogic& rhs) const
	{
		return front != rhs.front || rear != rhs.rear || left != rhs.left || right != rhs.right;
	}
}INILogic;

typedef struct INICarVehicle {
	int		brand;
	int		wheelbase;
	char    name[64];

// #if VEHICLE_TABLE_TYPE
// #if 0
// 	char	type[8];
// 	int		length;
// 	int		width;
// 	int		height;
// 	int		front_gauge;
// 	int		rear_gauge;
// 	int		front_suspension;
// 	int		rear_suspension;
// 	float	min_radius;
// 	float	radius_inside;
// 	float	radius_outside;
// #else
// 	uchar	angle;
// #endif
	bool operator != (const INICarVehicle& rhs) const
	{
		if (strcmp(name, rhs.name))
			return true;

// #if VEHICLE_TABLE_TYPE
// #if 0
// 		if (strcmp(type, rhs.type))
// 			return true;
// #else
// 		if (angle != rhs.angle)
// 			return true;
// #endif
		return brand != rhs.brand;
	}

}INICarVehicle;

typedef struct INIParkMonitor {
	park_monitor_func	used;
	park_monitor_power	power;
	park_monitor_state  enable;
	int      fps;
	int      hours;
	float    volt;

	bool operator != (const INIParkMonitor& rhs) const
	{
		return fps != rhs.fps || hours != rhs.hours || enable != rhs.enable
			|| volt != rhs.volt || used != rhs.used || power != rhs.power;
	}
}INIParkMonitor;

// typedef struct INICarID {
// 	carid_type      type;
// 	char            text[8][4];

// 	bool operator != (const INICarID& rhs) const
// 	{
// 		for (int i = 0; i < 8; i++)
// 		{
// 			if (strcmp(text[i], rhs.text[i]))
// 				return true;
// 		}
// 		return type != rhs.type;
// 	}
// }INICarID;

typedef struct INIPanoramaExit {
	uint led;
	uint tap;
	uint remote;
	bool operator != (const INIPanoramaExit& rhs) const
	{
		return led != rhs.led || tap != rhs.tap || remote != rhs.remote;
	}
}INIPanoramaExit;

typedef struct INICarColor {
	int hue;
	int saturate;
	int value;

	bool operator != (const INICarColor& rhs) const
	{
		return hue != rhs.hue || saturate != rhs.saturate || value != rhs.value;
	}
}INICarColor;

typedef struct INICameraParam {
	int brightness;
	int contrast;
	int saturate;

	bool operator != (const INICameraParam& rhs) const
	{
		return brightness != rhs.brightness || contrast != rhs.contrast || saturate != rhs.saturate;
	}
}INICameraParam;

// typedef struct INITrack {
// 	track_type		type;
// 	bool			front;
// 	bool			rear;
// 	bool			src_front;
// 	bool			src_rear;

// 	bool operator != (const INITrack& rhs) const
// 	{
// 		return type != rhs.type || front != rhs.front || rear != rhs.rear || src_front != rhs.src_front || src_rear != rhs.src_rear;
// 	}
// }INITrack;

#define SEL_LAN_STR(type, cn, en, tw)   (type == lantype_cn ? cn : (type == lantype_tw ? tw : en))

static char* SelLanStr(lantype type, char* cn, char* en, char* tw) {
	return SEL_LAN_STR(type, cn, en, tw);
}

class CSetings;
#define GCSetings CSetings::instance()

class CSetings
{
public:
	static CSetings& instance();

	virtual ~CSetings() {};

protected:
	CSetings();
	CSetings(const CSetings&) = delete;
	CSetings& operator=(const CSetings&) = delete;

public:
	void Wirte2File();
	void SetWirteChange(bool en);

	//语言选项
	void GetLan(char* pcLan);
	std::string GetLanStr();

	lantype GetLanType();
	void SetLanType(lantype type);

	//左右灯和档位设置
	void GetLogic(INILogic& logic);
	void SetLogic(INILogic& logic);

	//速度超过多少退出360
	uint GetLogicSpeed();
	void SetLogicSpeed(uint speed);

	//车型参数
	void GetCarVehicle(INICarVehicle& vehicle);
	void SetCarVehicle(INICarVehicle& vehicle);
	int GetWheelBase();

	//停车监控
	void SetParkMonitor(INIParkMonitor& park);
	void GetParkMonitor(INIParkMonitor& park);
	void SetParkVoltage(float value);
	float GetParkVoltage();
	// park_monitor_state GetParkState();
	// void SetParkState(park_monitor_state state);
	//停车监控隐藏关闭功能
	bool GetParkMonitorShowEn();

	//录像状态
	bool GetRecord() const;
	void SetRecord(bool en);

	//雷达状态
	bool GetRadarState()const;
	void SetRadarState(bool en);

	//cvbs常显开关
	bool GetCvbsOutputState() const;
	void SetCvbsOutputState(bool en);

	//双闪触发
	bool GetLedDoubleSwitchState() const;
	void SetLedDoubleSwitchState(bool en);

	//车底透明
	int GetHyaline() const;
	void SetHyaline(int val);

	//????
	// void SetCarId(INICarID& carId);
	// void GetCarId(INICarID& carId);

	//车牌开关
	bool GetCarIdDisplayEn();
	void SetCarIdDisplayEn(bool en);

	//有一些不需要显示车牌的
	bool GetPlateSwitch() const;

	//轨迹跟随
	bool GetTrackFollowEn();
	void SetTrackFollowEn(bool en);

	//开机旋转
	int GetBootRotate();
	void SetBootRotate(int val);

	//360退出设置
	void GetPanoramaExitTime(INIPanoramaExit& panora);
	void SetPanoramaExitTime(INIPanoramaExit& panora);

	//在线渲染车身颜色
	void SetCarColor(INICarColor& color);
	void GetCarColor(INICarColor& color);

	//设置摄像头类型时候需要设置默认的值
	void SetCameraSize(int size);

	//当前调节的摄像头
	int GetCamIndex();
	void SetCamIndex(int index);
	//获取每个摄像头参数
	void GetCameraParam(INICameraParam& param, int index);
	void SetCameraParam(INICameraParam& param, int index);
	//获取摄像头类型的默认参数
	void GetDefCameraParam(int camSize, INICameraParam& param);

	//屏幕亮度
	int GetDefaultGamma();
	int GetScreenGamma();
	void SetScreenGamma(int value);

	//这个需要保留,增加一个手动开关
	uint GetPortEnable();
	//获取串口ID
	uint GetFirstPort();
	void SetFirstPort(uint port);

	//串口二可以动态变化设置
	uint GetSecondPort();
	void SetSecondPort(uint port);
	void SetSecondPortBaud(uint baud);
	uint GetSecondPortBaud();

	//2d????
	// aerial_view GetAerialView();
	// void SetAerialView(aerial_view view);

	//??????
	void SetResolution(int w, int h);
	// void SetResolutionStyle(resolution_type style);
	// resolution_type GetResolutionStyle();
	// //???????С
	// BvSize GetCanvasSize();

	//???ó??·???????????汾?????????
	std::string GetCarModelPath();
	void SetCarModelPath(std::string strPath);
	void checkCarModeState();
	void checkCarModelPath();

	//检测是否支持开门
	bool IsSupportDoor();
	//检测是否支持车牌
	bool IsSupportCarPlate();

	//获取密码
	std::string GetPassword();

	//底板波特率
	int GetMucPortBaudRate();
	void SetMucPortBaudRate(int baud);

	//开机延时
	uint GetBootDelay();
	void SetBootDelay(uint val);

	//双闪间隔延时开关
	bool GetDoubleDelayEn();
	void SetDoubleDelayEn(bool en);

	//轨迹设置
	void InitTrack();
	// void SetTrackState(INITrack& track);
	// void GetTrackState(INITrack& track);
	// void SetTrackType(track_type type);
	// track_type GetTrackType();

	//遥控器类型
	int GetRemoteType();
	void SetRemoteType(int idx);

	//屏幕旋转
	void SetDispRotateAngle(rotate_angle angle);
	rotate_angle GetDispRotateAngle();

	//触摸坐标旋转
	void SetTSRotateAngle(rotate_angle angle);
	rotate_angle GetTSRotateAngle();

	//时钟偏置
	void SetClockFreq(int val);
	int GetClockFreq();
	void SetClockFreqEn(int val);
	int GetClockFreqEn();

	//倒车触发
	bool GetRearSwitch360();
	void SetRearSwitch360(bool en);

	//右转向灯触发回原车
	void SetRightTriggerOriginal(bool en);
	bool GetRightTriggerOriginal();

	//雷达触发距离
	void SetRadarTriggerDistance(int val);
	int GetRadarTriggerDistance();

	//双闪延时间隔时间设置
	void SetDoubleDelayTime(uint val);
	uint GetDoubleDelayTime();

	//3D雷达墙
	bool GetRadar3D();
	void SetRadar3D(bool en);
	int GetRadar3DStyle();
	void SetRadar3DStyle(int style);

	//转向触发设置
	void SetSteerAngle(int val);
	int GetSteerAngle();
	void SetSteerAngleType(int val);
	int GetSteerAngleType();

	//低速触发全景
	int GetLowSpeed();
	void SetLowSpeed(int val);
	
	//开关同显功能
	void SetDualSwitch(bool en);
	bool GetDualSwitch();

	//UI???
	// ui_type GetUIStyle();
	// void SetUIStyle(ui_type type);
	void InitUIStyle();
	
	//?????????????????
	void SetBYDDispReversal(bool en);
	bool GetBYDDispReversal();
	bool GetBYDStyle();

	//3D??????
	void SetFullRotate(bool en);
	bool GetFullRotate();

	//???????????
	void SetLedAnimation(bool en);
	bool GetLedAnimation();
	
protected:
	void checkParam();
	void change() { m_bChange = true; }
	void initXml();
	void initLan();

private:
	char m_chLan[4];
	INILogic            m_udtLogic;
	INICarVehicle       m_udtCarVehicle;
	INIParkMonitor      m_udtPark;
	INIPanoramaExit     m_udtExit;
	// INICarID            m_udtCarID;
	INICarColor         m_udtColor;
	// INITrack			m_udtTrack;
	//?????????
	int                 m_nCamIndex;
	INICameraParam      m_udtDefCam1080;
	INICameraParam      m_udtDefCam720;
	INICameraParam      m_udtCam[4];

	std::string m_strCarModelPath;
	std::string m_strPassword;

	uint m_nFirstPort;
	uint m_nSecondPort;
	uint m_nSecondPortBaud;
	uint m_nPortEnable;
	uint m_nBootDelay;
	uint m_nDoubleDelayTime;

	//如果有修改就写文件
	bool m_bChange;
	bool m_bRadar;
	bool m_bCVBSOutput;
	bool m_bDoubleSwitch;
	bool m_bRecord;
	bool m_bIDDisplay;
	bool m_bTrackFollow;
	bool m_bDoorState;
	bool m_bCarPlateState;
	bool m_bShowParkMonitor;
	bool m_bDoubleDelay;
	bool m_bRearSwitch360;
	bool m_bRightTrigger;
	bool m_bRadar3D;
	bool m_bDualSwitch;

	// resolution_type m_enStyle;
	// aerial_view m_enAerialView;
	rotate_angle m_dispRotate;
	rotate_angle m_tsRotate;
	// lantype m_enLan;

	int m_nRotate;
	int m_nScreenGamma;
	int m_nDefaultGamma;
	int m_nSpeed;
	int m_mcuPortBaudRate;
	int m_nClockFreq;
	int m_nClockFreqEn;
	int m_nHyaline;
	int m_nRadarTriggerDistance;
	int m_nRadar3DStyle;

	//????
	int m_nLowSpeed;
	int m_nSteerAngle;
	int m_nSteerAngleType;

	//????
	// ui_type m_enUi;
	int m_nID8Idx;
	bool m_bBydStyle;
	bool m_bBYDDispReversal;

	//????????
	bool m_bFullRotate;
	bool m_bLedAnima;
};

// #include "cv.h"

typedef struct INISrcView {
	int id;
	int x;
	int y;
	int width;
	int height;
	int mirror;
	int rotangle;

	bool operator != (const INISrcView& rhs) const
	{
		return (x != rhs.x || y != rhs.y || width != rhs.width || height != rhs.height || mirror != rhs.mirror || rotangle != rhs.rotangle);
	}
}INISrcView;

typedef struct INIModelMergeMap
{
	int id;
	// CvPoint2D32f dst[4];

	// bool operator != (const INIModelMergeMap& rhs) const
	// {
	// 	return (dst[0].x != rhs.dst[0].x || dst[0].y != rhs.dst[0].y || dst[1].x != rhs.dst[1].x || dst[1].y != rhs.dst[1].y ||
	// 		dst[2].x != rhs.dst[2].x || dst[2].y != rhs.dst[2].y || dst[3].x != rhs.dst[3].x || dst[3].y != rhs.dst[3].y);
	// }

	bool operator==(const INIModelMergeMap& other) const {
		return id == other.id;
	}
}INIModelMergeMap;

typedef struct INIModelUndistort
{
	int id;
	// CvScalar fov;

	// bool operator != (const INIModelUndistort& rhs) const
	// {
	// 	return (fov.val[0] != rhs.fov.val[0] || fov.val[1] != rhs.fov.val[1] || fov.val[2] != rhs.fov.val[2] || fov.val[3] != rhs.fov.val[3]);
	// }
	bool operator==(const INIModelUndistort& other) const {
		return id == other.id;
	}
}INIModelUndistort;

typedef struct INIViewModelRect
{
	int model;
	int type;
	// CvRect rc;
}INIViewModelRect;

class ViewParam;
#define GViewParam ViewParam::instance()

class ViewParam {
public:
	static ViewParam& instance();

	virtual ~ViewParam() {};

protected:
	ViewParam();
	ViewParam(const ViewParam&) = delete;
	ViewParam& operator=(const ViewParam&) = delete;

public:
	void WriteFile();
	void Init();
	void RegisterViewID(const std::vector<std::pair<int, std::string>>& vec);
	void RegisterViewIDMoreModel(int viewID, const std::string& str, const std::vector<int>& vecModel, bool bHold = false);
	// void RegisterModelMergeMap(int viewID, int modelID, const std::string& str, const BvRect rc);
	// void RegisterModelUndistort(int viewID, int modelID, const std::string& str, const BvRect rc);

	void GetMergeMapStr(std::vector<int>& vec, std::vector<std::string>& vecStr);
	void GetUndistortStr(std::vector<int>& vec, std::vector<std::string>& vecStr);

	void SetSrcRect(int idx, INISrcView& src);
	void GetSrcRect(int idx, INISrcView& src);

	int GetModelRects(int id, std::vector<INIViewModelRect>& vec, int type);
	void SetModelMergeMap(int id, INIModelMergeMap& model);
	void GetModelMergeMap(int id, INIModelMergeMap& model);
	void GetModelMergeMapDefault(int id, INIModelMergeMap& model);
	void SetModelUndistort(int id, INIModelUndistort& model);
	void GetModelUndistort(int id, INIModelUndistort& model);
	void GetModelUndistortDefault(int id, INIModelUndistort& model);

protected:
	void InitSrcView();
	void InitModelView();
	void AddDefModelMergeMap(int modelID);
	void AddDefModelUndistort(int modelID);

private:
	bool m_bChange;

	std::vector<INISrcView> m_vecSrcView;
	std::vector<INISrcView> m_vecDefSrcView;

	std::vector<INIModelMergeMap> m_vecDefModelMergeMap;
	std::vector<INIModelMergeMap> m_vecModelMergeMap;

	std::vector<INIModelUndistort> m_vecDefModelUndistort;
	std::vector<INIModelUndistort> m_vecModelUndistort;

	std::map<int, bool> m_mapViewComplexModel;
	std::map<int, std::string> m_mapModelUndistort;
	std::map<int, std::string> m_mapModelMergeMap;
	std::map<int, std::vector<INIViewModelRect>> m_mapViewModel;
};
