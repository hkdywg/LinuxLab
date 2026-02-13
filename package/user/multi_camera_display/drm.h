#ifndef __DRM_H_
#define __DRM_H_

#include "util.h"
#include <libdrm/drm.h>
#include <libdrm/drm_mode.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

struct buffer {
    unsigned int bo_handle;
    unsigned int fb_handle;
    unsigned int fb_id;
    int fd;
};

int drm_nv12_buffer_create(int drmfd, struct buffer *buffer, struct setup *config);
int drm_find_mode(int drmfd,  drmModeModeInfo *info, struct setup *config, uint32_t *con);
int drm_find_plane(int drmfd, struct setup *config);

#endif
