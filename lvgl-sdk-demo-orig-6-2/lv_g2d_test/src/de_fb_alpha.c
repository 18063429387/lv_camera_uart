#include "de_fb_alpha.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#define __user
#include "sunxi_display2.h"

/* Allwinner DE2.0 typically has up to 4 channels, each with up to 4 layers */
#define MAX_CHANNEL 4
#define MAX_LAYER   4

int de_fb_layer_enable_pixel_alpha(int fb_width, int fb_height)
{
    int dispfh, ret;
    int found = 0;
    unsigned long arg[6];

    dispfh = open("/dev/disp", O_RDWR);
    if(dispfh == -1) {
        printf("[de_fb_alpha] open /dev/disp failed: %s\n", strerror(errno));
        return -1;
    }

    for(unsigned int ch = 0; ch < MAX_CHANNEL && !found; ch++) {
        for(unsigned int lid = 0; lid < MAX_LAYER && !found; lid++) {
            struct disp_layer_config cfg;
            memset(&cfg, 0, sizeof(cfg));
            cfg.channel = ch;
            cfg.layer_id = lid;

            memset(arg, 0, sizeof(arg));
            arg[0] = 0; /* screen_id */
            arg[1] = (unsigned long)&cfg;
            arg[2] = 1; /* get one layer config */

            ret = ioctl(dispfh, DISP_LAYER_GET_CONFIG, (void *)arg);
            if(ret != 0) {
                continue;
            }

            if(!cfg.enable) {
                continue;
            }

            if(cfg.info.mode != LAYER_MODE_BUFFER) {
                continue;
            }

            int layer_w = (int)cfg.info.screen_win.width;
            int layer_h = (int)cfg.info.screen_win.height;

            if(layer_w == fb_width && layer_h == fb_height) {
                printf("[de_fb_alpha] Found FB DE layer at "
                       "channel=%u layer_id=%u "
                       "(alpha_mode=%u alpha_value=%u format=%u)\n",
                       ch, lid,
                       cfg.info.alpha_mode,
                       cfg.info.alpha_value,
                       cfg.info.fb.format);

                cfg.info.alpha_mode = 0;      /* pixel alpha */
                cfg.info.alpha_value = 0xff;
                cfg.info.fb.format = DISP_FORMAT_ARGB_8888;

                printf("[de_fb_alpha] Setting pixel alpha mode + ARGB8888 "
                       "for channel=%u layer_id=%u\n", ch, lid);

                memset(arg, 0, sizeof(arg));
                arg[0] = 0; /* screen_id */
                arg[1] = (unsigned long)&cfg;
                arg[2] = 1; /* set one layer config */

                ret = ioctl(dispfh, DISP_LAYER_SET_CONFIG, (void *)arg);
                if(ret != 0) {
                    printf("[de_fb_alpha] DISP_LAYER_SET_CONFIG failed: %s "
                           "(errno=%d)\n", strerror(errno), errno);
                    close(dispfh);
                    return -1;
                }

                printf("[de_fb_alpha] DE FB layer reconfigured for "
                       "pixel alpha successfully\n");
                found = 1;
            }
        }
    }

    close(dispfh);

    if(!found) {
        printf("[de_fb_alpha] WARNING: No DE layer matching "
               "framebuffer %dx%d found. Transparency may not work.\n",
               fb_width, fb_height);
        return -1;
    }

    return 0;
}
