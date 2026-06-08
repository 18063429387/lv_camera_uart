#include "camera_preview.h"
#include "uart_agreement/uart_agreement_c_api.h"
#include "../tinyxml2/examples/bvtext_c.h"
#include "ipc_bridge.h"

#include <stdbool.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "../lv_port_linux/lvgl/lvgl.h"
#include "../lv_port_linux/src/lib/driver_backends.h"
#include "../lv_port_linux/src/lib/simulator_settings.h"
#include "../lv_port_linux/lvgl/src/libs/freetype/lv_freetype.h"

static volatile sig_atomic_t g_exit_flag = 0;

extern simulator_settings_t settings;

static void uart_agreement_rx_cb(uint8_t cmd,
                                 const uint8_t *data,
                                 uint8_t len,
                                 void *user_data)
{
    (void)cmd;
    (void)data;
    (void)len;
    (void)user_data;
}

static void app_sig_handler(int sig)
{
    (void)sig;
    g_exit_flag = 1;
}

static void install_app_signal_handler(void)
{
    signal(SIGINT, app_sig_handler);
    signal(SIGTERM, app_sig_handler);
    signal(SIGQUIT, app_sig_handler);
    signal(SIGHUP, app_sig_handler);
}

// 按钮回调
static void myBtn_event(lv_event_t * event)
{
    lv_obj_t * btn = lv_event_get_target(event);
    if (lv_event_get_code(event) == LV_EVENT_CLICKED) {
        static uint8_t cnt = 0;
        cnt++;
        ipc_bridge_send_to_biz_str("button_clicked");
        lv_obj_t * label = lv_obj_get_child(btn, 0);
        lv_label_set_text_fmt(label, "Button: %d", cnt);
    }
}

static int lvgl_port_init(void)
{
    settings.window_width = 800;
    settings.window_height = 480;
    settings.maximize = false;
    settings.fullscreen = false;
    lv_init();
    driver_backends_register();
    if (driver_backends_init_backend(NULL) != 0) {
        printf("LVGL display backend init failed\n");
        return -1;
    }

    return 0;
}

static void ui_init(void)
{
    /* 1. 获取默认显示设备并配置透明支持 */
    lv_display_t *disp = lv_display_get_default();
    lv_display_set_color_format(disp, LV_COLOR_FORMAT_ARGB8888); // 启用 Alpha 通道

    /* 2. 获取活动屏幕并设置其背景透明 */
    lv_obj_t *scr = lv_screen_active();
    lv_obj_set_style_bg_opa(scr, LV_OPA_TRANSP, LV_PART_MAIN);   // 屏幕背景完全透明

    /* 3. 将底层图层背景也设置为透明 */
    lv_obj_t *bottom_layer = lv_layer_bottom();
    lv_obj_set_style_bg_opa(bottom_layer, LV_OPA_TRANSP, LV_PART_MAIN);

    /* 4. 初始化 FreeType 并加载微软雅黑 TTF */
    lv_freetype_init(256);
    lv_font_t *cjk_font = lv_freetype_font_create(
        "/usr/res/font/msyh.ttf",
        LV_FREETYPE_FONT_RENDER_MODE_BITMAP,
        16,
        LV_FREETYPE_FONT_STYLE_NORMAL);

    if (!cjk_font) {
        printf("<ERROR> failed to load /usr/res/font/msyh.ttf\n");
    }

    /* 从.xml加载文本 */
    bvtext_load("chineseSimplified.xml");

    /*
        中文简体
        中文繁體
        English（英语）
        한국어（韩语 / 朝鲜语）
        Tiếng Việt（越南语）
        Bahasa Melayu（马来语）
        Русский язык（俄语）
        العربية（阿拉伯语） 
    */
    char cn_text[128];
    char tw_text[128];
    char en_text[128];
    char kr_text[128];
    char vi_text[128];
    char ms_text[128];
    char ru_text[128];
    char ar_text[128];
    bvtext_get_value("system/language-1", cn_text, sizeof(cn_text));
    bvtext_get_value("system/language-2", tw_text, sizeof(tw_text));
    bvtext_get_value("system/language-3", en_text, sizeof(en_text));
    bvtext_get_value("system/language-4", kr_text, sizeof(kr_text));
    bvtext_get_value("system/language-5", vi_text, sizeof(vi_text));
    bvtext_get_value("system/language-6", ms_text, sizeof(ms_text));
    bvtext_get_value("system/language-7", ru_text, sizeof(ru_text));
    bvtext_get_value("system/language-8", ar_text, sizeof(ar_text));

#define LANG_LABEL_Y_START  36
#define LANG_LABEL_Y_STEP   44

    lv_obj_t *label_cn = lv_label_create(scr);
    if (cjk_font) {
        lv_obj_set_style_text_font(label_cn, cjk_font, 0);
    }
    lv_label_set_text(label_cn, cn_text);
    lv_obj_set_style_text_color(label_cn, lv_color_hex(0xffffff), 0);
    lv_obj_align(label_cn, LV_ALIGN_TOP_MID, 0, LANG_LABEL_Y_START + 0 * LANG_LABEL_Y_STEP);

    lv_obj_t *label_tw = lv_label_create(scr);
    if (cjk_font) {
        lv_obj_set_style_text_font(label_tw, cjk_font, 0);
    }
    lv_label_set_text(label_tw, tw_text);
    lv_obj_set_style_text_color(label_tw, lv_color_hex(0xe8e8e8), 0);
    lv_obj_align(label_tw, LV_ALIGN_TOP_MID, 0, LANG_LABEL_Y_START + 1 * LANG_LABEL_Y_STEP);

    lv_obj_t *label_en = lv_label_create(scr);
    if (cjk_font) {
        lv_obj_set_style_text_font(label_en, cjk_font, 0);
    }
    lv_label_set_text(label_en, en_text);
    lv_obj_set_style_text_color(label_en, lv_color_hex(0xaaaaaa), 0);
    lv_obj_align(label_en, LV_ALIGN_TOP_MID, 0, LANG_LABEL_Y_START + 2 * LANG_LABEL_Y_STEP);

    lv_obj_t *label_kr = lv_label_create(scr);
    if (cjk_font) {
        lv_obj_set_style_text_font(label_kr, cjk_font, 0);
    }
    lv_label_set_text(label_kr, kr_text);
    lv_obj_set_style_text_color(label_kr, lv_color_hex(0xcccccc), 0);
    lv_obj_align(label_kr, LV_ALIGN_TOP_MID, 0, LANG_LABEL_Y_START + 3 * LANG_LABEL_Y_STEP);

    lv_obj_t *label_vi = lv_label_create(scr);
    if (cjk_font) {
        lv_obj_set_style_text_font(label_vi, cjk_font, 0);
    }
    lv_label_set_text(label_vi, vi_text);
    lv_obj_set_style_text_color(label_vi, lv_color_hex(0xbbbbbb), 0);
    lv_obj_align(label_vi, LV_ALIGN_TOP_MID, 0, LANG_LABEL_Y_START + 4 * LANG_LABEL_Y_STEP);

    lv_obj_t *label_ms = lv_label_create(scr);
    if (cjk_font) {
        lv_obj_set_style_text_font(label_ms, cjk_font, 0);
    }
    lv_label_set_text(label_ms, ms_text);
    lv_obj_set_style_text_color(label_ms, lv_color_hex(0xbbbbbb), 0);
    lv_obj_align(label_ms, LV_ALIGN_TOP_MID, 0, LANG_LABEL_Y_START + 5 * LANG_LABEL_Y_STEP);

    lv_obj_t *label_ru = lv_label_create(scr);
    if (cjk_font) {
        lv_obj_set_style_text_font(label_ru, cjk_font, 0);
    }
    lv_label_set_text(label_ru, ru_text);
    lv_obj_set_style_text_color(label_ru, lv_color_hex(0xbbbbbb), 0);
    lv_obj_align(label_ru, LV_ALIGN_TOP_MID, 0, LANG_LABEL_Y_START + 6 * LANG_LABEL_Y_STEP);

    lv_obj_t *label_ar = lv_label_create(scr);
    if (cjk_font) {
        lv_obj_set_style_text_font(label_ar, cjk_font, 0);
    }
    lv_label_set_text(label_ar, ar_text);
    lv_obj_set_style_text_color(label_ar, lv_color_hex(0xbbbbbb), 0);
    lv_obj_align(label_ar, LV_ALIGN_TOP_MID, 0, LANG_LABEL_Y_START + 7 * LANG_LABEL_Y_STEP);

    // 触摸测试
#if 1
    lv_obj_t * myBtn = lv_button_create(scr);
    lv_obj_set_size(myBtn, 120, 50);
    lv_obj_set_style_bg_color(myBtn, lv_color_hex(0xDD5A2C), LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(myBtn, lv_color_hex(0x42C724), LV_STATE_PRESSED);
    lv_obj_add_event_cb(myBtn, myBtn_event, LV_EVENT_CLICKED, NULL);

    lv_obj_t * label_btn = lv_label_create(myBtn);
    lv_obj_align(label_btn, LV_ALIGN_CENTER, 50, 0);
    lv_label_set_text(label_btn, "Test");
#endif
}

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    camera_preview_cfg_t cam_cfg = {
        .dev_id = 0,
        .input_sel = 0,
        .width = 1920,
        .height = 1080,
        .fps = 30,
        .mode = 4,
        .req_buf_count = 5,
    };

    install_app_signal_handler();

    if (lvgl_port_init() != 0) {
        return 1;
    }

    /* 注册触摸输入设备 */
    {
        lv_indev_t * indev = lv_indev_create();
        lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
        lv_indev_set_read_cb(indev, uart_ts_read);
    }

    ui_init();

    if (camera_preview_init(&cam_cfg) != 0) {
        printf("camera_preview_init failed\n");
    }

    uart_agreement_register_callback(uart_agreement_rx_cb, NULL);
    if (uart_agreement_init() != 0) {
        printf("uart_agreement_init failed\n");
    } else if (uart_agreement_start() != 0) {
        printf("uart_agreement_start failed\n");
    }

    /* 初始化 IPC 双通道 */
    if (ipc_bridge_init() != 0) {
        printf("ipc_bridge_init failed\n");
    }

    while (!g_exit_flag) {
        lv_timer_handler();
        camera_preview_poll(5);

        /* 轮询业务进程发来的消息 */
        char ipc_buf[512];
        int ipc_len = ipc_bridge_try_recv_from_biz(ipc_buf, sizeof(ipc_buf));
        if (ipc_len > 0) {
            printf("[IPC biz->ui] 收到 %d 字节: %s\n", ipc_len, ipc_buf);
            /* TODO: 根据消息内容更新 UI */
        }

        usleep(1000);
    }

    ipc_bridge_deinit();
    uart_agreement_stop();
    camera_preview_deinit();
    return 0;
}
