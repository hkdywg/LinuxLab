/*
* drm_debug_direct_show.c 
*   drm direct show for debug
*
*  (C) 2026.03.04 <hkdywg@163.com>
*
*  This program is free software; you can redistribute it and/r modify
*  it under the terms of the GNU General Public License version 2 as
*  published by the Free Software Foundation.
*
*
*/
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/workqueue.h>
#include <linux/dma-buf.h>

#include <drm/drm_atomic_uapi.h>
#include <drm/drm_prime.h>
#include <drm/drm_fb_helper.h>
#include <drm/drm_framebuffer.h>
#include <drm/drm_gem_cma_helper.h>
#include <drm/drm_gem_framebuffer_helper.h>
#include <drm/drm_fourcc.h>
#include <drm/drm_modeset_helper.h>
#include "drm_debug.h"

uint32_t drm_get_bpp(const struct drm_format_info *info)
{
    if (!info || !info->cpp[0])
        return 0;

    switch (info->format) {
    case DRM_FORMAT_YUV420_8BIT:
        return 12;
    case DRM_FORMAT_YUV420_10BIT:
        return 15;
    case DRM_FORMAT_VUY101010:
        return 30;
    default:
        break;
    }

    return info->cpp[0] * 8;
}
EXPORT_SYMBOL_GPL(drm_get_bpp);

static const struct drm_framebuffer_funcs drm_direct_show_fb_funcs = {
    .destroy = drm_gem_fb_destroy,
    .create_handle = drm_gem_fb_create_handle,
};

struct drm_framebuffer *drm_direct_show_fb_alloc(struct drm_device *dev,
                        const struct drm_direct_show_buffer *buffer)
{
    struct drm_mode_fb_cmd2 mode_cmd = { 0 };
    const struct drm_format_info *info;
    struct drm_framebuffer *fb;
    unsigned int i;
    int ret;

    if (!dev || !buffer || !buffer->gem)
        return ERR_PTR(-EINVAL);

    info = drm_format_info(buffer->pixel_format);
    if (!info)
        return ERR_PTR(-EINVAL);

    mode_cmd.width = buffer->width;
    mode_cmd.height = buffer->height;
    mode_cmd.pixel_format = buffer->pixel_format;

    for (i = 0; i < info->num_planes && i < 4; i++) {
        mode_cmd.pitches[i] = buffer->pitch[i];
        mode_cmd.offsets[i] = buffer->offsets[i];
    }

    fb = kzalloc(sizeof(*fb), GFP_KERNEL);
    if (!fb)
        return ERR_PTR(-ENOMEM);

    drm_helper_mode_fill_fb_struct(dev, fb, &mode_cmd);

    for (i = 0; i < info->num_planes && i < 4; i++) {
        fb->obj[i] = buffer->gem;
        drm_gem_object_get(buffer->gem);
    }

    ret = drm_framebuffer_init(dev, fb, &drm_direct_show_fb_funcs);
    if (ret) {
        for (i = 0; i < info->num_planes && i < 4; i++) {
            if (fb->obj[i])
                drm_gem_object_put(fb->obj[i]);
        }
        kfree(fb);
        return ERR_PTR(ret);
    }

    return fb;
}
EXPORT_SYMBOL_GPL(drm_direct_show_fb_alloc);

static int drm_direct_show_export_dmabuf(struct drm_direct_show_buffer *buffer)
{
    struct dma_buf *dmabuf;
    int fd;

    if (!buffer || !buffer->gem)
        return -EINVAL;

    dmabuf = drm_gem_prime_export(buffer->gem, O_RDWR);
    if (IS_ERR(dmabuf))
        return PTR_ERR(dmabuf);

    fd = dma_buf_fd(dmabuf, O_CLOEXEC);
    if (fd < 0) {
        dma_buf_put(dmabuf);
        return fd;
    }

    buffer->dmabuf = dmabuf;
    buffer->dmabuf_fd = fd;

    return 0;
}

static int drm_direct_show_calc_layout(struct drm_direct_show_buffer *buffer)
{
    const struct drm_format_info *info;
    unsigned int i;

    if (!buffer)
        return -EINVAL;

    info = drm_format_info(buffer->pixel_format);
    if (!info)
        return -EINVAL;

    buffer->bpp = info->cpp[0] * 8;

    for (i = 0; i < ARRAY_SIZE(buffer->pitch); i++) {
        buffer->pitch[i] = 0;
        buffer->offsets[i] = 0;
        buffer->plane_vaddr[i] = NULL;
    }

    buffer->pitch[0] = ALIGN(buffer->width * info->cpp[0], 8);
    buffer->offsets[0] = 0;
    buffer->size = (size_t)buffer->pitch[0] * buffer->height;

    for (i = 1; i < info->num_planes && i < ARRAY_SIZE(buffer->pitch); i++) {
        u32 plane_w = DIV_ROUND_UP(buffer->width, info->hsub);
        u32 plane_h = DIV_ROUND_UP(buffer->height, info->vsub);

        buffer->offsets[i] = buffer->size;
        buffer->pitch[i] = ALIGN(plane_w * info->cpp[i], 8);
        buffer->size += (size_t)buffer->pitch[i] * plane_h;
    }

    return 0;
}

static inline struct drm_gem_object *drm_direct_show_dma_create(struct drm_device *drm,
                                                        size_t size)
{
    struct drm_gem_cma_object *cma_obj;

    if (!drm || !size)
        return ERR_PTR(-EINVAL);

    cma_obj = drm_gem_cma_create(drm, size);
    if (IS_ERR(cma_obj))
        return ERR_CAST(cma_obj);

    return &cma_obj->base;
}

static inline void *drm_direct_show_dma_obj_vaddr(struct drm_gem_object *obj)
{
    if (!obj)
        return NULL;
    return to_drm_gem_cma_obj(obj)->vaddr;
}

int drm_direct_show_alloc_buffer(struct drm_device *drm,
                    struct drm_direct_show_buffer *buffer)
{
    struct drm_gem_object *obj;
    const struct drm_format_info *info;
    unsigned int i;
    int ret;

    if (!drm || !buffer)
        return -EINVAL;

    ret = drm_direct_show_calc_layout(buffer);
    if (ret)
        return ret;

    obj = drm_direct_show_dma_create(drm, buffer->size);
    if (IS_ERR(obj))
        return PTR_ERR(obj);

    buffer->gem = obj;
    buffer->vaddr = drm_direct_show_dma_obj_vaddr(obj);
    if (!buffer->vaddr) {
        ret = -ENOMEM;
        goto err_put_gem;
    }

    info = drm_format_info(buffer->pixel_format);
    if (!info) {
        ret = -EINVAL;
        goto err_put_gem;
    }

    for (i = 0; i < info->num_planes && i < 4; i++) {
        buffer->plane_vaddr[i] = buffer->vaddr + buffer->offsets[i];
    }

    buffer->fb = drm_direct_show_fb_alloc(drm, buffer);
    if (IS_ERR(buffer->fb)) {
        ret = PTR_ERR(buffer->fb);
        buffer->fb = NULL;
        goto err_put_gem;
    }

    ret = drm_direct_show_export_dmabuf(buffer);
    if (ret) {
        DRM_ERROR("export dmabuf failed: %d\n", ret);
        goto err_put_fb;
    }

    return 0;

err_put_fb:
    if (buffer->fb) {
        drm_framebuffer_put(buffer->fb);
        buffer->fb = NULL;
    }

err_put_gem:
    if (buffer->gem) {
        drm_gem_object_put(buffer->gem);
        buffer->gem = NULL;
    }

    return ret;
}
EXPORT_SYMBOL_GPL(drm_direct_show_alloc_buffer);

void drm_direct_show_free_buffer(struct drm_device *drm,
                    struct drm_direct_show_buffer *buffer)
{
    struct drm_gem_object *obj;

    if (!drm || !buffer)
        return;

    if (buffer->dmabuf_fd >= 0) {
        put_unused_fd(buffer->dmabuf_fd);
        buffer->dmabuf_fd = -1;
    }

    obj = buffer->gem;
    if (obj) {
        mutex_lock(&drm->object_name_lock);
        if (obj->dma_buf) {
            dma_buf_put(obj->dma_buf);
            obj->dma_buf = NULL;
        }
        mutex_unlock(&drm->object_name_lock);
    }

    if (buffer->fb) {
        drm_framebuffer_put(buffer->fb);
        buffer->fb = NULL;
    }

    if (buffer->gem) {
        drm_gem_object_put(buffer->gem);
        buffer->gem = NULL;
    }

    if (buffer->dmabuf) {
        dma_buf_put(buffer->dmabuf);
        buffer->dmabuf = NULL;
    }

    buffer->vaddr = NULL;
}
EXPORT_SYMBOL_GPL(drm_direct_show_free_buffer);

struct drm_plane *drm_direct_show_get_plane(struct drm_device *drm, const char *name)
{
    struct drm_plane *plane;

    if (!drm)
        return NULL;

    drm_for_each_plane(plane, drm) {
        if (!strncmp(plane->name, name, strlen(plane->name)))
            return plane;
    }

    if (name)
        DRM_ERROR("Failed to find plane: %s\n", name);

    return NULL;
}
EXPORT_SYMBOL_GPL(drm_direct_show_get_plane);

struct drm_crtc *drm_direct_show_get_crtc(struct drm_device *drm, const char *name)
{
    struct drm_crtc *crtc;

    if (!drm)
        return NULL;

    drm_for_each_crtc(crtc, drm) {
        if (!crtc->state || !crtc->state->active)
            continue;

        if (!name)
            return crtc;

        if (!strncmp(crtc->name, name, strlen(crtc->name)))
            return crtc;
    }

    DRM_ERROR("Failed to find active crtc: %s\n", name ? name : "(null)");
    return NULL;
}
EXPORT_SYMBOL_GPL(drm_direct_show_get_crtc);

struct drm_property *drm_direct_show_find_prop(struct drm_device *dev,
                        struct drm_mode_object *obj, const char *prop_name)
{
    int i;

    if (!dev || !obj || !prop_name || !obj->properties)
        return NULL;

    for (i = 0; i < obj->properties->count; i++) {
        struct drm_property *prop = obj->properties->properties[i];

        if (!strncmp(prop->name, prop_name, DRM_PROP_NAME_LEN))
            return prop;
    }

    return NULL;
}
EXPORT_SYMBOL_GPL(drm_direct_show_find_prop);

int drm_direct_show_commit(struct drm_device *drm,
                    struct drm_direct_show_commit_info *commit_info)
{
    int ret;
    struct drm_plane *plane;
    struct drm_crtc *crtc;
    struct drm_framebuffer *fb;

    if (!drm || !commit_info)
        return -EINVAL;

    plane = commit_info->plane;
    crtc = commit_info->crtc;
    fb = commit_info->buffer ? commit_info->buffer->fb : NULL;

    if (!plane || !crtc || !fb) {
        DRM_ERROR("Invalid commit parameters\n");
        return -EINVAL;
    }

    drm_modeset_lock_all(drm);

    if (!plane->funcs || !plane->funcs->update_plane) {
        DRM_ERROR("Plane has no update_plane function\n");
        ret = -ENOSYS;
        goto unlock;
    }

    ret = plane->funcs->update_plane(plane, crtc, fb,
                            commit_info->dst_x, commit_info->dst_y,
                            commit_info->dst_w, commit_info->dst_h,
                            commit_info->src_x << 16,
                            commit_info->src_y << 16,
                            commit_info->src_w << 16,
                            commit_info->src_h << 16,
                            drm->mode_config.acquire_ctx);
    if (ret)
        DRM_ERROR("update_plane failed: %d\n", ret);

unlock:
    drm_modeset_unlock_all(drm);

    return ret;
}
EXPORT_SYMBOL_GPL(drm_direct_show_commit);
