#include "CSetings.h"

#include "CMyFile.h"
#include "ref_inc/import_param.h"
// #include "viewerwarp.h"
// #include "viewid_def.h"
#include "CarStateMonitor.h"
#include "CSystemConf.h"
#include "threadUart.h"

#include "core/bvglobal.h"
#include "tool/util.h"
// #include "tool/caridgen.h"
// #include "tool/CExpansionBoard.h"
// #include "simpleini/SimpleIni.h"

#if defined(BV_PLATFORM_T5)
#include "enc_api.h"
#endif

// extern int GiRemoteType;

using namespace util;

static const char* bvLogicNames[] = {
	"none","2d","3d","narrow","src"
};

static const char* bvTapNames[] = {
	"none","src","wide","uview",
};

static const char* aerialNames[] = {
	"none","left","right"
};

static const char* styleNames[] = {
	"1280x720","1920x720","1024x1364","1080x1920",
};

static const char* cTrackName[] = {
	"", "jetta.vs5", "default", "benz.55", "jetta.vs5", "bmw.gp", "audi.a5l"
};

static const char* uiStyleNames[] = {
	"general","audi","bmw","benz","newC","cadillac","lexus_1280","lexus_1920", "audiA4", "audiA6", "volvo",
	"ford","newC_1364","newBmw","bmw_id8","ford_half_1","ford_half_2","byd","audiA5L","toyota",
};

// bool loadCarID(carid_type type, const char* str, char text[][4])
// {
// 	int arr[] = { 0, 7, 8, 7, 7, 4, 4 };
// 	int cnt = arr[type];

// 	std::string strStr = str;
// 	std::vector<std::string> vecStr = util::splitString(strStr, ",");

// 	bool bOk = true;
// 	if (cnt == 0 || vecStr.size() == 0)
// 	{
// 		vecStr.clear();
// 		strStr = "#18,A,8,8,8,8,8,8";
// 		vecStr = util::splitString(strStr, ",");
// 		bOk = false;
// 		cnt = 8;
// 	}

// 	for (int i = 0; i < cnt; i++)
// 	{
// 		size_t pos = vecStr[i].find("#");
// 		if (pos != std::string::npos)
// 		{
// 			std::string strNum = vecStr[i].substr(1);
// 			int num = util::string2number<int>(strNum);
// 			SetCarIDStr(num, text[i]);
// 		}
// 		else
// 			strcpy(text[i], vecStr[i].c_str());
// 	}

// 	for (int i = cnt; i < 8; i++)
// 		strcpy(text[i], "8");

// 	return bOk;
// }

// bool storeCarID(carid_type type, char text[][4], char* buf)
// {
// 	int arr[] = { 0, 7, 8, 7, 7, 4, 4 };
// 	int cnt = arr[type];

// 	if (cnt == 0)
// 	{
// 		strcpy(buf, "#18,A,8,8,8,8,8,8");
// 		return false;
// 	}

// 	std::string strText;
// 	for (int i = 0; i < 8; i++)
// 	{
// 		std::string str = text[i];
// 		if (str.size() > 1)
// 		{
// 			int index = GetCarIDIndex(text[i]);
// 			if (index != -1)
// 			{
// 				strText += ("#" + std::to_string(index));
// 			}
// 			else
// 			{
// 				if (i == 0)
// 					strText += "#18";
// 				else if (i == 1)
// 					strText += "A";
// 				else
// 					strText += "8";
// 			}
// 		}
// 		else
// 			strText += str;

// 		if (i != 7)
// 			strText += ",";
// 	}

// 	strcpy(buf, strText.c_str());

// 	return true;
// }

bool loadCameraParam(char* str, INICameraParam& cam)
{
	std::vector<std::string> vecStr = util::splitString(str, ",");
	int cnt = sizeof(INICameraParam) / sizeof(int);
	if (cnt != vecStr.size())
		return false;

	// int* arr = new int[cnt];
	// memset(arr, 0, sizeof(arr));
	int* arr = new int[cnt]();     // 圆括号表示值初始化，所有元素自动填 0

	for (int i = 0; i < vecStr.size(); ++i)
		arr[i] = atoi(vecStr[i].c_str());

	memcpy(&cam, arr, sizeof(INICameraParam));

	delete[] arr;

	return true;
}

void storeCameraParam(char* str, char* strSec, INICameraParam& cam)
{
	int arr[3] = { 0 };
	memcpy(arr, &cam, sizeof(arr));

	sprintf(str, "%s=%d,%d,%d\r\n", strSec, arr[0], arr[1], arr[2]);
}

CSetings::CSetings()
{
	m_bChange = false;
	m_bDoorState = false;
	m_bCarPlateState = false;
	m_nSecondPortBaud = 19200;

	char chPath[128];

#if WIN32
	strcpy(chPath, "setings.ini");
#else
	strcpy(chPath, "/usr/bv/setings.ini");
#endif

	CMyFile file;
	file.open(chPath, "rb");

	char pcFileBuf[FILE_BUFFER_SIZE];
	char str[128] = { 0 };

	file.read(pcFileBuf, file.getFileSize());
	file.close();

	/// <summary>
	/// lan
	/// </summary>
	getNameValueStr(pcFileBuf, "lan", "lan", "cn", m_chLan);
	initLan();

	/// <summary>
	/// logic
	/// </summary>
	getNameValueStr(pcFileBuf, "logic", "front", "none", str);
	str2Enum<bvtap_type>(bvTapNames, COUNTOF(bvTapNames), str, m_udtLogic.front);

	getNameValueStr(pcFileBuf, "logic", "rear", "none", str);
	str2Enum<bvtap_type>(bvTapNames, COUNTOF(bvTapNames), str, m_udtLogic.rear);

	getNameValueStr(pcFileBuf, "logic", "left", "2d", str);
	str2Enum<bvlogic_type>(bvLogicNames, COUNTOF(bvLogicNames), str, m_udtLogic.left);

	getNameValueStr(pcFileBuf, "logic", "right", "2d", str);
	str2Enum<bvlogic_type>(bvLogicNames, COUNTOF(bvLogicNames), str, m_udtLogic.right);

	m_nSpeed = getNameValue(pcFileBuf, "logic", "speed", 15);

	m_nRotate = getNameValue(pcFileBuf, "logic", "rotate", 2);
	m_udtExit.led = getNameValue(pcFileBuf, "logic", "led_exit", 5000);
	m_udtExit.tap = getNameValue(pcFileBuf, "logic", "tap_exit", 15000);
	m_udtExit.remote = getNameValue(pcFileBuf, "logic", "remote_exit", DELAY_MAX);

	/// <summary>
	/// vehicle
	/// </summary>
// #if VEHICLE_TABLE_TYPE
#if 0
	getNameValueStr(pcFileBuf, "vehicle", "brand", "1003", str);
	m_udtCarVehicle.brand = atoi(str);

	getNameValueStr(pcFileBuf, "vehicle", "wheelbase", "3024", str);
	m_udtCarVehicle.wheelbase = atoi(str);

	getNameValueStr(pcFileBuf, "vehicle", "type", "none", str);
	strcpy(m_udtCarVehicle.type, str);

	getNameValueStr(pcFileBuf, "vehicle", "name", "none", str);
	strcpy(m_udtCarVehicle.name, str);

	m_udtCarVehicle.length = getNameValue(pcFileBuf, "vehicle", "length", 4975);
	m_udtCarVehicle.width = getNameValue(pcFileBuf, "vehicle", "width", 1866);
	m_udtCarVehicle.height = getNameValue(pcFileBuf, "vehicle", "height", 1447);
	m_udtCarVehicle.front_gauge = getNameValue(pcFileBuf, "vehicle", "front_gauge", 1590);
	m_udtCarVehicle.rear_gauge = getNameValue(pcFileBuf, "vehicle", "rear_gauge", 1517);
	m_udtCarVehicle.front_suspension = getNameValue(pcFileBuf, "vehicle", "front_suspension", 1010);
	m_udtCarVehicle.rear_suspension = getNameValue(pcFileBuf, "vehicle", "rear_suspension", 1085);
	m_udtCarVehicle.min_radius = getNameValue(pcFileBuf, "vehicle", "min_radius", 5.8f);
	m_udtCarVehicle.radius_inside = getNameValue(pcFileBuf, "vehicle", "radius_inside", 33.0f);
	m_udtCarVehicle.radius_outside = getNameValue(pcFileBuf, "vehicle", "radius_outside", 37.0f);

#else
	// getNameValueStr(pcFileBuf, "vehicle", "brand", "4", str);
	m_udtCarVehicle.brand = atoi(str);

	// getNameValueStr(pcFileBuf, "vehicle", "wheelbase", "3012", str);
	m_udtCarVehicle.wheelbase = atoi(str);

	// getNameValueStr(pcFileBuf, "vehicle", "angle", "32", str);
	// m_udtCarVehicle.angle = atoi(str);

	// getNameValueStr(pcFileBuf, "vehicle", "name", "A6L (12~18)", str);
	strcpy(m_udtCarVehicle.name, str);
#endif
	/// <summary>
	/// system
	/// </summary>
	// m_nFirstPort = getNameValue(pcFileBuf, "system", "protocol1", 0);
	// m_nSecondPort = getNameValue(pcFileBuf, "system", "protocol2", 0);
	//0???????		1??????1????????2???		2??????1????????2??		3??????
	// m_nPortEnable = getNameValue(pcFileBuf, "system", "protocol_auto", 2);

	// m_bRadar = !!getNameValue(pcFileBuf, "system", "radar", 1);
	// m_nRadarTriggerDistance = getNameValue(pcFileBuf, "system", "radar_distance", 2);
	// m_bCVBSOutput = !!getNameValue(pcFileBuf, "system", "cvbs_output_always", output_switch);
	m_bDoubleSwitch = !!getNameValue(pcFileBuf, "system", "double_switch", 1);
	// m_mcuPortBaudRate = getNameValue(pcFileBuf, "system", "mcubaudrate", 115200);
	// m_nBootDelay = getNameValue(pcFileBuf, "system", "bootdelay", 0);
	// m_bDoubleDelay = !!getNameValue(pcFileBuf, "system", "double_delay", 0);
	// GiRemoteType = getNameValue(pcFileBuf, "system", "remote_type", 1);
	m_nDoubleDelayTime = getNameValue(pcFileBuf, "system", "double_delay_time", 3000);
	// m_bDualSwitch = !!getNameValue(pcFileBuf, "system", "dual_switch", (int)1);

	/// <summary>
	/// rotate
	/// </summary>
	// m_dispRotate = (rotate_angle)getNameValue(pcFileBuf, "rotate", "disp_rotate", rotate_0);
	// m_tsRotate = (rotate_angle)getNameValue(pcFileBuf, "rotate", "ts_rotate", rotate_0);
	// m_nClockFreq = getNameValue(pcFileBuf, "rotate", "clock_freq", 0);
	// m_nClockFreqEn = getNameValue(pcFileBuf, "rotate", "clock_freq_en", 0);

	/// <summary>
	/// track
	/// </summary>
	// m_udtTrack.type = (track_type)getNameValue(pcFileBuf, "track", "type", track_static);
	// m_udtTrack.front = getNameValue(pcFileBuf, "track", "front", 1);
	// m_udtTrack.rear = getNameValue(pcFileBuf, "track", "rear", 1);
	// m_udtTrack.src_front = getNameValue(pcFileBuf, "track", "src_front", 1);
	// m_udtTrack.src_rear = getNameValue(pcFileBuf, "track", "src_rear", 1);
	// m_bTrackFollow = !!getNameValue(pcFileBuf, "track", "track_follow", 0);

	/// <summary>
	/// park
	/// </summary>
	// m_udtPark.enable = (park_monitor_state)getNameValue(pcFileBuf, "park", "enable", pm_off);
	// if (m_udtPark.enable != pm_on_always) {
	// 	m_udtPark.enable = pm_off;
	// }
	// m_udtPark.hours = getNameValue(pcFileBuf, "park", "hours", 2);
	// m_udtPark.volt = getNameValue(pcFileBuf, "park", "volt", 11.5f);
	// m_udtPark.fps = getNameValue(pcFileBuf, "park", "fps", 1);

	// m_udtPark.power = (getNameValue(pcFileBuf, "park", "power", pm_power_core) == pm_power_core) ? pm_power_core : pm_power_decoder;
	// if (MainBd::instance().IsCoreBoard()) {
	// 	// ????壬???????????????????????????????????????????y???
	// 	m_bShowParkMonitor = !!getNameValue(pcFileBuf, "park", "show", 0);
	// 	m_udtPark.used = getNameValue(pcFileBuf, "park", "used", 0) ? pm_func_open : pm_func_hide;
	// }
	// else {
	// 	// ?????壬?????????????,?????????????????
	// 	m_bShowParkMonitor = true;			// ???????????????????y???
	// 	m_udtPark.power = pm_power_core;
	// 	m_udtPark.used = getNameValue(pcFileBuf, "park", "used", 1) ? pm_func_open : pm_func_hide;
	// }

	// m_udtPark.used = (m_bShowParkMonitor == false) ? pm_func_hide : m_udtPark.used;
	// m_udtPark.enable = (m_udtPark.used == pm_func_hide) ? pm_off : m_udtPark.enable;	// ??????????????

	/// <summary>
	/// car_id
	/// </summary>
	// m_bIDDisplay = !!getNameValue(pcFileBuf, "car_id", "en", 1);
	// m_udtCarID.type = (carid_type)getNameValue(pcFileBuf, "car_id", "type", carid_blue);
	// getNameValueStr(pcFileBuf, "car_id", "text", "#18,A,8,8,8,8,8,8", str);
	// loadCarID(m_udtCarID.type, str, m_udtCarID.text);

	/// <summary>
	/// recorder
	/// </summary>
	// m_bRecord = !!getNameValue(pcFileBuf, "recorder", "recorder", (int)1);

	/// <summary>
	/// car_model
	/// </summary>
// #if OPENGL_PICTURE
#if 0
	getNameValueStr(pcFileBuf, "car_model", "model_path", "bvresource", str);
	m_strCarModelPath = str;
	checkCarModeState();
#else
	/// <summary>
	/// car_color
	/// </summary>
	// m_udtColor.hue = getNameValue(pcFileBuf, "car_color", "hue", 360);
	// m_udtColor.saturate = getNameValue(pcFileBuf, "car_color", "saturate", 0);
	// m_udtColor.value = getNameValue(pcFileBuf, "car_color", "value", 98);

	// getNameValueStr(pcFileBuf, "car_model", "model_path", "Audi/A6L", str);
	m_bDoorState = true;
	m_bCarPlateState = true;
	m_strCarModelPath = str;
#endif
	// m_nHyaline = getNameValue(pcFileBuf, "car_model", "hyaline", 0);

	/// <summary>
	/// screen
	/// </summary>
	// m_nDefaultGamma = getNameValue(pcFileBuf, "screen", "def_gamma", (int)50);
	// m_nScreenGamma = getNameValue(pcFileBuf, "screen", "gamma", (int)50);

	/// <summary>
	/// camera
	/// </summary>
	m_nCamIndex = 0;		//??粻??????????
	// int val = getNameValue(pcFileBuf, "camera", "def_bright_1080", (int)999999);
	// if (val == 999999)
	// {
	// 	auto callback = [&](char* strKey, INICameraParam& cam) {
	// 		// getNameValueStr(pcFileBuf, "camera", strKey, "40,70,81", str);
	// 		loadCameraParam(str, cam);
	// 	};

	// 	for (int i = 0; i < 4; i++)
	// 	{
	// 		char ch[16] = { 0 };
	// 		sprintf(ch, "cam_%d", i);
	// 		callback(ch, m_udtCam[i]);
	// 	}

	// 	callback("cam_720", m_udtDefCam720);
	// 	callback("cam_1080", m_udtDefCam1080);
	// }
	// else
	// {
	// 	m_udtDefCam1080.brightness = getNameValue(pcFileBuf, "camera", "def_bright_1080", (int)42);
	// 	m_udtDefCam1080.contrast = getNameValue(pcFileBuf, "camera", "def_contra_1080", (int)65);
	// 	m_udtDefCam1080.saturate = getNameValue(pcFileBuf, "camera", "def_satura_1080", (int)80);

	// 	m_udtDefCam720.brightness = getNameValue(pcFileBuf, "camera", "def_bright_720", (int)40);
	// 	m_udtDefCam720.contrast = getNameValue(pcFileBuf, "camera", "def_contra_720", (int)70);
	// 	m_udtDefCam720.saturate = getNameValue(pcFileBuf, "camera", "def_satura_720", (int)81);

	// 	auto camCallback = [&](int index, int def, const char* text)->int {
	// 		char ch[16] = { 0 };
	// 		sprintf(ch, "%s_%d", text, index);

	// 		return getNameValue(pcFileBuf, "camera", ch, def);
	// 	};

	// 	for (int i = 0; i < 4; i++)
	// 	{
	// 		m_udtCam[i].brightness = camCallback(i, m_udtDefCam1080.brightness, "bright");
	// 		m_udtCam[i].contrast = camCallback(i, m_udtDefCam1080.contrast, "contra");
	// 		m_udtCam[i].saturate = camCallback(i, m_udtDefCam1080.saturate, "satura");
	// 	}
	// }

	/// <summary>
	/// special_switch
	/// </summary>
	// getNameValueStr(pcFileBuf, "special_switch", "aerial_view", "right", str);
	// str2Enum<aerial_view>(aerialNames, COUNTOF(aerialNames), str, m_enAerialView);

	// getNameValueStr(pcFileBuf, "special_switch", "widget_style", "1280x720", str);
	// str2Enum<resolution_type>(styleNames, COUNTOF(styleNames), str, m_enStyle);

	// getNameValueStr(pcFileBuf, "special_switch", "ui_type", "general", str);
	// str2Enum<ui_type>(uiStyleNames, COUNTOF(uiStyleNames), str, m_enUi);

	m_bRearSwitch360 = !!getNameValue(pcFileBuf, "special_switch", "rear_switch", 1);
	// m_bRightTrigger = !!getNameValue(pcFileBuf, "special_switch", "right_trigger", (int)0);

	// m_bRadar3D = !!getNameValue(pcFileBuf, "special_switch", "radar_3d", (int)0);
	// m_nRadar3DStyle = getNameValue(pcFileBuf, "special_switch", "radar3d_style", (int)0);
	
	// m_bBydStyle = !!getNameValue(pcFileBuf, "special_switch", "byd_style", 0);
	// m_bBYDDispReversal = !!getNameValue(pcFileBuf, "special_switch", "byd_disp_reversal", (int)0);

	/// <summary>
	/// passwd
	/// </summary>
	// getNameValueStr(pcFileBuf, "passwd", "root", "8888", str);
	m_strPassword = str;

	/// <summary>
	/// steer_speed
	/// </summary>
	// m_nSteerAngle = getNameValue(pcFileBuf, "steer_speed", "angle", (int)0);
	// m_nSteerAngleType = getNameValue(pcFileBuf, "steer_speed", "type", (int)0);
	// m_nLowSpeed = getNameValue(pcFileBuf, "steer_speed", "low_speed", 0);
	
	/// <summary>
	/// other
	/// </summary>
	// m_bFullRotate = !!getNameValue(pcFileBuf, "other", "full_rotate", (int)1);
	// m_bLedAnima = !!getNameValue(pcFileBuf, "other", "led_anima", (int)0);

	checkParam();
	initXml();
}

CSetings& CSetings::instance()
{
	static CSetings _instance;
	return _instance;
}

void CSetings::Wirte2File()
{
	if (!m_bChange)
		return;

	m_bChange = false;

	char pcFileBuf[FILE_BUFFER_SIZE] = { 0 };

	char str[512] = { 0 };
	char text[512] = { 0 };

	// strcpy(pcFileBuf, "[setings]\r\nver=cayenne\r\n\r\n");
	/// <summary>
	/// lan
	/// </summary>
	sprintf(str, "[lan]\r\nlan=%s\r\n\r\n", m_chLan);
	strcat(pcFileBuf, str);

	/// <summary>
	/// logic
	/// </summary>
	sprintf(str, "[logic]\r\nrotate=%d\r\nfront=%s\r\nrear=%s\r\nleft=%s\r\nright=%s\r\n\r\n",
		m_nRotate, bvTapNames[m_udtLogic.front], bvTapNames[m_udtLogic.rear], bvLogicNames[m_udtLogic.left], bvLogicNames[m_udtLogic.right]);
	strcat(pcFileBuf, str);

	sprintf(str, "speed=%d\r\nled_exit=%d\r\ntap_exit=%d\r\nremote_exit=%d\r\n\r\n",
		m_nSpeed, m_udtExit.led, m_udtExit.tap, m_udtExit.remote);
	strcat(pcFileBuf, str);

	/// <summary>
	/// vehicle
	/// </summary>

// #if VEHICLE_TABLE_TYPE
#if 0
	sprintf(str, "[vehicle]\r\nbrand=%d\r\nwheelbase=%d\r\ntype=%s\r\nname=%s\r\n\r\n",
		m_udtCarVehicle.brand, m_udtCarVehicle.wheelbase, m_udtCarVehicle.type, m_udtCarVehicle.name);
	strcat(pcFileBuf, str);

	sprintf(str, "length=%d\r\nwidth=%d\r\nheight=%d\r\nfront_gauge=%d\r\nrear_gauge=%d\r\nfront_suspension=%d\r\nrear_suspension=%d\r\n",
		m_udtCarVehicle.length, m_udtCarVehicle.width, m_udtCarVehicle.height, m_udtCarVehicle.front_gauge, m_udtCarVehicle.rear_gauge, m_udtCarVehicle.front_suspension, m_udtCarVehicle.rear_suspension);
	strcat(pcFileBuf, str);

	sprintf(str, "min_radius=%2.3f\r\nradius_inside=%2.3f\r\nradius_outside=%2.3f\r\n\r\n",
		m_udtCarVehicle.min_radius, m_udtCarVehicle.radius_inside, m_udtCarVehicle.radius_outside);
	strcat(pcFileBuf, str);
#else
	// sprintf(str, "[vehicle]\r\nbrand=%d\r\nwheelbase=%d\r\nangle=%d\r\nname=%s\r\n\r\n",
	// 	m_udtCarVehicle.brand, m_udtCarVehicle.wheelbase, m_udtCarVehicle.angle, m_udtCarVehicle.name);
	// strcat(pcFileBuf, str);
#endif
	/// <summary>
	/// system
	/// </summary>
	sprintf(str, "[system]\r\nprotocol1=%d\r\nprotocol2=%d\r\nprotocol_auto=%d\r\nradar=%d\r\ndouble_switch=%d\r\ncvbs_output_always=%d\r\nmcubaudrate=%d\r\n", m_nFirstPort, m_nSecondPort, m_nPortEnable, m_bRadar, m_bDoubleSwitch, m_bCVBSOutput, m_mcuPortBaudRate);
	strcat(pcFileBuf, str);

	sprintf(str, "bootdelay=%d\r\ndouble_delay=%d\r\nradar_distance=%d\r\ndouble_delay_time=%d\r\ndual_switch=%d\r\n\r\n",
		m_nBootDelay, m_bDoubleDelay, m_nRadarTriggerDistance, m_nDoubleDelayTime, m_bDualSwitch);
	strcat(pcFileBuf, str);

	/// <summary>
	/// ???
	/// </summary>
	// sprintf(str, "[rotate]\r\ndisp_rotate=%d\r\nts_rotate=%d\r\nclock_freq=%d\r\nclock_freq_en=%d\r\n\r\n",
	// 	m_dispRotate, m_tsRotate, m_nClockFreq, m_nClockFreqEn);
	// strcat(pcFileBuf, str);

	/// <summary>
	/// ??
	/// </summary>
	// sprintf(str, "[track]\r\ntype=%d\r\nfront=%d\r\nrear=%d\r\nsrc_front=%d\r\nsrc_rear=%d\r\ntrack_follow=%d\r\n\r\n",
	// 	m_udtTrack.type, m_udtTrack.front, m_udtTrack.rear, m_udtTrack.src_front, m_udtTrack.src_rear, m_bTrackFollow);
	// strcat(pcFileBuf, str);

	/// <summary>
	/// park
	/// </summary>
	// sprintf(str, "[park]\r\nused=%d\r\npower=%d\r\nenable=%d\r\nhours=%d\r\nvolt=%2.1f\r\nfps=%d\r\nshow=%d\r\n\r\n",
	// 	m_udtPark.used == pm_func_hide ? 0 : 1, m_udtPark.power == pm_power_core ? pm_power_core : pm_power_decoder,
		// m_udtPark.enable, m_udtPark.hours, m_udtPark.volt, m_udtPark.fps, m_bShowParkMonitor);
	// strcat(pcFileBuf, str);

	/// <summary>
	/// car_id
	/// </summary>
	/// 
	// storeCarID(m_udtCarID.type, m_udtCarID.text, text);
	// sprintf(str, "[car_id]\r\nen=%d\r\ntype=%d\r\ntext=%s\r\n\r\n", m_bIDDisplay, m_udtCarID.type, text);
	// strcat(pcFileBuf, str);

// #ifndef OPENGL_PICTURE
#if 1
	/// <summary>
	/// car_color
	/// </summary>
	sprintf(str, "[car_color]\r\nhue=%d\r\nsaturate=%d\r\nvalue=%d\r\n\r\n", m_udtColor.hue, m_udtColor.saturate, m_udtColor.value);
	// strcat(pcFileBuf, str);
#endif // !OPENGL_PICTURE

	/// <summary>
	/// recorder
	/// </summary>
	sprintf(str, "[recorder]\r\nrecorder=%d\r\n\r\n", m_bRecord);
	// strcat(pcFileBuf, str);

	/// <summary>
	/// car_model
	/// </summary>
	sprintf(str, "[car_model]\r\nmodel_path=%s\r\nhyaline=%d\r\n\r\n", m_strCarModelPath.c_str(), m_nHyaline);
	// strcat(pcFileBuf, str);

	/// <summary>
	/// camera
	/// </summary>
	memset(text, 0, 512);
	for (int i = 0; i < 4; i++)
	{
		char ch[16] = { 0 };
		sprintf(ch, "cam_%d", i);
		storeCameraParam(str, ch, m_udtCam[i]);
		strcat(text, str);
	}

	storeCameraParam(str, "\r\ncam_720", m_udtDefCam720);
	strcat(text, str);

	storeCameraParam(str, "cam_1080", m_udtDefCam1080);
	strcat(text, str);

	sprintf(str, "[camera]\r\n%s\r\n", text);
	strcat(pcFileBuf, str);

	/// <summary>
	/// screen
	/// </summary>
	sprintf(str, "[screen]\r\ndef_gamma=%d\r\ngamma=%d\r\n\r\n", m_nDefaultGamma, m_nScreenGamma);
	// strcat(pcFileBuf, str);

	/// <summary>
	/// special_switch
	/// </summary>
	sprintf(str, "[special_switch]\r\nrear_switch=%d\r\nright_trigger=%d\r\nradar_3d=%d\r\nradar3d_style=%d\r\n\r\n",
		m_bRearSwitch360, m_bRightTrigger, m_bRadar3D, m_nRadar3DStyle);
	strcat(pcFileBuf, str);

	// sprintf(str, "ui_type=%s\r\nbyd_style=%d\r\nbyd_disp_reversal=%d\r\n\r\n", uiStyleNames[m_enUi], m_bBydStyle, m_bBYDDispReversal);
	strcat(pcFileBuf, str);

	/// <summary>
	/// passwd
	/// </summary>
	sprintf(str, "[passwd]\r\nroot=%s\r\n\r\n", m_strPassword.c_str());
	// strcat(pcFileBuf, str);

	/// <summary>
	/// steer_speed
	/// </summary>
	sprintf(str, "[steer_speed]\r\ntype=%d\r\nangle=%d\r\nlow_speed=%d\r\n\r\n",m_nSteerAngleType, m_nSteerAngle, m_nLowSpeed);
	// strcat(pcFileBuf, str);

	/// <summary>
	/// other
	/// </summary>
	sprintf(str, "[other]\r\nfull_rotate=%d\r\nled_anima=%d\r\n\r\n", m_bFullRotate, m_bLedAnima);
	// strcat(pcFileBuf, str);

	/// <summary>
	/// end
	/// </summary>
	strcat(pcFileBuf, "[end]\r\n");

	CMyFile file;
	char chPath[128];

// #if WIN32
#if 0
	strcpy(chPath, "setings.ini");
#else
	strcpy(chPath, "/usr/bv/setings.ini");
#endif

	file.open(chPath, "wb");
	file.write(pcFileBuf, strlen(pcFileBuf));
	file.close();
}

void CSetings::SetWirteChange(bool en)
{
	m_bChange = en;
}

void CSetings::GetLan(char* pcLan)
{
	if (pcLan)
		strcpy(pcLan, m_chLan);
}

std::string CSetings::GetLanStr()
{
	return m_chLan;
}

// lantype CSetings::GetLanType()
// {
// 	return m_enLan;
// }

// void CSetings::SetLanType(lantype type)
// {
// 	if (m_enLan != type)
// 		change();

// 	if (type == lantype_tw)
// 		strcpy(m_chLan, "tw");
// 	else if (type == lantype_en)
// 		strcpy(m_chLan, "en");
// 	else if (type == lantype_ru)
// 		strcpy(m_chLan, "ru");
// 	else if (type == lantype_jp)
// 		strcpy(m_chLan, "jp");
// 	else if (type == lantype_kr)
// 		strcpy(m_chLan, "kr");
// 	else if (type == lantype_vn)
// 		strcpy(m_chLan, "vn");
// 	else if (type == lantype_ma)
// 		strcpy(m_chLan, "ma");
// 	else
// 		strcpy(m_chLan, "cn");
// }

void CSetings::GetLogic(INILogic& logic)
{
	memcpy(&logic, &m_udtLogic, sizeof(INILogic));
}

void CSetings::SetLogic(INILogic& logic)
{
	if (logic != m_udtLogic)
		change();
	memcpy(&m_udtLogic, &logic, sizeof(INILogic));
}

uint CSetings::GetLogicSpeed()
{
	return m_nSpeed;
}

void CSetings::SetLogicSpeed(uint speed)
{
	if (speed != m_nSpeed)
	{
		GCarMonitor.SetSpeedThres(speed);
		change();
	}
	m_nSpeed = speed;
}

void CSetings::GetCarVehicle(INICarVehicle& vehicle)
{
	memcpy(&vehicle, &m_udtCarVehicle, sizeof(INICarVehicle));
}

void CSetings::SetCarVehicle(INICarVehicle& vehicle)
{
	if (vehicle != m_udtCarVehicle)
		change();
	memcpy(&m_udtCarVehicle, &vehicle, sizeof(INICarVehicle));
}

int CSetings::GetWheelBase()
{
	return m_udtCarVehicle.wheelbase;
}

void CSetings::SetParkMonitor(INIParkMonitor& park)
{
	park.enable = (park.used == pm_func_hide) ? pm_off : park.enable;	// 如隐藏，则强制关闭
	if (park != m_udtPark)
		change();
	memcpy(&m_udtPark, &park, sizeof(INIParkMonitor));
}

void CSetings::GetParkMonitor(INIParkMonitor& park)
{
	memcpy(&park, &m_udtPark, sizeof(INIParkMonitor));
}

void CSetings::SetParkVoltage(float value)
{
	// if (value != m_udtPark.volt)
	// 	change();
	// m_udtPark.volt = value;
}

float CSetings::GetParkVoltage()
{
	// return m_udtPark.volt;
}

// park_monitor_state CSetings::GetParkState()
// {
// 	return m_udtPark.enable;
// }

// void CSetings::SetParkState(park_monitor_state state)
// {
// 	if (m_udtPark.enable != state)
// 		change();
// 	m_udtPark.enable = state;
// }

bool CSetings::GetParkMonitorShowEn()
{
	return m_bShowParkMonitor;
}

bool CSetings::GetRecord() const
{
	return m_bRecord;
}

void CSetings::SetRecord(bool en)
{
	if (m_bRecord != en)
	{
// #ifndef WIN32
#if 1
		// vrEnableRecord(en);
#endif // !WIN32

		change();
	}
	m_bRecord = en;
}

bool CSetings::GetRadarState() const
{
	return m_bRadar;
}

void CSetings::SetRadarState(bool en)
{
	if (m_bRadar != en)
	{
		unsigned char stage[] = { 0, 0, 0, 0 };
		// for (int i = 0; i < 4; i++) {
		// 	viewSetRadarData(i, stage, 4, 1);
		// 	GCarMonitor.SetRadar(i, stage, 4, 1);
		// }

		change();
	}
	m_bRadar = en;
}

bool CSetings::GetCvbsOutputState() const
{
	return m_bCVBSOutput;
}

void CSetings::SetCvbsOutputState(bool en)
{
	if (m_bCVBSOutput != en)
		change();
	m_bCVBSOutput = en;
}

bool CSetings::GetLedDoubleSwitchState() const
{
	return m_bDoubleSwitch;
}

void CSetings::SetLedDoubleSwitchState(bool en)
{
	if (m_bDoubleSwitch != en)
	{
		GCarMonitor.SetLedDoubleSwitch(en);
		change();
	}
	m_bDoubleSwitch = en;
}

int CSetings::GetHyaline() const
{
	return m_nHyaline;
}

void CSetings::SetHyaline(int val)
{
	if (m_nHyaline != val)
		change();
	m_nHyaline = val;
}

// void CSetings::SetCarId(INICarID& carId)
// {
// 	if (carId != m_udtCarID)
// 		change();
// 	memcpy(&m_udtCarID, &carId, sizeof(INICarID));
// }

// void CSetings::GetCarId(INICarID& carId)
// {
// 	memcpy(&carId, &m_udtCarID, sizeof(INICarID));
// }

bool CSetings::GetCarIdDisplayEn()
{
	return m_bIDDisplay;
}

void CSetings::SetCarIdDisplayEn(bool en)
{
	if (m_bIDDisplay != en)
	{
		// viewSetCarIdEnable(en);
		// viewSetCarIdUpdate(NULL);
		change();
	}
	m_bIDDisplay = en;
}

bool CSetings::GetPlateSwitch() const
{
	// if (m_udtCarID.type == carid_none)
	// 	return false;
	return true;
}

bool CSetings::GetTrackFollowEn()
{
	return m_bTrackFollow;
}

void CSetings::SetTrackFollowEn(bool en)
{
	if (m_bTrackFollow != en)
	{
		// viewSetTrackAngleEnable(en);
		change();
	}
	m_bTrackFollow = en;
}

int CSetings::GetBootRotate()
{
	return m_nRotate;
}

void CSetings::SetBootRotate(int val)
{
	if (val < 0 || val > 3)
		return;

	if (m_nRotate != val)
	{
		change();
		// if (val == 0)
		// 	GCarMonitor.SetBootRotate(false);
		// else
		// 	GCarMonitor.SetBootRotate(true);
	}
	m_nRotate = val;
}

void CSetings::GetPanoramaExitTime(INIPanoramaExit& panora)
{
	memcpy(&panora, &m_udtExit, sizeof(INIPanoramaExit));
}

void CSetings::SetPanoramaExitTime(INIPanoramaExit& panora)
{
	if (m_udtExit != panora)
	{
		// unsigned int uiIdleTime[screenontype_end] = { panora.tap,  10000,  panora.tap, panora.led, 5000,  5000, DELAY_MAX, panora.remote };
		// GCarMonitor.SetTimeParam(0, uiIdleTime, 1);
		change();
	}
	memcpy(&m_udtExit, &panora, sizeof(INIPanoramaExit));
}

void CSetings::SetCarColor(INICarColor& color)
{
	if (m_udtColor != color)
		change();
	memcpy(&m_udtColor, &color, sizeof(INICarColor));
}

void CSetings::GetCarColor(INICarColor& color)
{
	memcpy(&color, &m_udtColor, sizeof(INICarColor));
}

void CSetings::SetCameraSize(int size)
{
	// if (size == CAMERA_SIZE_1080)
	// {
	// 	for (int i = 0; i < 4; i++)
	// 		memcpy(&m_udtCam[i], &m_udtDefCam1080, sizeof(INICameraParam));
	// }
	// else
	{
		for (int i = 0; i < 4; i++)
			memcpy(&m_udtCam[i], &m_udtDefCam720, sizeof(INICameraParam));
	}
	change();
}

int CSetings::GetCamIndex()
{
	return m_nCamIndex;
}

void CSetings::SetCamIndex(int index)
{
	m_nCamIndex = index;
}

void CSetings::GetCameraParam(INICameraParam& param, int index)
{
	memcpy(&param, &m_udtCam[index], sizeof(INICameraParam));
}

void CSetings::SetCameraParam(INICameraParam& param, int index)
{
	if (m_udtCam[index] != param)
		change();
	memcpy(&m_udtCam[index], &param, sizeof(INICameraParam));
}

void CSetings::GetDefCameraParam(int camSize, INICameraParam& param)
{
	// if (camSize == CAMERA_SIZE_1080)
	// 	memcpy(&param, &m_udtDefCam1080, sizeof(INICameraParam));
	// else if (camSize == CAMERA_SIZE_720)
	// 	memcpy(&param, &m_udtDefCam720, sizeof(INICameraParam));
}

int CSetings::GetDefaultGamma()
{
	return m_nDefaultGamma;
}

int CSetings::GetScreenGamma()
{
	return m_nScreenGamma;
}

void CSetings::SetScreenGamma(int value)
{
	if (m_nScreenGamma != value)
		change();
	m_nScreenGamma = value;
}

uint CSetings::GetPortEnable()
{
	return m_nPortEnable;
}

uint CSetings::GetFirstPort()
{
	return m_nFirstPort;
}

void CSetings::SetFirstPort(uint port)
{
	if (m_nFirstPort != port)
		change();
	m_nFirstPort = port;
}

uint CSetings::GetSecondPort()
{
	return m_nSecondPort;
}

void CSetings::SetSecondPort(uint port)
{
	if (m_nSecondPort != port)
		change();
	m_nSecondPort = port;
}

void CSetings::SetSecondPortBaud(uint baud)
{
	m_nSecondPortBaud = baud;
}

uint CSetings::GetSecondPortBaud()
{
	return m_nSecondPortBaud;
}

// aerial_view CSetings::GetAerialView()
// {
// 	return m_enAerialView;
// }

// void CSetings::SetAerialView(aerial_view view)
// {
// 	if (m_enAerialView != view)
// 		change();
// 	m_enAerialView = view;
// }

void CSetings::SetResolution(int w, int h)
{
// #if DISP_AUTO_WEIGET_STYLE
#if 0
	resolution_type style = m_enStyle;
	float fScale = w * 1.0f / h;

	if (w > h)
	{
		//????????
		if (fScale > 2.0f)
			style = resolution_1920x720;
		else
			style = resolution_1280x720;
	}
	else
	{
		//????????
		if (fScale < 0.7f)
			style = resolution_1080x1920;
		else
			style = resolution_1024x1364;
	}

	if (m_bBydStyle)
	{
		if (m_bBYDDispReversal)
		{
			if (w > h)
			{
				m_enUi = ui_general;
				style = resolution_1024x1364;
			}
			else
			{
				m_enUi = ui_byd;
				style = resolution_1280x720;
			}
		}
		else
		{
			if (w < h)
			{
				m_enUi = ui_general;
				style = resolution_1024x1364;
			}
			else
			{
				m_enUi = ui_byd;
				style = resolution_1280x720;
			}
		}
	}

	printf("----line = %d, func = %s, width = %d, height = %d, scale = %f\n", __LINE__, __FUNCTION__, w, h, fScale);
	SetResolutionStyle(style);
#endif
}

// void CSetings::SetResolutionStyle(resolution_type style)
// {
// 	if (m_enStyle != style)
// 		change();
// 	m_enStyle = style;
// }

// resolution_type CSetings::GetResolutionStyle()
// {
// 	return m_enStyle;
// }

// BvSize CSetings::GetCanvasSize()
// {
// 	BvSize size;
// 	if (m_enUi == ui_audi)
// 		size = BvSize(1540, 720);
// 	else if (m_enUi == ui_bmw || m_enUi == ui_newBmw)
// 		size = BvSize(1920, 720);
// 	else if (m_enUi == ui_benz)
// 		size = BvSize(1920, 720);
// 	else if (m_enUi == ui_newC)
// 		size = BvSize(1624, 1728);
// 	else if (m_enUi == ui_cadillac)
// 		size = BvSize(1600, 900);
// 	else if (m_enUi == ui_lexus_1280)
// 		size = BvSize(1280, 720);
// 	else if (m_enUi == ui_lexus_1920)
// 		size = BvSize(1920, 720);
// 	else if (m_enUi == ui_audiA4 || m_enUi == ui_audiA6)
// 		size = BvSize(1280, 720);
// 	else if (m_enUi == ui_volvo)
// 		size = BvSize(1024, 1364);
// 	else if (m_enUi == ui_ford || m_enUi == ui_ford_half_1 || m_enUi == ui_ford_half_2)
// 		size = BvSize(2016, 756);
// 	else if (m_enUi == ui_newC_1364)
// 		size = BvSize(1624, 1364);
// 	else if (m_enUi == ui_bmw_id8)
// 		size = BvSize(2880, 840);
// 	else if (m_enUi == ui_byd)
// 		size = BvSize(1280, 720);
// 	else if (m_enUi == ui_audiA5L)
// 		size = BvSize(1920, 720);
// 	else if (m_enUi == ui_toyota)
// 		size = BvSize(1920, 720);
// 	else
// 	{
// 		const BvSize sizes[] = { BvSize(1280, 720),BvSize(1920, 720),BvSize(1024, 1364),BvSize(1080, 1920) };
// 		size = sizes[m_enStyle];
// 	}
// 	return size;
// }

std::string CSetings::GetCarModelPath()
{
	return m_strCarModelPath;
}

void CSetings::SetCarModelPath(std::string strPath)
{
	if (strPath != m_strCarModelPath)
		change();
	m_strCarModelPath = strPath;
}

//???????????
void CSetings::checkCarModeState()
{
// #if WIN32
#if 0
	std::string strPath = "model/";
#else
	std::string strPath = "/usr/";
#endif

	std::string strCarModel = strPath + m_strCarModelPath;

	char doorModel2dStatic[256];
	char doorModel2dDynamic[256];
	char doorModel3dStatic[256];
	char doorModel3dDynamic[256];

	sprintf(doorModel2dStatic, "%s/rotate/door/2d/00.bin", strCarModel.c_str());
	sprintf(doorModel2dDynamic, "%s/rotate/door/2d/Left_00000.bin", strCarModel.c_str());
	sprintf(doorModel3dStatic, "%s/rotate/door/3d/00.bin", strCarModel.c_str());
	sprintf(doorModel3dDynamic, "%s/rotate/door/3d/Left_00000.bin", strCarModel.c_str());

	FILE* pFP2DStatic = fopen(doorModel2dStatic, "r");;
	FILE* pFP3DStatic = fopen(doorModel3dStatic, "r");
	FILE* pFP2DDynamic = fopen(doorModel2dDynamic, "r");
	FILE* pFP3DDynamic = fopen(doorModel3dDynamic, "r");

	if ((pFP2DStatic != NULL && pFP2DDynamic != NULL) || (pFP3DStatic != NULL && pFP3DDynamic != NULL))
		m_bDoorState = true;

	if (pFP2DStatic)
		fclose(pFP2DStatic);

	if (pFP3DStatic)
		fclose(pFP3DStatic);

	if (pFP2DDynamic)
		fclose(pFP2DDynamic);

	if (pFP3DDynamic)
		fclose(pFP3DDynamic);

	char carIdModelFile[256];
	sprintf(carIdModelFile, "%s/carid/3d.carid", strCarModel.c_str());

	FILE* pFPCarid = fopen(carIdModelFile, "r");
	if (pFPCarid)
	{
		m_bCarPlateState = true;
		fclose(pFPCarid);
	}
}

//??鳵?·??
void CSetings::checkCarModelPath()
{
// #if OPENGL_PICTURE
#if 0
	std::string strDefault = "bvresource";
// #if WIN32
#if 0
	std::string strPath = "model/";
#else
	std::string strPath = "/usr/";
#endif

#else
	std::string strDefault = "Audi/A6L";
// #if WIN32
#if 0
	std::string strPath = "D:/carmodel/render/";
#else
	std::string strPath = "/usr/bvresource/model/";
#endif
#endif

	strPath += m_strCarModelPath;

	char customModelFile[256];
	sprintf(customModelFile, "%s/custom3d.ini", strPath.c_str());

	FILE* pFPcustom = fopen(customModelFile, "r");
	if (pFPcustom)
		fclose(pFPcustom);
	else
		m_strCarModelPath = strDefault;
}

bool CSetings::IsSupportDoor()
{
	return m_bDoorState;
}

bool CSetings::IsSupportCarPlate()
{
	return m_bCarPlateState;
}

std::string CSetings::GetPassword()
{
	return m_strPassword;
}

int CSetings::GetMucPortBaudRate(void)
{
	return m_mcuPortBaudRate;
}

void CSetings::SetMucPortBaudRate(int baud)
{
	if (baud != m_mcuPortBaudRate)
		change();
	m_mcuPortBaudRate = baud;
}

uint CSetings::GetBootDelay(void)
{
	return m_nBootDelay;
}

void CSetings::SetBootDelay(uint val)
{
	if (val != m_nBootDelay)
		change();
	m_nBootDelay = val;
}

bool CSetings::GetDoubleDelayEn()
{
	return m_bDoubleDelay;
}

void CSetings::SetDoubleDelayEn(bool en)
{
	if (en != m_bDoubleDelay)
	{
		// GCarMonitor.SetDoubleDelay(en);
		change();
	}
	m_bDoubleDelay = en;
}

void CSetings::InitTrack()
{
	// viewSetTrackEnable(1);
	// viewSetTrackAngleEnable(m_bTrackFollow);

	// viewSetTrackStyle(cTrackName[m_udtTrack.type], cTrackName[m_udtTrack.type]);

	// viewSetModelTrackEnable(MODEL_ID_CALI_FRONT, m_udtTrack.front);
	// viewSetModelTrackEnable(MODEL_ID_CALI_REAR, m_udtTrack.rear);
	// viewSetModelTrackEnable(MODEL_ID_FRONT, m_udtTrack.src_front);
	// viewSetModelTrackEnable(MODEL_ID_REAR, m_udtTrack.src_rear);
}

// void CSetings::SetTrackState(INITrack& track)
// {
// 	if (m_udtTrack != track)
// 	{
// 		viewSetModelTrackEnable(MODEL_ID_CALI_FRONT, track.front);
// 		viewSetModelTrackEnable(MODEL_ID_CALI_REAR, track.rear);
// 		viewSetModelTrackEnable(MODEL_ID_FRONT, track.src_front);
// 		viewSetModelTrackEnable(MODEL_ID_REAR, track.src_rear);

// 		change();
// 	}
// 	memcpy(&m_udtTrack, &track, sizeof(INITrack));
// }

// void CSetings::GetTrackState(INITrack& track)
// {
// 	memcpy(&track, &m_udtTrack, sizeof(INITrack));
// }

// void CSetings::SetTrackType(track_type type)
// {
// 	if (m_udtTrack.type != type)
// 	{
// 		viewSetTrackStyle(cTrackName[type], cTrackName[type]);
// 		change();
// 	}
// 	m_udtTrack.type = type;
// }

// track_type CSetings::GetTrackType()
// {
// 	return m_udtTrack.type;
// }

int CSetings::GetRemoteType()
{
	// return GiRemoteType;
}

void CSetings::SetRemoteType(int idx)
{
	// if (idx != GiRemoteType)
	// 	change();
	// GiRemoteType = idx;
}

void CSetings::SetDispRotateAngle(rotate_angle angle)
{
	if (angle != m_dispRotate)
		change();
	m_dispRotate = angle;
}

rotate_angle CSetings::GetDispRotateAngle()
{
	return m_dispRotate;
}

void CSetings::SetTSRotateAngle(rotate_angle angle)
{
	if (angle != m_tsRotate)
		change();
	m_tsRotate = angle;
}

// rotate_angle CSetings::GetTSRotateAngle()
// {
// 	return m_tsRotate;
// }

void CSetings::SetClockFreq(int val)
{
	if (val != m_nClockFreq)
		change();
	m_nClockFreq = val;
}

int CSetings::GetClockFreq()
{
	return m_nClockFreq;
}

void CSetings::SetClockFreqEn(int val)
{
	m_nClockFreqEn = val;
	change();
}

int CSetings::GetClockFreqEn()
{
	return m_nClockFreqEn;
}

bool CSetings::GetRearSwitch360()
{
	return m_bRearSwitch360;
}

void CSetings::SetRearSwitch360(bool en)
{
	if (m_bRearSwitch360 != en)
	{
		change();
		GCarMonitor.SetRearSwitch360(en);
	}
	m_bRearSwitch360 = en;
}

void CSetings::SetRightTriggerOriginal(bool en)
{
	if (en != m_bRightTrigger)
		change();
	m_bRightTrigger = en;
}

bool CSetings::GetRightTriggerOriginal()
{
	return m_bRightTrigger;
}

void CSetings::SetRadarTriggerDistance(int val)
{
	if (m_nRadarTriggerDistance != val)
	{
		GCarMonitor.SetRadarTriggerLevel(val, val, val, val);
		change();
	}
	m_nRadarTriggerDistance = val;
}

int CSetings::GetRadarTriggerDistance()
{
	return m_nRadarTriggerDistance;
}

void CSetings::SetDoubleDelayTime(uint val)
{
	if (val != m_nDoubleDelayTime)
	{
		GCarMonitor.SetDoubleDelayTime(val);
		change();
	}
	m_nDoubleDelayTime = val;
}

uint CSetings::GetDoubleDelayTime()
{
	return m_nDoubleDelayTime;
}

bool CSetings::GetRadar3D()
{
	return m_bRadar3D;
}

void CSetings::SetRadar3D(bool en)
{
	if (m_bRadar3D != en)
		change();

	m_bRadar3D = en;
}

int CSetings::GetRadar3DStyle()
{
	return m_nRadar3DStyle;
}

void CSetings::SetRadar3DStyle(int style)
{
	if (m_nRadar3DStyle != style)
		change();

	m_nRadar3DStyle = style;
}

void CSetings::SetSteerAngle(int val)
{
	if (m_nSteerAngle != val)
	{
		GCarMonitor.SetSteerAngle(val);
		change();
	}
	m_nSteerAngle = val;
}

int CSetings::GetSteerAngle()
{
	return m_nSteerAngle;
}

void CSetings::SetSteerAngleType(int val)
{
	if (m_nSteerAngleType != val)
		change();
	m_nSteerAngleType = val;
}

int CSetings::GetSteerAngleType()
{
	return m_nSteerAngleType;
}

int CSetings::GetLowSpeed()
{
	return m_nLowSpeed;
}

void CSetings::SetLowSpeed(int val)
{
	if (m_nLowSpeed != val)
	{
		GCarMonitor.SetLowSpeed(val);
		change();
	}

	m_nLowSpeed = val;
}

void CSetings::SetDualSwitch(bool en)
{
	if (m_bDualSwitch != en)
		change();
	m_bDualSwitch = en;
}

bool CSetings::GetDualSwitch()
{
	return m_bDualSwitch;
}

// ui_type CSetings::GetUIStyle()
// {
// 	return m_enUi;
// }

// void CSetings::SetUIStyle(ui_type type)
// {
// 	if (m_enUi == type)
// 		return;

// 	m_enUi = type;
// 	m_bChange = true;

// 	std::vector<std::string> vecCarModel = {
// 			"Default/A6L", "Audi/A6L", "Bmw/5Series", "Benz/E-Class", "Benz/E-Class", "Cadillac/XT5",
// 			"Lexus/NX","Lexus/NX" ,"Audi/A4","Audi/A6L","Volvo/S90","Ford/Mondeo","Benz/E-Class","Bmw/5Series","Bmw/5Series",
// 			"Ford/Mondeo","Ford/Mondeo","BYD/QinPlus","Audi/A5-2026","Toyota/Camry-2024",
// 	};

// 	std::string strCarModel;
// 	if (type < vecCarModel.size())
// 		strCarModel = vecCarModel[type];
// 	else
// 		strCarModel = "Default/A6L";

// // #if WIN32
// #if 0
// 	std::string strPath = "D:/carmodel/render/" + strCarModel;
// #else
// 	std::string strPath = "/usr/bvresource/model/" + strCarModel;
// #endif

// 	if (util::existFile(strPath))
// 		m_strCarModelPath = strCarModel;

// 	m_udtLogic.front = bvtap_none;
// 	m_udtLogic.rear = bvtap_none;
// 	m_udtLogic.left = bvlogic_2d;
// 	m_udtLogic.right = bvlogic_2d;

// 	InitUIStyle();

// 	if (type == ui_audiA6)
// 	{
// 		m_udtColor.hue = 125; m_udtColor.saturate = 23; m_udtColor.value = 9;
// 	}

// 	if (type == ui_newBmw || type == ui_bmw_id8)
// 		m_udtTrack.type = track_type4;

// 	char chText[128] = { 0 };
// 	std::string strType = uiStyleNames[type];
// 	if (type == ui_general)
// 		sprintf(chText, "cp -rf /usr/bv/ui/menu/play_icon /usr/bv/ui/");
// 	else if (type == ui_lexus_1280 || type == ui_lexus_1920)
// 		sprintf(chText, "cp -rf /usr/bv/ui/menu_lexus/play_icon /usr/bv/ui/");
// 	else if (type == ui_ford || type == ui_ford_half_1 || type == ui_ford_half_2)
// 		sprintf(chText, "cp -rf /usr/bv/ui/menu_ford/play_icon /usr/bv/ui/");
// 	else if (type == ui_bmw_id8)
// 		sprintf(chText, "cp -rf /usr/bv/ui/menu_bmw_id8/play_icon /usr/bv/ui/");
// 	else
// 		sprintf(chText, "cp -rf /usr/bv/ui/menu_%s/play_icon /usr/bv/ui/", strType.c_str());

// 	system(chText);

// 	if (type != ui_general)
// 	{
// 		if (m_enLan != lantype_cn && m_enLan != lantype_tw && m_enLan != lantype_en)
// 			SetLanType(lantype_en);
// 	}

// 	if (type == ui_bmw || type == ui_newBmw || type == ui_bmw_id8)
// 		SetRadar3D(true);
// 	else
// 		SetRadar3D(false);

// 	if (type == ui_bmw_id8)
// 		SetRadar3DStyle(radar3d_radar6644);
// 	else
// 		SetRadar3DStyle(radar3d_default);

// 	if (type == ui_byd)
// 		m_bBydStyle = true;
// 	else
// 		m_bBydStyle = false;

// 	if (type == ui_benz)
// 		m_udtTrack.type = track_type2;

// 	if (type == ui_audiA5L)
// 	{
// 		m_udtTrack.type = track_type5;
// 		m_dispRotate = rotate_90;
// 		m_tsRotate = rotate_90;
// 	}
// }

// void CSetings::InitUIStyle()
// {
// 	//??????????????1920x720????
// 	if (m_enUi == ui_benz || m_enUi == ui_bmw)
// 	{
// 		m_enStyle = resolution_1920x720;
// 		m_enAerialView = av_left;
// 	}

// 	if (m_enUi == ui_newBmw || m_enUi == ui_bmw_id8)
// 	{
// 		m_enStyle = resolution_1920x720;
// 		m_enAerialView = av_right;
// 	}

// 	if (m_enUi == ui_ford || m_enUi == ui_ford_half_1 || m_enUi == ui_ford_half_2)
// 		m_enStyle = resolution_1920x720;

// 	if (m_enUi == ui_lexus_1280)
// 		m_enStyle = resolution_1280x720;
// 	if (m_enUi == ui_lexus_1920)
// 		m_enStyle = resolution_1920x720;

// 	if (m_enUi == ui_audiA4 || m_enUi == ui_audiA6)
// 	{
// 		m_enStyle = resolution_1280x720;
// 		m_enAerialView = av_right;
// 		m_dispRotate = rotate_0;
// 		m_tsRotate = rotate_0;
// 	}

// 	if (m_enUi == ui_volvo)
// 	{
// 		m_enStyle = resolution_1024x1364;
// 		m_enAerialView = av_right;
// 	}
	
// 	if (m_enUi != ui_general)
// 		m_nSteerAngle = 0;

// 	if (m_enUi == ui_audiA4 || m_enUi == ui_audiA6)
// 		m_nHyaline = 0;

// 	if (m_enUi == ui_byd)
// 		m_enStyle = resolution_1280x720;

// 	if (m_enUi == ui_audiA5L)
// 	{
// 		m_enStyle = resolution_1920x720;
// 		m_enAerialView = av_left;
// 	}

// 	if (m_enUi == ui_toyota)
// 		m_enStyle = resolution_1920x720;
// }

void CSetings::SetBYDDispReversal(bool en)
{
	if (en != m_bBYDDispReversal)
		change();
	m_bBYDDispReversal = en;
}

bool CSetings::GetBYDDispReversal()
{
	return m_bBYDDispReversal;
}

bool CSetings::GetBYDStyle()
{
	return m_bBydStyle;
}

void CSetings::SetFullRotate(bool en)
{
	if (m_bFullRotate != en)
		change();
	m_bFullRotate = en;
}

bool CSetings::GetFullRotate()
{
	return m_bFullRotate;
}

void CSetings::SetLedAnimation(bool en)
{
	if (m_bLedAnima != en)
		change();
	m_bLedAnima = en;
}

bool CSetings::GetLedAnimation()
{
	return m_bLedAnima;
}

void CSetings::checkParam()
{
	// m_nDefaultGamma = qBound(0, m_nDefaultGamma, 100);

	// m_udtDefCam1080.brightness = qBound(0, m_udtDefCam1080.brightness, 100);
	// m_udtDefCam1080.contrast = qBound(0, m_udtDefCam1080.contrast, 100);
	// m_udtDefCam1080.saturate = qBound(0, m_udtDefCam1080.saturate, 100);

	// m_udtDefCam720.brightness = qBound(0, m_udtDefCam720.brightness, 100);
	// m_udtDefCam720.contrast = qBound(0, m_udtDefCam720.contrast, 100);
	// m_udtDefCam720.saturate = qBound(0, m_udtDefCam720.saturate, 100);

	// m_nBootDelay = qBound(0, (int)m_nBootDelay, 100);

	bool bOk = false;
	util::string2number<int>(m_strPassword, &bOk);

	if (!bOk)
		m_strPassword = "8888";

	checkCarModelPath();

	// InitUIStyle();
}

void CSetings::initXml()
{
	std::string strPath;
	// if (m_enLan == lantype_cn)
	// 	strPath = "chineseSimplified.xml";
	// else if (m_enLan == lantype_tw)
	// 	strPath = "chineseTraditional.xml";
	// else if (m_enLan == lantype_ru)
	// 	strPath = "russian.xml";
	// else if (m_enLan == lantype_kr)
	// 	strPath = "korean.xml";
	// else if (m_enLan == lantype_jp)
	// 	strPath = "japanese.xml";
	// else if (m_enLan == lantype_vn)
	// 	strPath = "vietnamese.xml";
	// else if (m_enLan == lantype_ma)
	// 	strPath = "malaysia.xml";
	// else
	// 	strPath = "english.xml";

	// xmlText.createXML(strPath);
}

void CSetings::initLan()
{
	// if (!strcmp(m_chLan, "cn"))
	// 	m_enLan = lantype_cn;
	// else if (!strcmp(m_chLan, "tw"))
	// 	m_enLan = lantype_tw;
	// else if (!strcmp(m_chLan, "ru"))
	// 	m_enLan = lantype_ru;
	// else if (!strcmp(m_chLan, "jp"))
	// 	m_enLan = lantype_jp;
	// else if (!strcmp(m_chLan, "kr"))
	// 	m_enLan = lantype_kr;
	// else if (!strcmp(m_chLan, "vn"))
	// 	m_enLan = lantype_vn;
	// else if (!strcmp(m_chLan, "ma"))
	// 	m_enLan = lantype_ma;
	// else
	// {
	// 	strcpy(m_chLan, "en");
	// 	m_enLan = lantype_en;
	// }
}

/*******************  ???????  *********************/
ViewParam& ViewParam::instance()
{
	static ViewParam _instance;
	return _instance;
}

ViewParam::ViewParam()
{
	m_bChange = false;

	// std::string strPath = BV_PATH + "view";

	int nCamSize = GCSystemConf.GetCameraSize();

	// if (nCamSize == CAMERA_SIZE_1080)
	// 	strPath += "/view.sub.1080.ini";
	// else
	// 	strPath += "/view.sub.ini";

	// CSimpleIni ini;
	// SI_Error ret = ini.LoadFile(strPath.c_str());
	// if (ret != SI_OK)
	// 	return;

	// std::list<CSimpleIniA::Entry> lstSection;
	// ini.GetAllSections(lstSection);

	// if (lstSection.empty())
	// 	return;

	auto mergemapCallback = [&](const char* sec, const char* key, INIModelMergeMap& mmm) {
		std::string strSec(sec);
		std::string strKey(key);

		mmm.id = util::string2number<int>(strSec.substr(5));

		for (int i = 0; i < 4; i++)
		{
			size_t pos = strKey.find_first_of(")");
			std::string str = strKey.substr(1, pos - 1);
			if (pos < strKey.length() - 1)
				strKey = strKey.substr(pos + 2);

			pos = str.find_first_of(",");
			// mmm.dst[i].x = util::string2number<float>(str.substr(0, pos));
			// mmm.dst[i].y = util::string2number<float>(str.substr(pos + 1));
		}
	};

	auto undistortCallback = [&](const char* sec, const char* key, INIModelUndistort& mu) {
		std::string strSec(sec);
		std::string strKey(key);

		mu.id = util::string2number<int>(strSec.substr(5));

		strKey.erase(0, 1);
		strKey.pop_back();

		std::vector<std::string> vecStr = util::splitString(strKey, ",");

		// for (int i = 0; i < vecStr.size(); i++)
		// {
		// 	mu.fov.val[i] = util::string2number<float>(vecStr[i]);
		// }
	};

	// for (const auto& sec : lstSection)
	// {
	// 	const char* chSec = sec.pItem;
	// 	std::string strSec = chSec;
	// 	size_t pos1 = strSec.find("model");
	// 	size_t pos2 = strSec.find("src");

	// 	if (pos1 != std::string::npos)
	// 	{
	// 		const char* chType = ini.GetValue(chSec, "type");
	// 		if (!strcmp(chType, "undistort"))
	// 		{
	// 			const char* chKey = ini.GetValue(chSec, "fov");
	// 			INIModelUndistort model;
	// 			undistortCallback(chSec, chKey, model);
	// 			m_vecModelUndistort.push_back(std::move(model));
	// 		}
	// 		else if (!strcmp(chType, "mergemap"))
	// 		{
	// 			const char* chKey = ini.GetValue(chSec, "dstpoints");
	// 			INIModelMergeMap model;
	// 			mergemapCallback(chSec, chKey, model);
	// 			m_vecModelMergeMap.push_back(std::move(model));
	// 		}
	// 	}
	// 	else if (pos2 != std::string::npos)
	// 	{
	// 		BvRect rc;
	// 		rc.strToRect(ini.GetValue(chSec, "rect"));

	// 		INISrcView src;
	// 		src.x = rc.x();
	// 		src.y = rc.y();
	// 		src.width = rc.width();
	// 		src.height = rc.height();
	// 		src.id = ini.GetLongValue(chSec, "id");
	// 		src.mirror = ini.GetLongValue(chSec, "mirror");
	// 		src.rotangle = ini.GetLongValue(chSec, "rotangle");
	// 		m_vecSrcView.push_back(src);
	// 	}
	// }
}

void ViewParam::SetSrcRect(int idx, INISrcView& src)
{
	bool bExits = false;
	for (auto& val : m_vecSrcView)
	{
		if (val.id == idx)
		{
			bExits = true;
			memcpy(&val, &src, sizeof(INISrcView));
			break;
		}
	}

	if (!bExits)
		m_vecSrcView.push_back(src);

	m_bChange = true;
}

void ViewParam::GetSrcRect(int idx, INISrcView& src)
{
	bool bExits = false;
	for (auto& val : m_vecSrcView)
	{
		if (val.id == idx)
		{
			bExits = true;
			memcpy(&src, &val, sizeof(INISrcView));
			break;
		}
	}

	if (!bExits)
	{
		for (auto& val : m_vecDefSrcView)
		{
			if (val.id == idx)
			{
				memcpy(&src, &val, sizeof(INISrcView));
				break;
			}
		}
	}
}

int ViewParam::GetModelRects(int id, std::vector<INIViewModelRect>& vec, int type)
{
	// if (type == model_typeid_mergemap)
	// {
	// 	if (m_mapViewModel.count(id) > 0)
	// 		vec = m_mapViewModel[id];
	// 	else
	// 		return -1;
	// }
	// else if (type == model_typeid_undistort)
	// {
	// 	if (m_mapViewModel.count(id + 1000) > 0)
	// 		vec = m_mapViewModel[id + 1000];
	// 	else
	// 	{
	// 		if (m_mapViewModel.count(id) > 0)
	// 			vec = m_mapViewModel[id];
	// 		else
	// 			return -1;
	// 	}
	// }

	if (m_mapViewComplexModel.count(id) > 0)
		return 1;

	return 0;
}

void ViewParam::SetModelMergeMap(int id, INIModelMergeMap& model)
{
	bool bExits = false;
	for (auto& val : m_vecModelMergeMap)
	{
		if (id == val.id)
		{
			bExits = true;
			memcpy(&val, &model, sizeof(INIModelMergeMap));
			break;
		}
	}

	if (!bExits)
	{
		m_vecModelMergeMap.push_back(model);
	}

	m_bChange = true;
}

void ViewParam::GetModelMergeMap(int id, INIModelMergeMap& model)
{
	bool bExits = false;

	for (auto& val : m_vecModelMergeMap)
	{
		if (val.id == id)
		{
			bExits = true;
			memcpy(&model, &val, sizeof(INIModelMergeMap));
			break;
		}
	}

	if (!bExits)
	{
		for (auto& val : m_vecDefModelMergeMap)
		{
			if (val.id == id)
			{
				memcpy(&model, &val, sizeof(INIModelMergeMap));
				break;
			}
		}
	}
}

void ViewParam::GetModelMergeMapDefault(int id, INIModelMergeMap& model)
{
	for (auto& val : m_vecDefModelMergeMap)
	{
		if (val.id == id)
		{
			memcpy(&model, &val, sizeof(INIModelMergeMap));
			break;
		}
	}
}

void ViewParam::SetModelUndistort(int id, INIModelUndistort& model)
{
	bool bExits = false;
	for (auto& val : m_vecModelUndistort)
	{
		if (id == val.id)
		{
			bExits = true;
			memcpy(&val, &model, sizeof(INIModelUndistort));
			break;
		}
	}

	if (!bExits)
		m_vecModelUndistort.push_back(model);

	m_bChange = true;
}

void ViewParam::GetModelUndistort(int id, INIModelUndistort& model)
{
	bool bExits = false;

	for (auto& val : m_vecModelUndistort)
	{
		if (val.id == id)
		{
			bExits = true;
			memcpy(&model, &val, sizeof(INIModelUndistort));
			break;
		}
	}

	if (!bExits)
	{
		for (auto& val : m_vecDefModelUndistort)
		{
			if (val.id == id)
			{
				memcpy(&model, &val, sizeof(INIModelUndistort));
				break;
			}
		}
	}
}

void ViewParam::GetModelUndistortDefault(int id, INIModelUndistort& model)
{
	for (auto& val : m_vecDefModelUndistort)
	{
		if (val.id == id)
		{
			memcpy(&model, &val, sizeof(INIModelUndistort));
			break;
		}
	}
}

void ViewParam::InitSrcView()
{
	// CvRect dstRC;
	// if (GCSystemConf.GetCameraSize() == CAMERA_SIZE_1080)
	// 	dstRC = cvRect(0, 0, 1920, 1080);
	// else
	// 	dstRC = cvRect(0, 0, 1280, 720);

	// std::vector<int> vecID = {
	// 	MODEL_ID_FRONT,
	// 	MODEL_ID_REAR,
	// 	MODEL_ID_LEFT,
	// 	MODEL_ID_RIGHT,
	// };

	// for (auto& val : vecID)
	// {
	// 	CvRect srcRC;

	// 	INISrcView ini;
	// 	ini.id = val;

	// 	viewGetModeSrcParam(val, &srcRC, &dstRC, &ini.mirror, &ini.rotangle);

	// 	ini.x = srcRC.x;
	// 	ini.y = srcRC.y;
	// 	ini.width = srcRC.width;
	// 	ini.height = srcRC.height;

	// 	m_vecDefSrcView.push_back(ini);
	// }

	// for (auto& ini : m_vecSrcView)
	// {
	// 	int id = ini.id;
	// 	CvRect srcRC = cvRect(ini.x, ini.y, ini.width, ini.height);
	// 	viewSetModeSrcParam(id, srcRC, dstRC, ini.mirror, ini.rotangle);
	// }
}

void ViewParam::InitModelView()
{
	int iCam;
	// CvRect dstRect;
	// CvPoint2D32f ptMerges[4];
	// CvPoint2D32f ptDsts[4];

	// CvScalar fov;
	// CvSize size;
	int iIsAutoCorrect;

	// for (auto& ini : m_vecModelMergeMap)
	// {
	// 	viewGetModelMergeMapParam(ini.id, iCam, ptMerges, ptDsts, dstRect);

	// 	memcpy(ptDsts, ini.dst, sizeof(CvPoint2D32f) * 4);

	// 	viewSetModelMergeMapParam(ini.id, ptMerges, ptDsts);
	// }

	// for (auto& ini : m_vecModelUndistort)
	// {
	// 	viewGetModelUndistortParam(ini.id, iCam, fov, size, iIsAutoCorrect, dstRect);

	// 	memcpy(&fov, &ini.fov, sizeof(CvScalar));

	// 	viewSetModelUndistortParam(ini.id, fov);
	// }
}

void ViewParam::AddDefModelMergeMap(int modelID)
{
	bool bExits = false;
	for (auto& val : m_vecDefModelMergeMap)
	{
		if (val.id == modelID)
		{
			bExits = true;
			break;
		}
	}

	if (bExits)
		return;

	int iCam;
	// CvRect dstRect;
	// CvPoint2D32f ptMerges[4];
	// CvPoint2D32f ptDsts[4];

	INIModelMergeMap p;
	p.id = modelID;

	// viewGetModelMergeMapParam(modelID, iCam, ptMerges, ptDsts, dstRect);
	// memcpy(p.dst, ptDsts, sizeof(CvPoint2D32f) * 4);
	m_vecDefModelMergeMap.push_back(p);
}

void ViewParam::AddDefModelUndistort(int modelID)
{
	bool bExits = false;
	for (auto& val : m_vecDefModelUndistort)
	{
		if (val.id == modelID)
		{
			bExits = true;
			break;
		}
	}

	if (bExits)
		return;

	int iCam;
	// CvRect dstRect;
	// CvSize size;
	int iIsAutoCorrect;

	INIModelUndistort p;
	p.id = modelID;
	// viewGetModelUndistortParam(modelID, iCam, p.fov, size, iIsAutoCorrect, dstRect);

	m_vecDefModelUndistort.push_back(p);
}

void ViewParam::WriteFile()
{
	if (!m_bChange)
		return;

	m_bChange = false;

	// char pcFileBuf[FILE_BUFFER_SIZE] = { 0 };

	char str[512] = { 0 };
	char text[512] = { 0 };

	for (auto& ini : m_vecSrcView)
	{
		// BvRect rc(ini.x, ini.y, ini.width, ini.height);
		// std::string strRc = rc.rectToStr();

		// sprintf(str, "[src%d]\r\nid=%d\r\nrect=%s\r\nmirror=%d\r\nrotangle=%d\r\n\r\n", ini.id, ini.id, strRc.c_str(), ini.mirror, ini.rotangle);
		// strcat(pcFileBuf, str);
	}

	for (auto& ini : m_vecModelMergeMap)
	{
		char chText[128];
		// sprintf(chText, "(%.4f,%.4f),(%.4f,%.4f),(%.4f,%.4f),(%.4f,%.4f)", ini.dst[0].x, ini.dst[0].y, ini.dst[1].x, ini.dst[1].y
		// 	, ini.dst[2].x, ini.dst[2].y, ini.dst[3].x, ini.dst[3].y);

		sprintf(str, "[model%d]\r\ntype=mergemap\r\ndstpoints=%s\r\n\r\n", ini.id, chText);
		// strcat(pcFileBuf, str);
	}

	for (auto& ini : m_vecModelUndistort)
	{
		char chText[128];
		// sprintf(chText, "(%.4f,%.4f,%.4f,%.4f)", ini.fov.val[0], ini.fov.val[1], ini.fov.val[2], ini.fov.val[3]);

		sprintf(str, "[model%d]\r\ntype=undistort\r\nfov=%s\r\n\r\n", ini.id, chText);
		// strcat(pcFileBuf, str);
	}

	// std::string strPath = BV_PATH + "view";
	int nCamSize = GCSystemConf.GetCameraSize();

	// if (nCamSize == CAMERA_SIZE_1080)
	// 	strPath += "/view.sub.1080.ini";
	// else
	// 	strPath += "/view.sub.ini";

	// CMyFile file;
	// file.open(strPath.c_str(), "wb");
	// file.write(pcFileBuf, strlen(pcFileBuf));
	// file.close();
}

void ViewParam::Init()
{
	InitSrcView();
	InitModelView();
}

void ViewParam::RegisterViewID(const std::vector<std::pair<int, std::string>>& vec)
{
	for (int i = 0; i < vec.size(); i++)
	{
		int viewID = vec[i].first;
		// if (!viewIsRenderIdValid(viewID))
		// 	continue;

		// ModelsInView models[10];
		// int cnt = viewGetModelsInView(viewID, models, 10);
		// if (cnt == 0)
		// 	continue;

		std::vector<INIViewModelRect> vecRc;
		// for (int j = 0; j < cnt; j++)
		// {
		// 	int modelType = models[j].iModelType;
		// 	int modelID = models[j].iModelId;

		// 	if (modelType != model_typeid_mergemap && modelType != model_typeid_undistort)
		// 		continue;

		// 	INIViewModelRect udtRc;
		// 	udtRc.rc = models[j].DstRect;
		// 	udtRc.model = modelID;
		// 	udtRc.type = modelType;
		// 	vecRc.push_back(udtRc);

		// 	if (modelType == model_typeid_mergemap)
		// 		AddDefModelMergeMap(modelID);
		// 	else if (modelType == model_typeid_undistort)
		// 		AddDefModelUndistort(modelID);
		// }

		bool b1 = false;
		bool b2 = false;
		// for (auto& val : vecRc)
		// {
		// 	if (val.type == model_typeid_mergemap)
		// 		b1 = true;
		// 	if (val.type == model_typeid_undistort)
		// 		b2 = true;
		// }

		if (b1 && b2)
			continue;

		m_mapViewModel[viewID] = vecRc;
		if (b1)
			m_mapModelMergeMap[viewID] = vec[i].second;
		if (b2)
			m_mapModelUndistort[viewID] = vec[i].second;
	}
}

void ViewParam::RegisterViewIDMoreModel(int viewID, const std::string& str, const std::vector<int>& vecModel, bool bHold /*= false*/)
{
	// if (!viewIsRenderIdValid(viewID))
	// 	return;

	// ModelsInView models[10];
	// int cnt = viewGetModelsInView(viewID, models, 10);

	int valMergemap = 0;
	int valUndistort = 0;
	std::vector<INIViewModelRect> vecViewModelRc;
	// for (int i = 0; i < cnt; i++)
	// {
	// 	ModelsInView udtModel = models[i];

	// 	int modelType = udtModel.iModelType;
	// 	int modelID = udtModel.iModelId;

	// 	if (modelType != model_typeid_mergemap && modelType != model_typeid_undistort)
	// 		continue;

	// 	if (modelType == model_typeid_mergemap)
	// 		valMergemap = 1;
	// 	else if (modelType == model_typeid_undistort)
	// 		valUndistort = 2;

	// 	auto it = std::find(vecModel.begin(), vecModel.end(), modelID);
	// 	if (it == vecModel.end())
	// 		continue;

	// 	INIViewModelRect udtRc;
	// 	udtRc.rc = udtModel.DstRect;
	// 	udtRc.model = modelID;
	// 	udtRc.type = modelType;

	// 	vecViewModelRc.push_back(udtRc);

	// 	if (modelType == model_typeid_mergemap)
	// 		AddDefModelMergeMap(modelID);
	// 	else if (modelType == model_typeid_undistort)
	// 		AddDefModelUndistort(modelID);
	// }

	//??????
	int valType = valMergemap | valUndistort;
	if (valType == 3)
		m_mapViewComplexModel[viewID] = true;
	else
		m_mapViewComplexModel[viewID] = bHold;

	std::vector<INIViewModelRect> vecMergeMap;
	std::vector<INIViewModelRect> vecUndistort;
	// for (auto& val : vecViewModelRc)
	// {
	// 	if (val.type == model_typeid_mergemap)
	// 		vecMergeMap.push_back(val);
	// 	if (val.type == model_typeid_undistort)
	// 		vecUndistort.push_back(val);
	// }

	bool b1 = vecMergeMap.empty();
	bool b2 = vecUndistort.empty();
	if (!b1 && b2)
	{
		m_mapViewModel[viewID] = vecViewModelRc;
		m_mapModelMergeMap[viewID] = str;
	}
	else if (b1 && !b2)
	{
		m_mapViewModel[viewID] = vecViewModelRc;
		m_mapModelUndistort[viewID] = str;
	}
	else
	{
		m_mapViewModel[viewID] = vecMergeMap;
		m_mapViewModel[viewID + 1000] = vecUndistort;

		m_mapModelMergeMap[viewID] = str;
		m_mapModelUndistort[viewID] = str;
	}
}

// void ViewParam::RegisterModelMergeMap(int viewID, int modelID, const std::string& str, const BvRect rc)
// {
// 	std::vector<INIViewModelRect> vecViewModelRc;

// 	INIViewModelRect udtRc;
// 	udtRc.rc = cvRect(rc.x(), rc.y(), rc.width(), rc.height());
// 	udtRc.model = modelID;
// 	udtRc.type = model_typeid_mergemap;

// 	AddDefModelMergeMap(modelID);
// 	vecViewModelRc.push_back(udtRc);

// 	m_mapViewModel[viewID] = vecViewModelRc;
// 	m_mapModelMergeMap[viewID] = str;
// }

// void ViewParam::RegisterModelUndistort(int viewID, int modelID, const std::string& str, const BvRect rc)
// {
// 	std::vector<INIViewModelRect> vecViewModelRc;

// 	INIViewModelRect udtRc;
// 	udtRc.rc = cvRect(rc.x(), rc.y(), rc.width(), rc.height());
// 	udtRc.model = modelID;
// 	udtRc.type = model_typeid_undistort;

// 	AddDefModelUndistort(modelID);
// 	vecViewModelRc.push_back(udtRc);

// 	m_mapViewModel[viewID] = vecViewModelRc;
// 	m_mapModelUndistort[viewID] = str;
// }

void ViewParam::GetMergeMapStr(std::vector<int>& vec, std::vector<std::string>& vecStr)
{
	for (auto& val : m_mapModelMergeMap)
	{
		vec.push_back(val.first);
		vecStr.push_back(val.second);
	}
}

void ViewParam::GetUndistortStr(std::vector<int>& vec, std::vector<std::string>& vecStr)
{
	for (auto& val : m_mapModelUndistort)
	{
		vec.push_back(val.first);
		vecStr.push_back(val.second);
	}
}
