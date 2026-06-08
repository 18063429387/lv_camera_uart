#include "camera_preview.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <assert.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <linux/fb.h>

#define __user

#include "sunxi_camera_v2.h"
#include "sunxi_display2.h"

#define CLEAR(x) (memset(&(x), 0, sizeof(x)))

typedef enum {
    TVD_PL_YUV420 = 0,
    TVD_MB_YUV420 = 1,
    TVD_PL_YUV422 = 2,
} TVD_FMT_T;

struct disp_screen {
    int x;
    int y;
    int w;
    int h;
};

struct test_layer_info {
	int screen_id;
	int layer_id;
	int mem_id;
	disp_layer_config layer_config;
	int addr_map;
	int width, height;/* screen size */
	int dispfh;/* device node handle */
	int fh;/* picture resource file handle */
	int mem;
	int clear;/* is clear layer */
	char filename[32];
	int full_screen;
	unsigned int pixformat;
	disp_output_type output_type;
};

struct tvd_dev {
    struct test_layer_info layer_info;
};

struct buffer {
    void *start[3];
    int length[3];
    int dmabuf_fd[3];
};

struct preview_ctx {
    int inited;
    int fd;
    int dev_id;
    int input_sel;
    int width;
    int height;
    int fps;
    int mode;
    int req_buf_count;

    unsigned int nplanes;
    unsigned int n_buffers;
    struct buffer *buffers;
    struct tvd_dev dev;
};

static struct preview_ctx g_ctx = {
    .fd = -1,
    .inited = 0,
};

static struct disp_screen get_disp_screen(int w1, int h1, int w2, int h2)
{
    struct disp_screen screen;
    float r1 = (float)w1 / (float)w2;
    float r2 = (float)h1 / (float)h2;

    if (r1 < r2) {
        screen.w = w2 * r1;
        screen.h = h2 * r1;
    } else {
        screen.w = w2 * r2;
        screen.h = h2 * r2;
    }

    screen.x = (w1 - screen.w) / 2;
    screen.y = (h1 - screen.h) / 2;
    return screen;
}

static int disp_init(struct preview_ctx *ctx)
{
    int fbfd;
    struct fb_var_screeninfo var;

    ctx->dev.layer_info.screen_id = 0;
    ctx->dev.layer_info.dispfh = open("/dev/disp", O_RDWR);
    if (ctx->dev.layer_info.dispfh == -1) {
        printf("open /dev/disp failed\n");
        return -1;
    }

    ctx->dev.layer_info.width = ctx->width;
    ctx->dev.layer_info.height = ctx->height;
    fbfd = open("/dev/fb0", O_RDONLY);
    if (fbfd >= 0) {
        CLEAR(var);
        if (ioctl(fbfd, FBIOGET_VSCREENINFO, &var) == 0) {
            ctx->dev.layer_info.width = var.xres;
            ctx->dev.layer_info.height = var.yres;
        }
        close(fbfd);
    }

    // ctx->dev.layer_info.output_type = DISP_OUTPUT_TYPE_LCD;
    ctx->dev.layer_info.output_type = DISP_OUTPUT_TYPE_HDMI;
    ctx->dev.layer_info.pixformat = TVD_PL_YUV420;

    ctx->dev.layer_info.layer_config.channel = 0;
    ctx->dev.layer_info.layer_config.layer_id = 0;
    ctx->dev.layer_info.layer_config.enable = 1;
    ctx->dev.layer_info.layer_config.info.mode = LAYER_MODE_BUFFER;
    ctx->dev.layer_info.layer_config.info.zorder = 0;
    ctx->dev.layer_info.layer_config.info.alpha_mode = 1;
    ctx->dev.layer_info.layer_config.info.alpha_value = 0xff;

    if (ctx->mode == 4) {
        ctx->dev.layer_info.layer_config.info.fb.format = DISP_FORMAT_YUV420_SP_UVUV;
    } else if (ctx->dev.layer_info.pixformat == TVD_PL_YUV420) {
        ctx->dev.layer_info.layer_config.info.fb.format = DISP_FORMAT_YUV420_P;
    } else {
        ctx->dev.layer_info.layer_config.info.fb.format = DISP_FORMAT_YUV422_SP_VUVU;
    }

    {
        struct disp_screen screen = get_disp_screen(ctx->dev.layer_info.width,
                                                    ctx->dev.layer_info.height,
                                                    ctx->width,
                                                    ctx->height);
        ctx->dev.layer_info.layer_config.info.screen_win.x = screen.x;
        ctx->dev.layer_info.layer_config.info.screen_win.y = screen.y;
        ctx->dev.layer_info.layer_config.info.screen_win.width = screen.w;
        ctx->dev.layer_info.layer_config.info.screen_win.height = screen.h;
    }

    return 0;
}

static int disp_set_addr(struct preview_ctx *ctx, struct v4l2_buffer *buf)
{
    unsigned long arg[6];

    ctx->dev.layer_info.layer_config.info.fb.size[0].width = ctx->width;
    ctx->dev.layer_info.layer_config.info.fb.size[0].height = ctx->height;
    ctx->dev.layer_info.layer_config.info.fb.crop.x = 0;
    ctx->dev.layer_info.layer_config.info.fb.crop.y = 0;
    ctx->dev.layer_info.layer_config.info.fb.crop.width = (unsigned long long)ctx->width << 32;
    ctx->dev.layer_info.layer_config.info.fb.crop.height = (unsigned long long)ctx->height << 32;

    if (ctx->nplanes == 1) {
        unsigned long long y_size = (unsigned long long)ctx->width * ctx->height;
        ctx->dev.layer_info.layer_config.info.fb.addr[0] = buf->m.planes[0].m.mem_offset;
        ctx->dev.layer_info.layer_config.info.fb.addr[1] = ctx->dev.layer_info.layer_config.info.fb.addr[0] + y_size;
        ctx->dev.layer_info.layer_config.info.fb.addr[2] = 0;
        ctx->dev.layer_info.layer_config.info.fb.size[1].width = ctx->width / 2;
        ctx->dev.layer_info.layer_config.info.fb.size[1].height = ctx->height / 2;
    } else {
        ctx->dev.layer_info.layer_config.info.fb.addr[0] = buf->m.planes[0].m.mem_offset;
        ctx->dev.layer_info.layer_config.info.fb.addr[1] = (ctx->nplanes > 1) ? buf->m.planes[1].m.mem_offset : 0;
        ctx->dev.layer_info.layer_config.info.fb.addr[2] = (ctx->nplanes > 2) ? buf->m.planes[2].m.mem_offset : 0;
        ctx->dev.layer_info.layer_config.info.fb.size[1].width = ctx->width / 2;
        ctx->dev.layer_info.layer_config.info.fb.size[1].height = ctx->height / 2;
        ctx->dev.layer_info.layer_config.info.fb.size[2].width = ctx->width / 2;
        ctx->dev.layer_info.layer_config.info.fb.size[2].height = ctx->height / 2;
    }

    arg[0] = ctx->dev.layer_info.screen_id;
    arg[1] = (unsigned long)&ctx->dev.layer_info.layer_config;
    arg[2] = 1;
    arg[3] = 0;
    if (ioctl(ctx->dev.layer_info.dispfh, DISP_LAYER_SET_CONFIG, (void *)arg) != 0) {
        printf("DISP_LAYER_SET_CONFIG failed\n");
        return -1;
    }

    return 0;
}

static void disp_disable(struct preview_ctx *ctx)
{
    unsigned long arg[6];
    struct disp_layer_config disp;

    if (ctx->dev.layer_info.dispfh < 0) {
        return;
    }

    memset(&disp, 0, sizeof(disp));
    disp.channel = ctx->dev.layer_info.layer_config.channel;
    disp.layer_id = ctx->dev.layer_info.layer_config.layer_id;
    disp.enable = 0;

    arg[0] = ctx->dev.layer_info.screen_id;
    arg[1] = (unsigned long)&disp;
    arg[2] = 1;
    arg[3] = 0;
    (void)ioctl(ctx->dev.layer_info.dispfh, DISP_LAYER_SET_CONFIG, (void *)arg);

    close(ctx->dev.layer_info.dispfh);
    ctx->dev.layer_info.dispfh = -1;
}

static int camera_init(struct preview_ctx *ctx)
{
    struct v4l2_input inp;
    struct v4l2_streamparm parms;
    struct v4l2_format fmt;
    char dev_name[32];

    snprintf(dev_name, sizeof(dev_name), "/dev/video%d", ctx->dev_id);
    ctx->fd = open(dev_name, O_RDWR | O_NONBLOCK, 0);
    if (ctx->fd < 0) {
        printf("open %s failed\n", dev_name);
        return -1;
    }

    inp.index = ctx->input_sel;
    if (ioctl(ctx->fd, VIDIOC_S_INPUT, &inp) < 0) {
        printf("VIDIOC_S_INPUT failed\n");
        return -1;
    }

    CLEAR(parms);
    parms.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    parms.parm.capture.timeperframe.numerator = 1;
    parms.parm.capture.timeperframe.denominator = ctx->fps;
    parms.parm.capture.capturemode = V4L2_MODE_VIDEO;
    if (ioctl(ctx->fd, VIDIOC_S_PARM, &parms) < 0) {
        printf("VIDIOC_S_PARM failed\n");
        return -1;
    }

    CLEAR(fmt);
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    fmt.fmt.pix_mp.width = ctx->width;
    fmt.fmt.pix_mp.height = ctx->height;
    fmt.fmt.pix_mp.field = V4L2_FIELD_NONE;

    switch (ctx->mode) {
    case 4:
        fmt.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_NV12;
        break;
    case 3:
        fmt.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_NV12M;
        break;
    case 1:
    default:
        fmt.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_YUV420M;
        break;
    }

    if (ioctl(ctx->fd, VIDIOC_S_FMT, &fmt) < 0) {
        printf("VIDIOC_S_FMT failed\n");
        return -1;
    }

    if (ioctl(ctx->fd, VIDIOC_G_FMT, &fmt) < 0) {
        printf("VIDIOC_G_FMT failed\n");
        return -1;
    }

    ctx->nplanes = fmt.fmt.pix_mp.num_planes;
    printf("camera fmt: %dx%d, planes=%u\n", fmt.fmt.pix_mp.width, fmt.fmt.pix_mp.height, ctx->nplanes);
    return 0;
}

static int req_frame_buffers(struct preview_ctx *ctx)
{
    unsigned int i;
    struct v4l2_requestbuffers req;

    CLEAR(req);
    req.count = ctx->req_buf_count;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    req.memory = V4L2_MEMORY_MMAP;

    if (ioctl(ctx->fd, VIDIOC_REQBUFS, &req) < 0) {
        printf("VIDIOC_REQBUFS failed\n");
        return -1;
    }

    ctx->buffers = calloc(req.count, sizeof(*ctx->buffers));
    if (!ctx->buffers) {
        return -1;
    }

    for (ctx->n_buffers = 0; ctx->n_buffers < req.count; ++ctx->n_buffers) {
        struct v4l2_buffer buf;

        CLEAR(buf);
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = ctx->n_buffers;
        buf.length = ctx->nplanes;
        buf.m.planes = calloc(ctx->nplanes, sizeof(struct v4l2_plane));
        if (!buf.m.planes) {
            return -1;
        }

        if (ioctl(ctx->fd, VIDIOC_QUERYBUF, &buf) < 0) {
            free(buf.m.planes);
            printf("VIDIOC_QUERYBUF failed\n");
            return -1;
        }

        for (i = 0; i < ctx->nplanes; i++) {
            ctx->buffers[ctx->n_buffers].length[i] = buf.m.planes[i].length;
            ctx->buffers[ctx->n_buffers].dmabuf_fd[i] = -1;
            ctx->buffers[ctx->n_buffers].start[i] = mmap(NULL,
                                                          buf.m.planes[i].length,
                                                          PROT_READ | PROT_WRITE,
                                                          MAP_SHARED,
                                                          ctx->fd,
                                                          buf.m.planes[i].m.mem_offset);
            if (ctx->buffers[ctx->n_buffers].start[i] == MAP_FAILED) {
                free(buf.m.planes);
                printf("mmap failed\n");
                return -1;
            }
        }

        free(buf.m.planes);
    }

    for (i = 0; i < ctx->n_buffers; ++i) {
        struct v4l2_buffer buf;

        CLEAR(buf);
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;
        buf.length = ctx->nplanes;
        buf.m.planes = calloc(ctx->nplanes, sizeof(struct v4l2_plane));
        if (!buf.m.planes) {
            return -1;
        }

        if (ioctl(ctx->fd, VIDIOC_QBUF, &buf) < 0) {
            free(buf.m.planes);
            printf("VIDIOC_QBUF failed\n");
            return -1;
        }
        free(buf.m.planes);
    }

    return 0;
}

static void free_frame_buffers(struct preview_ctx *ctx)
{
    unsigned int i, j;

    if (!ctx->buffers) {
        return;
    }

    for (i = 0; i < ctx->n_buffers; ++i) {
        for (j = 0; j < ctx->nplanes; j++) {
            if (ctx->buffers[i].start[j] && ctx->buffers[i].length[j] > 0) {
                munmap(ctx->buffers[i].start[j], ctx->buffers[i].length[j]);
            }
        }
    }

    free(ctx->buffers);
    ctx->buffers = NULL;
    ctx->n_buffers = 0;
}

int camera_preview_init(const camera_preview_cfg_t *cfg)
{
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;

    if (!cfg) {
        return -1;
    }
    if (g_ctx.inited) {
        return 0;
    }

    g_ctx.dev_id = cfg->dev_id;
    g_ctx.input_sel = cfg->input_sel;
    g_ctx.width = cfg->width;
    g_ctx.height = cfg->height;
    g_ctx.fps = cfg->fps > 0 ? cfg->fps : 30;
    g_ctx.mode = cfg->mode;
    g_ctx.req_buf_count = cfg->req_buf_count > 0 ? cfg->req_buf_count : 5;

    if (camera_init(&g_ctx) < 0) {
        goto fail;
    }
    if (req_frame_buffers(&g_ctx) < 0) {
        goto fail;
    }
    if (disp_init(&g_ctx) < 0) {
        goto fail;
    }

    if (ioctl(g_ctx.fd, VIDIOC_STREAMON, &type) < 0) {
        printf("VIDIOC_STREAMON failed\n");
        goto fail;
    }

    g_ctx.inited = 1;
    return 0;

fail:
    camera_preview_deinit();
    return -1;
}

int camera_preview_poll(int timeout_ms)
{
    struct v4l2_buffer buf;
    fd_set fds;
    struct timeval tv;
    int r;

    if (!g_ctx.inited) {
        return -1;
    }

    FD_ZERO(&fds);
    FD_SET(g_ctx.fd, &fds);

    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;

    r = select(g_ctx.fd + 1, &fds, NULL, NULL, &tv);
    if (r < 0) {
        if (errno == EINTR) {
            return 0;
        }
        return -1;
    }
    if (r == 0) {
        return 0;
    }

    CLEAR(buf);
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    buf.memory = V4L2_MEMORY_MMAP;
    buf.length = g_ctx.nplanes;
    buf.m.planes = calloc(g_ctx.nplanes, sizeof(struct v4l2_plane));
    if (!buf.m.planes) {
        return -1;
    }

    if (ioctl(g_ctx.fd, VIDIOC_DQBUF, &buf) < 0) {
        free(buf.m.planes);
        return -1;
    }

    assert(buf.index < g_ctx.n_buffers);
    (void)disp_set_addr(&g_ctx, &buf);

    if (ioctl(g_ctx.fd, VIDIOC_QBUF, &buf) < 0) {
        free(buf.m.planes);
        return -1;
    }

    free(buf.m.planes);
    return 1;
}

void camera_preview_deinit(void)
{
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;

    if (g_ctx.fd >= 0) {
        (void)ioctl(g_ctx.fd, VIDIOC_STREAMOFF, &type);
    }

    disp_disable(&g_ctx);
    free_frame_buffers(&g_ctx);

    if (g_ctx.fd >= 0) {
        close(g_ctx.fd);
        g_ctx.fd = -1;
    }

    memset(&g_ctx.dev, 0, sizeof(g_ctx.dev));
    g_ctx.nplanes = 0;
    g_ctx.inited = 0;
}
