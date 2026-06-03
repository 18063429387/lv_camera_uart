#ifndef DE_FB_ALPHA_H
#define DE_FB_ALPHA_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Reconfigure the Display Engine layer associated with /dev/fb0
 * to use pixel alpha mode (alpha_mode=0) and ARGB8888 format.
 *
 * Must be called AFTER the Linux framebuffer driver has created
 * the DE layer (i.e. after opening /dev/fb0 via LVGL's FBDEV backend).
 *
 * @param fb_width   Framebuffer width (from fb_var_screeninfo.xres)
 * @param fb_height  Framebuffer height (from fb_var_screeninfo.yres)
 * @return 0 on success, -1 if the layer could not be found or reconfigured
 */
int de_fb_layer_enable_pixel_alpha(int fb_width, int fb_height);

#ifdef __cplusplus
}
#endif

#endif /* DE_FB_ALPHA_H */
