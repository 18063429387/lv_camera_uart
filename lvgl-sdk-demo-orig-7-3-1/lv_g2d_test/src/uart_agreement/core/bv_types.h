#pragma once

#include <string>

#define TUNE_MAX_LEN                        24.0f
#define WIDGET_TS_TUNE_MAX_LEN              800.0f
#define TUNE_MAX_ANGLE                      3.1415926f
#define TS_TUNE_MAX_HEIGHT                  100.0f
#define M_PI								3.14159265358979323846

#define DELAY_MAX							999999999
#define CAMERA_SIZE_720						720
#define CAMERA_SIZE_1080					1080
#define FILE_BUFFER_SIZE					4096

#define DISP_PARAM_CUSTOM_ID				255
#define DISP_PARAM_CUSTOM_PARAM				"Custom"
#define Hyaline_Transmittance_Val			0.2f

//������Ⱦ��������Ϊ0��ͼƬ������Ϊ1
#define OPENGL_PICTURE						0

//�����������Ƿ�Ҫ����Ӧ�ֱ��ʽ�����	1:����Ӧ 
#define DISP_AUTO_WEIGET_STYLE				1

//���ͱ���Ϊͨ�ó��ͱ���ר�ó��ͱ�			0:ͨ�ó��ͱ�	1:ר�ó��ͱ�
#define VEHICLE_TABLE_TYPE					0

//�ֳ�ң��������
#define REMOTE_TYPE_SW					0
#define REMOTE_TYPE_XCP					1

#if (OPENGL_PICTURE == 0)
#undef OPENGL_PICTURE
#endif

#if WIN32
const std::string	UI_PATH = "ui/";
const std::string	BV_PATH = "./";
#else
const std::string	UI_PATH = "/usr/bv/ui/";
const std::string	BV_PATH = "/usr/bv/";
#endif

enum key_action {
	key_dumy,
	turn_left,
	turn_right,
	key_up,
	key_down,
	key_left,
	key_right,
	key_enter,
	key_menu,
	key_return,
};

//����������Ĳ����߼�����car_door�ĺ�������
enum carbody_data {
	car_none,
	car_led_left,
	car_led_right,
	car_led_doubleflash,
	car_remote,
	car_taps_R,
	car_taps_D,
	car_rot360,
	car_default,
	car_360_exit,
	car_narrow,
	car_limit,
	car_round,
	car_3d_mode,
	car_expand_front,
	car_expand_rear,
	car_zoom_front,
	car_zoom_rear,
	car_left_3d,
	car_right_3d,
	car_rot360_full,	//����һ�ܣ�������ģ��͸����
};

enum view_mode {
	viewmode_menu,
	viewmode_setting,
};

//����ѡ��
enum carid_type {
	carid_none,
	carid_blue,
	carid_new,
	carid_yellow_black,
	carid_black_white,
	carid_fv_white_black,
	carid_fv_yellow_black,
};

//���ҵƴ�����ͼ
enum bvlogic_type {
	bvlogic_none,
	bvlogic_2d,
	bvlogic_3d,
	bvlogic_narrow,
	bvlogic_src,
	bvlogic_front,
	bvlogic_animation,
};

//��λ������ͼ
enum bvtap_type {
	bvtap_none,
	bvtap_src,
	bvtap_wide,
	bvtap_u,
};

//����
enum lantype {
	lantype_cn,
	lantype_tw,
	lantype_en,
	lantype_kr,
	lantype_vn,
	lantype_ma,
	lantype_ru,
	lantype_jp,
};

//�켣����
enum track_type {
	track_off,
	track_static,
	track_type1,
	track_type2,
	track_type3,
	track_type4,
	track_type5,
};

//cvbs���Կ���
enum cvbs_setting {
	output_switch,
	output_always,
};

enum park_monitor_func {
	pm_func_hide,
	pm_func_open,
};

enum park_monitor_power {
	pm_power_core,
	pm_power_decoder,
};

enum park_monitor_state {
	pm_off,
	pm_on_once,
	pm_on_always,
};

//���ͼ
enum aerial_view {
	av_none,
	av_left,
	av_right,
};

//��Ļ��񣬺�����ʵ�ֵ�ʱ��ֻ�����Ҫʵ�ֵģ�����ʵ�ֵľͲ��������棬����1280x480
enum resolution_type {
	resolution_1280x720,
	resolution_1920x720,
	resolution_1024x1364,		//768x1024
	resolution_1080x1920,
};

//��������
enum disp_type {
	disp_cvbs,
	disp_ahd,
	disp_tvi,
	disp_vga,
	disp_hdmi,
	disp_lvds
};

// ����ͷ�ӿ�����
enum camera_interface {
	interface_ahd = 0,
	interface_tvi,
};

enum radar3d_style {
	radar3d_default = 0,
	radar3d_radar6644,
};

enum uart_define {
	uart_def_uart = 0,
	uart_def_key,
	uart_def_key_tx,
};

//��ǰ¼��״̬
enum record_state {
	record_none,
	record_null,
	record_recording,
	record_stop,
	record_error,
};

//��Ļ��ת
enum rotate_angle {
	rotate_0 = 0,
	rotate_90,
	rotate_180,
	rotate_270,
};

//��ǰʱ���ʽ
enum time_format {
	time_yyyy_MM_dd_hh_mm_ss_slash,			//yyyy/MM/dd hh:mm:ss
	time_yyyy_MM_dd_hh_mm_ss_bar,			//yyyy-MM-dd hh:mm:ss
};

//�������ͣ�һ���ǿ�����ͼ��һ���ǿ��ƿ��أ���Ҫ������
enum voice_type {
	voice_hyaline_on,
	voice_hyaline_off,
	voice_ambient_on,
	voice_ambient_off,
	voice_fragrance_on,
	voice_fragrance_off,
};

typedef struct {
	char    pressure;
	int     xpos;
	int     ypos;
}mouse_action;

typedef struct {
	uint    id;
	uint    lcd_dclk_freq;
	uint    lcd_x;
	uint    lcd_y;
	uint    lcd_vt;
	uint    lcd_ht;
	uint    lcd_vbp;
	uint    lcd_hbp;
	uint    lcd_vspw;
	uint    lcd_hspw;
	uint    lcd_hv_clk_phase;
	uint    lcd_hv_sync_polarity;
	uint	lcd_lvds_mode;
	uint	lcd_lvds_link;
	char    iface[64];
	char    param[64];
	// ע�⣺ ��Ϊ�����䶯���ݵ����������disp_priority֮ǰ���������disp_priority֮��
	uint    disp_priority;
	uint    lcd_fps;
	uint    enable;
}disp_param_t;

typedef struct local_time_t {
	uint year;
	uint month;
	uint day;
	uint hour;
	uint minute;
	uint second;

	bool operator!=(const local_time_t& rhs) const
	{
		return year != rhs.year || month != rhs.month || day != rhs.day || hour != rhs.hour || minute != rhs.minute || second != rhs.second;
	}

}local_time_t;

typedef struct {
	char cImgFile[256];
	char cConfigIni[256];
	char cCalibIni[256];
	char cBvIni[256];
	char c3DModelIni[256];
	char cViewIni[256];
	char cRunIni[256];
	char cShaderPath[256];
	char cUiPath[256];
	char cSaveImgPath[256];                         //  ����4��1ͼƬʱ��·������/mnt

	char cPlayApp[256];                             //  ���ų����ļ���

	int  iIsRecorder;                               //  �Ƿ�¼��

	int  iImgWidth;
	int  iImgHeight;

	int  iIsCmdControlMode;
}app_param_t;

#define CALIBRA_MIN_METER		  25
#define CALIBRA_MAX_METER		  300
#define CALIBRA_STATE_SUCCESS     0
#define CALIBRA_STATE_FAIL        1
#define CALIBRA_STATE_ING		  2
#define CALIBRA_STATE_ERR_INI     3
#define CALIBRA_STATE_ERR_SYNC    4
#define CALIBRA_STATE_END		  5
//�궨����
typedef struct {
	int state;
	int front;
	int rear;
	int left;
	int right;

}calibra_t;

typedef struct {
	int cloth;
	int width;
	int head;
	int tail;

	int front[8][2];
	int rear[8][2];
	int left[8][2];
	int right[8][2];

}calibra_anchor_point_t;

/********************************����ö�ٹ��ܣ�����������***************************************/
enum ui_type {
	ui_general,
	ui_audi,
	ui_bmw,
	ui_benz,
	ui_newC,
	ui_cadillac,
	ui_lexus_1280,
	ui_lexus_1920,
	ui_audiA4,
	ui_audiA6,
	ui_volvo,
	ui_ford,
	ui_newC_1364,
	ui_newBmw,
	ui_bmw_id8,
	ui_ford_half_1,
	ui_ford_half_2,
	ui_byd,
	ui_bmw_id8_840,
	ui_volvo_1160,
	ui_audiA5L,
	ui_toyota,
};