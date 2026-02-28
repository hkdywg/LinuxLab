#include <signal.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <poll.h>
#include <linux/videodev2.h>
#include <time.h>

#include "drm.h"
#include "parameter_parser.h"

#define V4L2_PLANE_NUM  1
#define V4L2_BUFFER_NUM 3

struct setup config;

struct v4l2_dev {
    int fd;
    struct buffer buffers[V4L2_BUFFER_NUM];
};

struct buffer disp_buf[2];

static int frame_count = 0;
static struct timespec start;

void print_fps() 
{
    if(frame_count == 0)
        clock_gettime(CLOCK_MONOTONIC, &start);

    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    frame_count++;
    
    long nsec = (now.tv_sec - start.tv_sec) * 1000000000 + (now.tv_nsec - start.tv_nsec);
    double sec = nsec / 1e9;

    if(sec > 5.0) {
        int fps = (int)(frame_count / sec  + 0.5);
        printf("fps: %d\n", fps);
        frame_count = 0;
        start = now;
    }
}

static void v4l2_check_capability(int fd)
{
    struct v4l2_capability caps;
    int ret;

    memset(&caps, 0, sizeof(struct v4l2_capability));
    ret = ioctl(fd, VIDIOC_QUERYCAP, &caps);
    if(ret) {
        printf("VIDIOC_QUERYCAP failed\n");
    }
    if(!caps.capabilities & V4L2_CAP_VIDEO_CAPTURE_MPLANE)
        printf("video: multiplaner capture is not supported\n");
}

static int v4l2_set_format(int fd, struct setup *config)
{
    struct v4l2_format fmt;
    int ret;

    memset(&fmt, 0, sizeof(struct v4l2_format));
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    ret = ioctl(fd, VIDIOC_G_FMT, &fmt);
    if(ret <  0) {
        printf("VIDIOC_G_FMT failed\n");
        return -1;
    }

    fmt.fmt.pix.width = config->width;
    fmt.fmt.pix.height = config->height;
    fmt.fmt.pix.colorspace = V4L2_COLORSPACE_SRGB;
    fmt.fmt.pix.xfer_func = V4L2_XFER_FUNC_DEFAULT;
    fmt.fmt.pix.ycbcr_enc = V4L2_YCBCR_ENC_601;
    if(config->in_fourcc)
        fmt.fmt.pix.pixelformat = config->in_fourcc;

    ret = ioctl(fd, VIDIOC_S_FMT, &fmt);
    if(ret < 0) {
        printf("VIDIOC_S_FMT failed\n");
        return -1;
    }
    
    return 0;
}

static int v4l2_requeset_buffers(int fd, struct setup *config)
{
    int cell_width, cell_height;
    rga_buffer_handle_t
}
