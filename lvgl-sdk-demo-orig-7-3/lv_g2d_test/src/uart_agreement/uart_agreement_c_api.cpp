#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "uart_agreement_c_api.h"
#include <pthread.h>
#include "threadUart.h"
#include "CarStateMonitor.h"
#include "ExternalIoCtrl.h"


static pthread_t s_tid_com1;
static pthread_t s_tid_com2;
static pthread_t s_tid_ctrl;
static pthread_t s_tid_tx;
static pthread_t s_tid_routinue_main;
static pthread_t s_tid_routinue_ctrl;
static pthread_t s_tid_ext_io;

static int s_started = 0;
static uart_agreement_rx_callback_t s_rx_cb = 0;
static void *s_rx_user_data = 0;

#define DISP_CTRL "/sys/class/leds/disp-ctrl/brightness"

static void DisplayCtrl(int iIsOn)
{
	// static int init = 0;
	static int fd = -1;
	// static int cvbsFd = -1;
	static int32_t ScreenOn = -1;
	int ret;

	// if (init == 0) {
	// 	cvbsFd = open(CVBS_OUTPUT, O_WRONLY);
	// 	if (cvbsFd < 0) {
	// 		printf("open %s failed!", CVBS_OUTPUT);
	// 		// 此处不可以return
	// 	}
	// 	init = 1;
	// }

	if (fd < 0) {
		fd = open(DISP_CTRL, O_WRONLY);
		if (fd < 0) {
			printf("open %s failed!", DISP_CTRL);
			return;
		}
	}

	// if (GCSetings.GetCvbsOutputState() != output_always && cvbsFd > 0) {
	// 	ret = write(cvbsFd, iIsOn ? "1" : "0", 1);
	// 	if (ret != 1) {
	// 		printf("write %s failed!\n", CVBS_OUTPUT);
	// 	}
	// 	else {
	// 		printf("CVBS %s\n", iIsOn ? "enable" : "disable");
	// 	}
	// }

	ret = write(fd, iIsOn ? "1\n" : "0\n", sizeof("1\n"));
	if (ret != sizeof("1\n")) {
		printf("write %d to %s failed!\n", iIsOn, DISP_CTRL);
	}

	// static ui_type ui = GCSetings.GetUIStyle();

	if (ScreenOn != iIsOn) {

		// if(ui != ui_audiA4 && ui != ui_audiA6)
		// 	SendScreenOnOffRequest(iIsOn);

		time_t currentTime;
		struct tm* pCurrentTime;
		time(&currentTime);
		pCurrentTime = localtime(&currentTime);
		printf("%s, %s, %s\n", __FUNCTION__, iIsOn == 1 ? "ON" : "OFF", asctime(pCurrentTime));

		// uint8_t ledstatus;
		// uint8_t ledFlashTab[] = {
		// 	LED_FLASH_4_TIMES,	// CVBS
		// 	LED_FLASH_5_TIMES,	// AHD
		// 	LED_FLASH_5_TIMES,	// TVI
		// 	LED_FLASH_ONCE,		// VGA
		// 	LED_FLASH_TWICE,	// HDMI
		// 	LED_FLASH_3_TIMES,	// LVDS
		// 	LED_FLASH_6_TIMES,
		// };
		// ScreenOn = iIsOn;
		// if (iIsOn) {
		// 	ledstatus = (uint8_t)ledFlashTab[GCSystemConf.GetInterface() % (sizeof(ledFlashTab) / sizeof(ledFlashTab[0]))];
		// }
		// else {
		// 	ledstatus = GCarMonitor.AccOn() ? LED_FLASH_ON100MS_OFF3S : (GCarMonitor.IsStartedParkMonitor() ? LED_FLASH_ON1S_OFF2S : LED_FLASH_ON1S_OFF1S);
		// }
		// RedLedCtrlQueue.PostMsg((void*)&ledstatus);
	}
}

extern "C" int uart_agreement_register_callback(uart_agreement_rx_callback_t cb, void *user_data)
{
    s_rx_cb = cb;
    s_rx_user_data = user_data;
    return 0;
}

extern "C" void uart_agreement_internal_on_frame(uint8_t cmd, const uint8_t *data, uint8_t len)
{
    if (s_rx_cb != 0) {
        s_rx_cb(cmd, data, len, s_rx_user_data);
    }
}

extern "C" int uart_agreement_init(void)
{
    InitComPort();
    return 0;
}

extern "C" int uart_agreement_start(void)
{
    DisplayCtrl(0);
    if (s_started) {
        return 0;
    }

    if (pthread_create(&s_tid_com1, 0, (void *(*)(void *))threadAvmCom1, 0) != 0) {
        return -1;
    }

    // if (pthread_create(&s_tid_com2, 0, (void *(*)(void *))threadAvmCom2, 0) != 0) {
    //     pthread_cancel(s_tid_com1);
    //     pthread_join(s_tid_com1, 0);
    //     return -2;
    // }

    if (pthread_create(&s_tid_ctrl, 0, (void *(*)(void *))threadControl_Uart, 0) != 0) {
        // pthread_cancel(s_tid_com2);
        pthread_cancel(s_tid_com1);
        // pthread_join(s_tid_com2, 0);
        pthread_join(s_tid_com1, 0);
        return -3;
    }

    if (pthread_create(&s_tid_tx, 0, (void *(*)(void *))threadAvmMsgQueueTx, 0) != 0) {
        pthread_cancel(s_tid_ctrl);
        // pthread_cancel(s_tid_com2);
        pthread_cancel(s_tid_com1);
        pthread_join(s_tid_ctrl, 0);
        // pthread_join(s_tid_com2, 0);
        pthread_join(s_tid_com1, 0);
        return -4;
    }
    if (pthread_create(&s_tid_ext_io, 0,  threadExtIoCtrl, 0) != 0) {
        printf("pthread_create threadExtIoCtrl failed!\n");
    }

    GCarMonitor.Init(DisplayCtrl);
    if (pthread_create(&s_tid_routinue_main, 0,  threadRoutinueMain, 0) != 0) {
        printf("pthread_create threadRoutinueMain failed!\n");
    }

    if (pthread_create(&s_tid_routinue_ctrl, 0,  threadRoutineOnOff, 0) != 0) {
        printf("pthread_create threadRoutineOnOff failed!\n");
    }

    s_started = 1;
    return 0;
}

extern "C" int uart_agreement_stop(void)
{
    if (!s_started) {
        return 0;
    }

    pthread_cancel(s_tid_ext_io);
    pthread_cancel(s_tid_routinue_ctrl);
    pthread_cancel(s_tid_routinue_main);

    pthread_cancel(s_tid_tx);
    pthread_cancel(s_tid_ctrl);
    // pthread_cancel(s_tid_com2);
    pthread_cancel(s_tid_com1);


    pthread_join(s_tid_ext_io, 0);
    pthread_join(s_tid_routinue_ctrl, 0);
    pthread_join(s_tid_routinue_main, 0);
    pthread_join(s_tid_tx, 0);
    pthread_join(s_tid_ctrl, 0);
    // pthread_join(s_tid_com2, 0);
    pthread_join(s_tid_com1, 0);

    s_started = 0;
    return 0;
}
