#ifndef CAMERA_PREVIEW_H
#define CAMERA_PREVIEW_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int dev_id;          /* /dev/videoX */
    int input_sel;       /* VIDIOC_S_INPUT */
    int width;
    int height;
    int fps;
    int mode;            /* 建议固定 4 -> V4L2_PIX_FMT_NV12 */
    int req_buf_count;   /* 建议 4~6 */
} camera_preview_cfg_t;

int camera_preview_init(const camera_preview_cfg_t *cfg);
int camera_preview_poll(int timeout_ms);
void camera_preview_deinit(void);

#ifdef __cplusplus
}
#endif

#endif /* CAMERA_PREVIEW_H */
