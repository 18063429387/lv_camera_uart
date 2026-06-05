#include "camera_preview.h"
#include "uart_agreement/uart_agreement_c_api.h"
#include "../tinyxml2/examples/bvtext_c.h"

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
#include "../lv_port_linux/lvgl/demos/lv_demos.h"

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

    /* 启用 ARGB8888，否则所有 bg_opa 透明度设置都不会生效 */
    lv_display_t * disp = lv_display_get_default();
    if (disp) {
        lv_display_set_color_format(disp, LV_COLOR_FORMAT_ARGB8888);
    }

    return 0;
}

/* 递归设置对象及其所有子对象的背景透明 */
static void lv_obj_set_bg_transparent_recursive(lv_obj_t * obj)
{
    lv_obj_set_style_bg_opa(obj, LV_OPA_TRANSP, LV_PART_MAIN);
    uint32_t child_count = lv_obj_get_child_count(obj);
    for (uint32_t i = 0; i < child_count; i++) {
        lv_obj_t * child = lv_obj_get_child(obj, i);
        lv_obj_set_bg_transparent_recursive(child);
    }
}

static void lv_demos_init(void)
{
    /* 获取活动屏幕并设置背景半透明（可看到底层摄像头） */
    lv_obj_t * scr = lv_screen_active();
    lv_obj_set_style_bg_opa(scr, LV_OPA_10, LV_PART_MAIN);
    lv_obj_set_style_bg_color(scr, lv_color_black(), LV_PART_MAIN);

    /* 底层图层背景透明 */
    lv_obj_t * bottom_layer = lv_layer_bottom();
    lv_obj_set_style_bg_opa(bottom_layer, LV_OPA_TRANSP, LV_PART_MAIN);

    /* 启动 LVGL 官方 widgets demo */
    lv_demo_widgets();

    /* lv_demo_widgets 内部创建的 tabview/panel/label 等全部有主题背景色，
     * 递归将所有对象的背景设为透明，让底层的摄像头画面能透过来 */
    lv_obj_set_bg_transparent_recursive(scr);
}

#if 0
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
#if 0
    lv_obj_t * myBtn = lv_button_create(scr);
    lv_obj_set_size(myBtn, 120, 50);
    lv_obj_set_style_bg_color(myBtn, lv_color_hex(0xDD5A2C), LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(myBtn, lv_color_hex(0x42C724), LV_STATE_PRESSED);
    lv_obj_add_event_cb(myBtn, myBtn_event, LV_EVENT_CLICKED, NULL);

    lv_obj_t * label_btn = lv_label_create(myBtn);
    lv_obj_align(label_btn, LV_ALIGN_CENTER, 150, 0);
    lv_label_set_text(label_btn, "Test");
#endif
}
#endif

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

    lv_demos_init();

    if (camera_preview_init(&cam_cfg) != 0) {
        printf("camera_preview_init failed\n");
    }

    uart_agreement_register_callback(uart_agreement_rx_cb, NULL);
    if (uart_agreement_init() != 0) {
        printf("uart_agreement_init failed\n");
    } else if (uart_agreement_start() != 0) {
        printf("uart_agreement_start failed\n");
    }

    while (!g_exit_flag) {
        lv_timer_handler();
        camera_preview_poll(5);
        usleep(1000);
    }

    uart_agreement_stop();
    camera_preview_deinit();
    return 0;
}
