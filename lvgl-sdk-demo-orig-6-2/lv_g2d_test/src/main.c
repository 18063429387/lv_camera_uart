#include "camera_preview.h"
#include "de_fb_alpha.h"
#include "uart_agreement/uart_agreement_c_api.h"

#include <stdbool.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/fb.h>

#include "../lv_port_linux/lvgl/lvgl.h"
#include "../lv_port_linux/src/lib/driver_backends.h"
#include "../lv_port_linux/src/lib/simulator_settings.h"

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

static int lvgl_port_init(int *fb_w, int *fb_h)
{
    printf("[lvgl] lvgl_port_init: begin\n");

    int fbfd;
    struct fb_var_screeninfo vinfo;

    settings.window_width = 800;
    settings.window_height = 480;
    settings.maximize = false;
    settings.fullscreen = false;

    printf("[lvgl] settings: %dx%d fullscreen=%d maximize=%d\n",
           settings.window_width, settings.window_height,
           settings.fullscreen, settings.maximize);

    printf("[lvgl] lv_init...\n");
    lv_init();

    printf("[lvgl] driver_backends_register...\n");
    driver_backends_register();

    printf("[lvgl] init backend: FBDEV (env LV_LINUX_FBDEV_DEVICE=%s)\n",
           getenv("LV_LINUX_FBDEV_DEVICE") ? getenv("LV_LINUX_FBDEV_DEVICE") : "(null)");

    if (driver_backends_init_backend("FBDEV") != 0) {
        printf("[lvgl] FBDEV backend init failed\n");
        driver_backends_print_supported();
        return -1;
    }

    printf("[lvgl] FBDEV backend initialized\n");

    /* Read back framebuffer resolution for DE alpha fixup */
    fbfd = open("/dev/fb0", O_RDONLY);
    if (fbfd >= 0) {
        memset(&vinfo, 0, sizeof(vinfo));
        if (ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo) == 0) {
            *fb_w = (int)vinfo.xres;
            *fb_h = (int)vinfo.yres;
            printf("[lvgl] Framebuffer: %dx%d bpp=%d transp=(%d,%d,%d)\n",
                   *fb_w, *fb_h, (int)vinfo.bits_per_pixel,
                   (int)vinfo.transp.offset,
                   (int)vinfo.transp.length,
                   (int)vinfo.transp.msb_right);
        } else {
            *fb_w = 800;
            *fb_h = 480;
        }
        close(fbfd);
    } else {
        *fb_w = 800;
        *fb_h = 480;
    }

    return 0;
}

static void ui_init(void)
{
#if 0
    lv_obj_t *scr = lv_screen_active();
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x20242a), 0);
    // lv_obj_set_style_bg_opa(scr, LV_OPA_TRANSP, 0);
    // lv_obj_set_style_border_opa(scr, LV_OPA_TRANSP, 0);

    lv_obj_t *label = lv_label_create(scr);
    lv_label_set_text(label, "LVGL 9.5 + Camera + UART");
    lv_obj_set_style_text_color(label, lv_color_hex(0xffffff), 0);
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 36);

    lv_obj_t *btn = lv_button_create(scr);
    lv_obj_set_size(btn, 220, 72);
    lv_obj_center(btn);

    lv_obj_t *btn_label = lv_label_create(btn);
    lv_label_set_text(btn_label, "Hello LVGL");
    lv_obj_center(btn_label);
#else
    lv_obj_t *scr = lv_screen_active();
    lv_obj_remove_style_all(scr);
    lv_obj_set_style_bg_opa(scr, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_opa(scr, LV_OPA_TRANSP, 0);
    lv_obj_set_style_outline_opa(scr, LV_OPA_TRANSP, 0);
    lv_obj_set_style_shadow_opa(scr, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(scr, 0, 0);

    /* Make all LVGL display layers transparent for camera overlay */
    lv_obj_set_style_bg_opa(lv_layer_bottom(), LV_OPA_TRANSP, 0);
    lv_obj_set_style_bg_opa(lv_layer_top(), LV_OPA_TRANSP, 0);
    lv_obj_set_style_bg_opa(lv_layer_sys(), LV_OPA_TRANSP, 0);

    lv_obj_t *label = lv_label_create(scr);
    lv_label_set_text(label, "LVGL 9.5 + Camera + UART");
    lv_obj_set_style_text_color(label, lv_color_hex(0xffffff), 0);
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 36);

    lv_obj_t *btn = lv_button_create(scr);
    lv_obj_set_size(btn, 220, 72);
    lv_obj_center(btn);
    lv_obj_set_style_bg_opa(btn, LV_OPA_70, 0);

    lv_obj_t *btn_label = lv_label_create(btn);
    lv_label_set_text(btn_label, "Hello LVGL");
    lv_obj_center(btn_label);
#endif
}

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    printf("[app] main: start\n");

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
    printf("[app] signal handler installed\n");

    printf("[app] lvgl_port_init...\n");
    int fb_w, fb_h;
    if (lvgl_port_init(&fb_w, &fb_h) != 0) {
        printf("[app] lvgl_port_init failed, exit\n");
        return 1;
    }

    printf("[app] camera_preview_init...\n");
    lv_obj_t *scr = lv_screen_active();
    lv_obj_set_style_bg_opa(scr, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_opa(scr, LV_OPA_TRANSP, 0);
    lv_obj_set_style_outline_opa(scr, LV_OPA_TRANSP, 0);
    printf("[app] camera_preview_init done\n");

    /* Verify color format is ARGB8888 */
    {
        lv_display_t *disp = lv_display_get_default();
        lv_color_format_t cf = lv_display_get_color_format(disp);
        printf("[app] Display color format = 0x%02x, has_alpha = %d, LV_COLOR_DEPTH = %d\n",
               (unsigned)cf,
               (int)lv_color_format_has_alpha(cf),
               LV_COLOR_DEPTH);
        if(!lv_color_format_has_alpha(cf)) {
            printf("[app] WARNING: color format has no alpha!"
                   " Transparency requires ARGB8888 (0x%02x) format.\n",
                   (unsigned)LV_COLOR_FORMAT_ARGB8888);
        }
    }

    printf("[app] de_fb_layer_enable_pixel_alpha (%dx%d)...\n", fb_w, fb_h);
    if (de_fb_layer_enable_pixel_alpha(fb_w, fb_h) != 0) {
        printf("[app] DE FB alpha fixup failed -- continuing anyway\n");
    }

    printf("[app] ui_init...\n");
    ui_init();
    printf("[app] ui_init done\n");

    printf("[app] camera_preview_init...\n");
    if (camera_preview_init(&cam_cfg) != 0) {
        printf("[app] camera_preview_init failed\n");
    } else {
        printf("[app] camera_preview_init ok\n");
    }

    uart_agreement_register_callback(uart_agreement_rx_cb, NULL);
    printf("[app] uart_agreement_init...\n");
    if (uart_agreement_init() != 0) {
        printf("[app] uart_agreement_init failed\n");
    } else if (uart_agreement_start() != 0) {
        printf("[app] uart_agreement_start failed\n");
    } else {
        printf("[app] uart_agreement_start ok\n");
    }

    printf("[app] main loop enter\n");
    uint32_t loop_cnt = 0;
    while (!g_exit_flag) {
        uint32_t idle_ms = lv_timer_handler();
        camera_preview_poll(1);

        loop_cnt++;
        if ((loop_cnt % 1000U) == 0U) {
            printf("[app] alive: loop=%u idle_ms=%u\n", loop_cnt, idle_ms);
        }

        if (idle_ms > 5U) idle_ms = 5U;
        usleep(idle_ms * 1000U);
    }

    printf("[app] exit loop, deinit...\n");
    uart_agreement_stop();
    camera_preview_deinit();
    printf("[app] exit done\n");
    return 0;
}
