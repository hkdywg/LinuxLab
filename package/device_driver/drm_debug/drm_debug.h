/*
* drm_debug.h 
*   drm debug header
*
*  (C) 2026.03.01 <hkdywg@163.com>
*
*  This program is free software; you can redistribute it and/r modify
*  it under the terms of the GNU General Public License version 2 as
*  published by the Free Software Foundation.
*
*/
#ifndef __DRM_DEBUG_H_
#define __DRM_DEBUG_H_

struct drm_direct_show_buffer {
    /* input */
    u32 width;
    u32 height;
    u32 pixel_format;
    /* output/layout */
    u32 bpp;                /* bpp for plane 0 (for debug) */
    u32 pitch[4];           
    u32 offsets[4];           
    size_t size;
    
    /* backing objects */
    struct drm_gem_object *gem;
    struct drm_framebuffer *fb;

    /* export (optional) */
    struct dma_buf *dmabuf; /* extra ref for kernel-side sync */
    int dmabuf_fd;          /* exported fd for external modules/userspace */

    /* kernel mapping (DMA helper local allocation) */
    void *vaddr; 
    void *plane_vaddr[4];    
};

struct drm_direct_show_commit_info {
    struct drm_crtc *crtc;
    struct drm_plane *plane;
    struct drm_direct_show_buffer *buffer;
    u32 src_x, src_y, src_w, src_h;
    u32 dst_x, dst_y, dst_w, dst_h;
    bool top_zpos;
};

void drm_fill_color_bar(uint32_t format, void *planes[3], unsigned int width,
                unsigned int height, unsigned int stride);

int drm_direct_show_alloc_buffer(struct drm_device *drm, 
                    struct drm_direct_show_buffer *buffer);

void drm_direct_show_free_buffer(struct drm_device *drm,
                    struct drm_direct_show_buffer *buffer);

int drm_direct_show_commit(struct drm_device *drm,
                    struct drm_direct_show_commit_info *commit_info);

int drm_modeset_debugfs_init(struct drm_device *drm, struct dentry *root);

#endif
