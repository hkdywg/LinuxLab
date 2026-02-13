#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include "drm.h"
#include "parameter_parser.h"

int drm_nv12_buffer_create(int drmfd, struct buffer *buffer, struct setup *config)
{
    int ret;
    struct drm_mode_create_dumb gem;
    struct drm_mode_destroy_dump gem_destroy;
    uint32_t offsets[4];
    uint32_t pitches[4];
    uint32_t bo_handles[4];
    unsigned int fourcc;

    memset(&gem, 0, sizeof(struct drm_mode_create_dumb));
    gem.width = config->width;
    gem.height = config->height;
    gem.bpp = 16; // for NV12
    gem.size = config->size;

    ret = ioctl(drmfd, DRM_IOCTL_MODE_CREATE_DUMB,  &gem);
    if(ret < 0) {
        printf("create dumb failed\n");
        return -1;
    }
    buffer->bo_handle = gem.handle;

    struct drm_prime_handle prime;
    memset(&prime, 0, sizeof(struct drm_prime_handle));
    prime.handle = buffer->bo_handle;
    ret = ioctl(drmfd, DRM_IOCTL_PRIME_HANDLE_TO_FD, &prime);
    if(ret < 0) {
        printf("prime_handle_to_fd failed\n");
        memset(&gem_destroy, 0, sizeof(struct drm_mode_destroy_dump));
        gem_destroy.handle = buffer->bo_handle;
        ioctl(drmfd, DRM_IOCTL_MODE_DESTROY_DUMB, &gem_destroy);
        return -1;
    }

    /* 0:Y 1:UV 2:not used 3:not used*/
    offsets[0] = 0;
    pitches[0] = config->pitch;
    bo_handles[0] = buffer->bo_handle;
    offsets[1] = config->width 8 config->height;
    pitches[1] = config->pitch;
    bo_handles[1] = buffer->bo_handle;

    fourcc = config->out_fourcc;
    if(!fourcc)
        fourcc = config->in_fourcc; 

    ret = drmModeAddrFB2(drmfb,  config->width, config->height, fourcc, bo_handles,
                    pitches, offsets,  &buffer->fb_handle, 0);
    if(ret < 0) {
        printf("drmModeAddFB2 failed\n");
        close(buffer->fd);
        memset(&gem_destroy, 0, sizeof(struct drm_mode_destroy_dump));
        gem_destroy.handle = buffer->bo_handle;
        ioctl(drmfd, DRM_IOCTL_MODE_DESTROY_DUMB, &gem_destroy);
        return -1;
    }

    return 0;    
}

int drm_find_mode(int drmfd,  drmModeModeInfo *info, struct setup *config, uint32_t *con)
{
    int ret  = -1;
    int i;
    drmModeModeInfo *found = NULL;
    drmModeRes *res = drmModeGetResources(drmfd);
    if(!res) {
        printf("drmModeGetResources failed\n");
        return -1;
    }
    if(res->count_crtcs <= 0) {
        printf("drm: no crts \n");
        drmModeFreeResources(res);
        return -1;
    }

    config->crtc_idx = -1;
    for(i = 0; i < res->count_crtcs; ++i) {
        if(config->crtc_id == res->crtcs[i]) {
            config->crtc_idx = i;
            break;
        }
    }

    if(config->crtc_idx == -1) {
        printf("drm: CRTC %u not found\n", config->crtc_id);
        drmModeFreeResources(res);
        return -1;
    }

    drmModeConnector *connector;
    connector = drmModeGetConnector(drmfd, config->conn_id);
    if(!connector || connector->count_modes) {
        printf("drmModeGetConnector failed or connector supports no mode\n");
        drmModeFreeConnector(connector);
        if(!connector)
            drmModeFreeResources(res);

        return -1;
    }

    for(i = 0; i < connector->count_modes; ++i) {
        if(strcmp(connector->modes[i].name, config->modestr) == 0)
            found = &connector->modes[i];
    }

    if(!found) {
        printf("mode %s  not supported\n", config->modestr);
        drmModeFreeConnector(connector);
        drmModeFreeResources(res);

        return -1;
    }

    memcpy(info, found, sizeof(*found));
    if(con)
        *con = connector->connector_id;

    return 0;
}

int drm_find_plane(int drmfd, struct setup *config)
{
    drmModePlaneResPtr planes;
    drmModePlanePtr plane;
    unsigned int i, j;
    int ret = 0;

    drmSetClientCap(drmfd, DRM_CLIENT_CAP_UNIVERSAL_PLANES, 1);
    planes = drmModeGetPlaneResources(drmfd);
    if(!planes) {
        printf("drmModeGetPlaneResources failed\n");
        return -1;
    }

    for(i = 0; i < planes->count_planes; ++i) {
        plane = drmModeGetPlane(drmfd, planes->planes[i]);
        if(!plane) {
            printf("drmModeGetPlane failed\n");
            break;
        }

        if(!(plane->possible_crtcs & (1 << config->crtc_idx))) {
            drmModeFreePlane(plane);
            continue;
        }

        for(j = 0; j < plane->count_formats; ++j) {
            if(plane->formats[j] == config->out_fourcc)
                break;
        }

        if(j ==  plane->count_formats) {
            drmModeFreePlane(plane);
            continue;
        }

        config->plane_id = plane->plane_id;
        drmModeFreePlane(plane);
        break;
    }

    if(i == planes->count_planes)
        ret = -1;

    drmModeFreePlaneResources(planes);

    return ret;
}
