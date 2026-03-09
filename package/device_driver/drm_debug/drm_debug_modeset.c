/*
* drm_debug_modeset.c
*   DRM modeset debug utility - query CRTC, Plane, Connector, Encoder status
*
*  (C) 2026.03.06 <hkdywg@163.com>
*
*  This program is free software; you can redistribute it and/r modify
*  it under the terms of the GNU General Public License version 2 as
*  published by the Free Software Foundation.
*
*/
#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/debugfs.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/kernel.h>
#include <drm/drm_print.h>
#include <drm/drm_vblank.h>
#include <drm/drm_atomic_uapi.h>
#include <drm/drm_drv.h>
#include <drm/drm_file.h>
#include <drm/drm_gem.h>
#include <drm/drm_fourcc.h>
#include <drm/drm_modes.h>
#include <drm/drm_connector.h>
#include <drm/drm_plane.h>
#include <drm/drm_crtc.h>
#include <drm/drm_encoder.h>
#include <drm/drm_framebuffer.h>
#include <drm/drm_debugfs.h>
#include <drm/drm_atomic.h>

struct drm_modeset_debugfs_entry {
    const char *name;
    int (*show_func)(struct seq_file *, void *);
};

static const char *drm_connector_status_name(enum drm_connector_status status)
{
    switch (status) {
    case connector_status_connected:
        return "connected";
    case connector_status_disconnected:
        return "disconnected";
    case connector_status_unknown:
    default:
        return "unknown";
    }
}

static const char *drm_connector_type_name(int type)
{
    switch (type) {
    case DRM_MODE_CONNECTOR_Unknown:
        return "Unknown";
    case DRM_MODE_CONNECTOR_VGA:
        return "VGA";
    case DRM_MODE_CONNECTOR_DVII:
        return "DVI-I";
    case DRM_MODE_CONNECTOR_DVID:
        return "DVI-D";
    case DRM_MODE_CONNECTOR_DVIA:
        return "DVI-A";
    case DRM_MODE_CONNECTOR_Composite:
        return "Composite";
    case DRM_MODE_CONNECTOR_SVIDEO:
        return "S-Video";
    case DRM_MODE_CONNECTOR_LVDS:
        return "LVDS";
    case DRM_MODE_CONNECTOR_Component:
        return "Component";
    case DRM_MODE_CONNECTOR_9PinDIN:
        return "9-Pin DIN";
    case DRM_MODE_CONNECTOR_DisplayPort:
        return "DisplayPort";
    case DRM_MODE_CONNECTOR_HDMIA:
        return "HDMI-A";
    case DRM_MODE_CONNECTOR_HDMIB:
        return "HDMI-B";
    case DRM_MODE_CONNECTOR_TV:
        return "TV";
    case DRM_MODE_CONNECTOR_eDP:
        return "eDP";
    case DRM_MODE_CONNECTOR_VIRTUAL:
        return "Virtual";
    case DRM_MODE_CONNECTOR_DSI:
        return "DSI";
    case DRM_MODE_CONNECTOR_DPI:
        return "DPI";
    default:
        return "Unknown";
    }
}

static const char *drm_plane_type_name(enum drm_plane_type type)
{
    switch (type) {
    case DRM_PLANE_TYPE_OVERLAY:
        return "Overlay";
    case DRM_PLANE_TYPE_PRIMARY:
        return "Primary";
    case DRM_PLANE_TYPE_CURSOR:
        return "Cursor";
    default:
        return "Unknown";
    }
}

static const char *drm_rotation_name(uint32_t rotation)
{
    switch (rotation & DRM_MODE_ROTATE_MASK) {
    case DRM_MODE_ROTATE_0:
        return "0";
    case DRM_MODE_ROTATE_90:
        return "90";
    case DRM_MODE_ROTATE_180:
        return "180";
    case DRM_MODE_ROTATE_270:
        return "270";
    default:
        return "Unknown";
    }

    return "Unknown";
}

static void drm_modeset_print_rotation(struct seq_file *m, struct drm_plane *plane,
                    struct drm_plane_state *state)
{
    struct drm_property *rotation_prop;
    uint32_t rotation;

    rotation_prop = plane->rotation_property;
    if (!rotation_prop)
        return;

    rotation = state->rotation;

    seq_printf(m, "  rotation: %s", drm_rotation_name(rotation));

    if (rotation & DRM_MODE_REFLECT_X)
        seq_printf(m, " + reflect-x");
    if (rotation & DRM_MODE_REFLECT_Y)
        seq_printf(m, " + reflect-y");

    seq_printf(m, " (0x%08x)\n", rotation);
}

#if 0
static bool plane_supports_rotation(struct drm_plane *plane)
{
    uint64_t supported;

    if(!plane->rotation_property)
        return false;

    supported = plane->rotation_property->values[0];

    if(supported & (DRM_MODE_ROTATE_90 | DRM_MODE_ROTATE_180 |
                    DRM_MODE_ROTATE_270 | DRM_MODE_REFLECT_X | DRM_MODE_REFLECT_Y))
        return true;
    
    return false;
}
#endif

static void drm_modeset_print_crtc_info(struct seq_file *m, struct drm_crtc *crtc)
{
    struct drm_crtc_state *state = crtc->state;
    struct drm_display_mode *mode;

    seq_printf(m, "CRTC[%d]: %s\n", drm_crtc_index(crtc), crtc->name);
    seq_printf(m, "  enabled: %s\n", state->enable ? "yes" : "no");
    seq_printf(m, "  active: %s\n", state->active ? "yes" : "no");
    seq_printf(m, "  self_refresh_active: %s\n",
            state->self_refresh_active ? "yes" : "no");

    if (state->enable) {
        mode = &state->mode;
        seq_printf(m, "  mode: %dx%d@%dHz\n", mode->hdisplay, mode->vdisplay,
               drm_mode_vrefresh(mode));
        seq_printf(m, "  clock: %d kHz\n", mode->clock);
        seq_printf(m, "  htotal: %d, hsync: %d-%d\n",
               mode->htotal, mode->hsync_start, mode->hsync_end);
        seq_printf(m, "  vtotal: %d, vsync: %d-%d\n",
               mode->vtotal, mode->vsync_start, mode->vsync_end);
        seq_printf(m, "  flags: 0x%08x\n", mode->flags);
        seq_printf(m, "  type: 0x%08x\n", mode->type);
    }

    seq_printf(m, "  gamma_size: %d\n", crtc->gamma_size);
    seq_printf(m, "  gamma_lut: %p\n", state->gamma_lut);
    seq_printf(m, "  degamma_lut: %p\n", state->degamma_lut);
    seq_printf(m, "  ctm: %p\n", state->ctm);

    seq_printf(m, "\n");
}

static void drm_modeset_print_plane_info(struct seq_file *m, struct drm_plane *plane)
{
    struct drm_plane_state *state = plane->state;
    struct drm_framebuffer *fb;
    char format_buf[32];
    int i;

    seq_printf(m, "PLANE[%d]: %s\n", drm_plane_index(plane), plane->name);
    seq_printf(m, "  type: %s\n", drm_plane_type_name(plane->type));
    seq_printf(m, "  enabled: %s\n", state->visible ? "yes" : "no");

    if (!state->visible)
        goto print_crtc;

    fb = state->fb;
    if (fb) {
        seq_printf(m, "  fb: %dx%d\n", fb->width, fb->height);

        snprintf(format_buf, sizeof(format_buf), "%p4cc", &fb->format->format);
        seq_printf(m, "  format: %s\n", format_buf);
        seq_printf(m, "  modifier: 0x%016llx\n", fb->modifier);

        for (i = 0; i < fb->format->num_planes; i++) {
            seq_printf(m, "  plane[%d]: pitch=%d, offset=%d\n",
                   i, fb->pitches[i], fb->offsets[i]);
        }
    }

    seq_printf(m, "  src: (%d,%d) %dx%d\n",
           state->src.x1 >> 16, state->src.y1 >> 16,
           (state->src.x2 - state->src.x1) >> 16,
           (state->src.y2 - state->src.y1) >> 16);
    seq_printf(m, "  dst: (%d,%d) %dx%d\n",
           state->dst.x1, state->dst.y1,
           state->dst.x2 - state->dst.x1,
           state->dst.y2 - state->dst.y1);

    seq_printf(m, "  zpos: %d\n", state->zpos);
    seq_printf(m, "  alpha: %d\n", state->alpha);
    seq_printf(m, "  pixel_blend_mode: %s\n",
           state->pixel_blend_mode == DRM_MODE_BLEND_PIXEL_NONE ? "none" :
           state->pixel_blend_mode == DRM_MODE_BLEND_PREMULTI ? "premulti" :
           "coverage");

    drm_modeset_print_rotation(m, plane, state);

print_crtc:
    if (state->crtc)
        seq_printf(m, "  crtc: %s\n", state->crtc->name);

    seq_printf(m, "\n");
}

static void drm_modeset_print_connector_info(struct seq_file *m,
                      struct drm_connector *connector)
{
    struct drm_display_mode *mode;

    seq_printf(m, "CONNECTOR[%d]: %s\n", connector->base.id, connector->name);
    seq_printf(m, "  type: %s\n", drm_connector_type_name(connector->connector_type));
    seq_printf(m, "  status: %s\n", drm_connector_status_name(connector->status));
    seq_printf(m, "  dpms: %d\n", connector->dpms);
    seq_printf(m, " ");

    if (connector->eld) {
        seq_printf(m, "  (EDID available)\n");
        seq_printf(m, "  edid: %p\n", connector->eld);
    }

    seq_printf(m, "  modes:\n");
    list_for_each_entry(mode, &connector->modes, head) {
        seq_printf(m, "    %s: %dx%d@%dHz\n",
               mode->name, mode->hdisplay, mode->vdisplay,
               drm_mode_vrefresh(mode));
    }

    seq_printf(m, "  tile_group: %p\n", connector->tile_group);
    seq_printf(m, "  poll_enabled: %s\n",
           connector->polled & DRM_CONNECTOR_POLL_CONNECT ? "yes" : "no");
    seq_printf(m, "  interlace_allowed: %s\n",
           connector->interlace_allowed ? "yes" : "no");
    seq_printf(m, "  doublescan_allowed: %s\n",
           connector->doublescan_allowed ? "yes" : "no");

    seq_printf(m, "\n");
}

static struct drm_crtc *drm_modeset_encoder_get_crtc(struct drm_encoder *encoder)
{
    struct drm_connector *connector;
    struct drm_device *dev = encoder->dev;
    bool uses_atomic = false;
    struct drm_connector_list_iter conn_iter;

    drm_connector_list_iter_begin(dev, &conn_iter);
    drm_for_each_connector_iter(connector, &conn_iter) {
        if (!connector->state)
            continue;

        uses_atomic = true;

        if (connector->state->best_encoder != encoder)
            continue;

        drm_connector_list_iter_end(&conn_iter);
        return connector->state->crtc;
    }
    drm_connector_list_iter_end(&conn_iter);

    if (uses_atomic)
        return NULL;

    return encoder->crtc;
}

static void drm_modeset_print_encoder_info(struct seq_file *m,
                      struct drm_encoder *encoder)
{
    struct drm_crtc *crtc;

    seq_printf(m, "ENCODER[%d]: %s\n", encoder->base.id, encoder->name);
    seq_printf(m, "  type: %s\n",
           encoder->encoder_type == DRM_MODE_ENCODER_NONE ? "None" :
           encoder->encoder_type == DRM_MODE_ENCODER_DAC ? "DAC" :
           encoder->encoder_type == DRM_MODE_ENCODER_TMDS ? "TMDS" :
           encoder->encoder_type == DRM_MODE_ENCODER_LVDS ? "LVDS" :
           encoder->encoder_type == DRM_MODE_ENCODER_TVDAC ? "TVDAC" :
           encoder->encoder_type == DRM_MODE_ENCODER_VIRTUAL ? "Virtual" :
           encoder->encoder_type == DRM_MODE_ENCODER_DSI ? "DSI" :
           encoder->encoder_type == DRM_MODE_ENCODER_DPMST ? "DP MST" :
           "Unknown");

    crtc = drm_modeset_encoder_get_crtc(encoder);
    seq_printf(m, "  crtc: %s\n", crtc ? crtc->name : "(none)");
    seq_printf(m, "  possible_crtcs: 0x%08x\n", encoder->possible_crtcs);
    seq_printf(m, "  possible_clones: 0x%08x\n", encoder->possible_clones);

    seq_printf(m, "\n");
}

static void drm_modeset_print_fb_info(struct seq_file *m, struct drm_framebuffer *fb)
{
    char format_buf[32];
    int i;

    if (!fb) {
        seq_printf(m, "  fb: (none)\n");
        return;
    }

    seq_printf(m, "FB[%d]: %dx%d\n", fb->base.id, fb->width, fb->height);

    snprintf(format_buf, sizeof(format_buf), "%p4cc", &fb->format->format);
    seq_printf(m, "  format: %s\n", format_buf);
    seq_printf(m, "  modifier: 0x%016llx\n", fb->modifier);
    seq_printf(m, "  flags: 0x%08x\n", fb->flags);

    for (i = 0; i < fb->format->num_planes; i++) {
        seq_printf(m, "  plane[%d]: pitch=%d, offset=%d, obj=%p\n",
               i, fb->pitches[i], fb->offsets[i], fb->obj[i]);
    }

    seq_printf(m, "\n");
}

static void drm_modeset_print_device_info(struct seq_file *m, struct drm_device *drm)
{
    seq_printf(m, "===========================================\n");
    seq_printf(m, "DRM Device Information\n");
    seq_printf(m, "===========================================\n");
    seq_printf(m, "driver: %s\n", drm->driver->name);
    seq_printf(m, "driver date: %s\n", drm->driver->date);
    seq_printf(m, "primary: %d\n", drm->primary ? drm->primary->index : -1);
    seq_printf(m, "render: %d\n", drm->render ? drm->render->index : -1);
    seq_printf(m, "device: 0x%lx\n", (unsigned long)drm->dev);
    seq_printf(m, "registered: %s\n", drm->registered ? "yes" : "no");
    seq_printf(m, "master: %s\n", drm->master ? "yes" : "no");

    seq_printf(m, "\nCounts:\n");
    seq_printf(m, "  crtc: %d\n", drm->mode_config.num_crtc);
    seq_printf(m, "  connector: %d\n", drm->mode_config.num_connector);
    seq_printf(m, "  encoder: %d\n", drm->mode_config.num_encoder);
    seq_printf(m, "  fb: %d\n", drm->mode_config.num_fb);

    seq_printf(m, "\n");
}

static int drm_modeset_show_crtcs(struct seq_file *m, void *data)
{
    struct drm_device *drm = m->private;
    struct drm_crtc *crtc;

    if (!drm)
        return -ENODEV;

    seq_printf(m, "===========================================\n");
    seq_printf(m, "CRTC Information\n");
    seq_printf(m, "===========================================\n");

    drm_modeset_lock_all(drm);
    drm_for_each_crtc(crtc, drm) {
        drm_modeset_print_crtc_info(m, crtc);
    }
    drm_modeset_unlock_all(drm);

    return 0;
}

static int drm_modeset_show_planes(struct seq_file *m, void *data)
{
    struct drm_device *drm = m->private;
    struct drm_plane *plane;

    if (!drm)
        return -ENODEV;

    seq_printf(m, "===========================================\n");
    seq_printf(m, "Plane Information\n");
    seq_printf(m, "===========================================\n");

    drm_modeset_lock_all(drm);
    drm_for_each_plane(plane, drm) {
        drm_modeset_print_plane_info(m, plane);
    }
    drm_modeset_unlock_all(drm);

    return 0;
}

static int drm_modeset_show_connectors(struct seq_file *m, void *data)
{
    struct drm_device *drm = m->private;
    struct drm_connector *connector;
    struct drm_connector_list_iter conn_iter;

    if (!drm)
        return -ENODEV;

    seq_printf(m, "===========================================\n");
    seq_printf(m, "Connector Information\n");
    seq_printf(m, "===========================================\n");

    drm_modeset_lock_all(drm);

    drm_connector_list_iter_begin(drm, &conn_iter);
    drm_for_each_connector_iter(connector, &conn_iter) {
        drm_modeset_print_connector_info(m, connector);
    }
    drm_connector_list_iter_end(&conn_iter);

    drm_modeset_unlock_all(drm);

    return 0;
}

static int drm_modeset_show_encoders(struct seq_file *m, void *data)
{
    struct drm_device *drm = m->private;
    struct drm_encoder *encoder;

    if (!drm)
        return -ENODEV;

    seq_printf(m, "===========================================\n");
    seq_printf(m, "Encoder Information\n");
    seq_printf(m, "===========================================\n");

    drm_modeset_lock_all(drm);
    drm_for_each_encoder(encoder, drm) {
        drm_modeset_print_encoder_info(m, encoder);
    }
    drm_modeset_unlock_all(drm);

    return 0;
}

static int drm_modeset_show_framebuffers(struct seq_file *m, void *data)
{
    struct drm_device *drm = m->private;
    struct drm_framebuffer *fb;
    int count = 0;

    if (!drm)
        return -ENODEV;

    seq_printf(m, "===========================================\n");
    seq_printf(m, "Framebuffer Information\n");
    seq_printf(m, "===========================================\n");

    drm_modeset_lock_all(drm);
    drm_for_each_fb(fb, drm) {
        drm_modeset_print_fb_info(m, fb);
        count++;
    }
    drm_modeset_unlock_all(drm);

    seq_printf(m, "Total framebuffers: %d\n", count);

    return 0;
}

static int drm_modeset_show_device(struct seq_file *m, void *data)
{
    struct drm_device *drm = m->private;

    if (!drm)
        return -ENODEV;

    drm_modeset_print_device_info(m, drm);
    return 0;
}

static int drm_modeset_show_summary(struct seq_file *m, void *data)
{
    struct drm_device *drm = m->private;
    struct drm_crtc *crtc;
    struct drm_plane *plane;
    struct drm_connector *connector;
    struct drm_encoder *encoder;
    struct drm_connector_list_iter conn_iter;
    int crtc_count = 0, plane_count = 0, connector_count = 0, encoder_count = 0;

    if (!drm)
        return -ENODEV;

    seq_printf(m, "===========================================\n");
    seq_printf(m, "DRM Status Summary\n");
    seq_printf(m, "===========================================\n");

    drm_modeset_lock_all(drm);

    drm_for_each_crtc(crtc, drm) {
        if (crtc->state && crtc->state->enable)
            crtc_count++;
    }
    drm_for_each_plane(plane, drm) {
        if (plane->state && plane->state->visible)
            plane_count++;
    }

    drm_connector_list_iter_begin(drm, &conn_iter);
    drm_for_each_connector_iter(connector, &conn_iter) {
        if (connector->status == connector_status_connected)
            connector_count++;
    }
    drm_connector_list_iter_end(&conn_iter);

    drm_for_each_encoder(encoder, drm) {
        encoder_count++;
    }

    drm_modeset_unlock_all(drm);

    seq_printf(m, "Enabled CRTCs: %d/%d\n", crtc_count, drm->mode_config.num_crtc);
    seq_printf(m, "Connected Connectors: %d/%d\n",
           connector_count, drm->mode_config.num_connector);
    seq_printf(m, "Encoders: %d/%d\n",
           encoder_count, drm->mode_config.num_encoder);

    seq_printf(m, "\nDebugfs entries available:\n");
    seq_printf(m, "  device   - Device information\n");
    seq_printf(m, "  crtcs    - CRTC details\n");
    seq_printf(m, "  planes   - Plane details\n");
    seq_printf(m, "  connectors - Connector details\n");
    seq_printf(m, "  encoders - Encoder details\n");
    seq_printf(m, "  framebuffers - Framebuffer details\n");

    seq_printf(m, "\n");

    return 0;
}

struct drm_modeset_debugfs_entry modeset_debug_entry[] = {
    { "summary",       drm_modeset_show_summary },
    { "device",        drm_modeset_show_device },
    { "crtcs",         drm_modeset_show_crtcs },
    { "planes",        drm_modeset_show_planes },
    { "connectors",    drm_modeset_show_connectors },
    { "encoders",      drm_modeset_show_encoders },
    { "framebuffers",  drm_modeset_show_framebuffers },
};

static int drm_modeset_set_plane_rotation(struct drm_device *drm,
                      const char *plane_name, uint32_t rotation)
{
    struct drm_plane *plane;
    struct drm_plane_state *state;
    struct drm_atomic_state *atomic_state;
    struct drm_crtc *crtc;
    struct drm_crtc_state *crtc_state;
    bool found = false;
    int ret;

    if (!drm || !plane_name)
        return -EINVAL;

    drm_modeset_lock_all(drm);

    drm_for_each_plane(plane, drm) {
        if (!strncmp(plane->name, plane_name, strlen(plane->name))) {
            found = true;
            break;
        }
    }

    if (!found) {
        DRM_ERROR("Plane not found: %s\n", plane_name);
        ret = -ENOENT;
        goto unlock;
    }

    if (!plane->rotation_property) {
        DRM_ERROR("Plane %s does not support rotation\n", plane_name);
        ret = -ENOTSUPP;
        goto unlock;
    }

    if (!(rotation & DRM_MODE_ROTATE_MASK)) {
        DRM_ERROR("Invalid rotation value: 0x%08x\n", rotation);
        ret = -EINVAL;
        goto unlock;
    }

    state = plane->state;
    if (!state || !state->crtc) {
        DRM_ERROR("Plane %s has no CRTC assigned\n", plane_name);
        ret = -ENODATA;
        goto unlock;
    }

    crtc = state->crtc;

    atomic_state = drm_atomic_state_alloc(drm);
    if (!atomic_state) {
        DRM_ERROR("Failed to allocate atomic state\n");
        ret = -ENOMEM;
        goto unlock;
    }

    atomic_state->acquire_ctx = drm->mode_config.acquire_ctx;
    crtc_state = drm_atomic_get_crtc_state(atomic_state, crtc);
    if (IS_ERR(crtc_state)) {
        ret = PTR_ERR(crtc_state);
        DRM_ERROR("Failed to get CRTC state: %d\n", ret);
        goto retry_state;
    }

    state = drm_atomic_get_plane_state(atomic_state, plane);
    if (IS_ERR(state)) {
        ret = PTR_ERR(state);
        DRM_ERROR("Failed to get plane state: %d\n", ret);
        goto retry_state;
    }

    state->rotation = rotation;
    crtc_state->active = true;

    ret = drm_atomic_commit(atomic_state);
    if (ret) {
        DRM_ERROR("Atomic commit failed: %d\n", ret);
        goto retry_state;
    }

    DRM_INFO("Plane %s rotation set to 0x%08x\n", plane_name, rotation);

retry_state:
    drm_atomic_state_put(atomic_state);

unlock:
    drm_modeset_unlock_all(drm);
    return ret;
}

static ssize_t drm_modeset_rotation_write(struct file *file,
                       const char __user *ubuf,
                       size_t len, loff_t *offp)
{
    struct seq_file *m = file->private_data;
    struct drm_device *drm = m->private;
    char buf[128] = {0};
    char plane_name[64] = {0};
    uint32_t rotation = 0;
    int ret;

    if (len > sizeof(buf) - 1)
        return -EINVAL;

    if (copy_from_user(buf, ubuf, len))
        return -EFAULT;

    buf[len] = '\0';

    /* Parse format: plane_name rotation
     * Examples:
     *   plane-0 90      - rotate 90 degrees
     *   plane-1 180     - rotate 180 degrees
     *   plane-0 270     - rotate 270 degrees
     *   plane-0 0       - no rotation
     *   plane-0 90+reflect-x  - rotate 90 + horizontal flip
     *   plane-0 90+reflect-y  - rotate 90 + vertical flip
     */
    ret = sscanf(buf, "%63s %d", plane_name, (int *)&rotation);
    if (ret != 2) {
        DRM_ERROR("Usage: echo 'plane_name rotation' > rotation\n");
        DRM_ERROR("  rotation: 0, 90, 180, 270\n");
        DRM_ERROR("  Example: echo 'plane0 90' > rotation\n");
        return -EINVAL;
    }

    /* Parse rotation value */
    if (strstr(buf, "90+reflect-x") || strstr(buf, "90+reflect-x"))
        rotation = DRM_MODE_ROTATE_90 | DRM_MODE_REFLECT_X;
    else if (strstr(buf, "90+reflect-y"))
        rotation = DRM_MODE_ROTATE_90 | DRM_MODE_REFLECT_Y;
    else if (strstr(buf, "180+reflect-x"))
        rotation = DRM_MODE_ROTATE_180 | DRM_MODE_REFLECT_X;
    else if (strstr(buf, "180+reflect-y"))
        rotation = DRM_MODE_ROTATE_180 | DRM_MODE_REFLECT_Y;
    else if (strstr(buf, "270+reflect-x"))
        rotation = DRM_MODE_ROTATE_270 | DRM_MODE_REFLECT_X;
    else if (strstr(buf, "270+reflect-y"))
        rotation = DRM_MODE_ROTATE_270 | DRM_MODE_REFLECT_Y;
    else if (strstr(buf, "reflect-x"))
        rotation = DRM_MODE_ROTATE_0 | DRM_MODE_REFLECT_X;
    else if (strstr(buf, "reflect-y"))
        rotation = DRM_MODE_ROTATE_0 | DRM_MODE_REFLECT_Y;
    else {
        switch (rotation) {
        case 0:
            rotation = DRM_MODE_ROTATE_0;
            break;
        case 90:
            rotation = DRM_MODE_ROTATE_90;
            break;
        case 180:
            rotation = DRM_MODE_ROTATE_180;
            break;
        case 270:
            rotation = DRM_MODE_ROTATE_270;
            break;
        default:
            DRM_ERROR("Invalid rotation: %d (use 0, 90, 180, 270)\n", rotation);
            return -EINVAL;
        }
    }

    ret = drm_modeset_set_plane_rotation(drm, plane_name, rotation);
    if (ret)
        return ret;

    return len;
}

static int drm_modeset_rotation_show(struct seq_file *m, void *data)
{
    struct drm_device *drm = m->private;
    struct drm_plane *plane;

    if (!drm)
        return -ENODEV;

    seq_printf(m, "===========================================\n");
    seq_printf(m, "Plane Rotation Control\n");
    seq_printf(m, "===========================================\n");
    seq_printf(m, "\nUsage:\n");
    seq_printf(m, "  echo 'plane_name rotation' > rotation\n");
    seq_printf(m, "\nExamples:\n");
    seq_printf(m, "  echo 'plane0 0' > rotation      # No rotation\n");
    seq_printf(m, "  echo 'plane0 90' > rotation     # Rotate 90 degrees\n");
    seq_printf(m, "  echo 'plane0 180' > rotation    # Rotate 180 degrees\n");
    seq_printf(m, "  echo 'plane0 270' > rotation    # Rotate 270 degrees\n");
    seq_printf(m, "  echo 'plane0 reflect-x' > rotation  # Horizontal flip\n");
    seq_printf(m, "  echo 'plane0 reflect-y' > rotation  # Vertical flip\n");
    seq_printf(m, "  echo 'plane0 90+reflect-x' > rotation # Rotate 90 + H flip\n");
    seq_printf(m, "\nCurrent plane rotation:\n");

    drm_modeset_lock_all(drm);
    drm_for_each_plane(plane, drm) {
        if (plane->state && plane->rotation_property) {
            drm_modeset_print_plane_info(m, plane);
        }
    }
    drm_modeset_unlock_all(drm);

    return 0;
}

static int drm_modeset_rotation_open(struct inode *inode, struct file *file)
{
    return single_open(file, drm_modeset_rotation_show, inode->i_private);
}

static const struct file_operations drm_modeset_rotation_fops = {
    .owner = THIS_MODULE,
    .open    = drm_modeset_rotation_open,
    .read    = seq_read,
    .write   = drm_modeset_rotation_write,
    .llseek   = seq_lseek,
    .release = single_release,
};

static int drm_modeset_debug_open(struct inode *inode, struct file *file)
{
    uint8_t i;
    const char *name = file->f_path.dentry->d_name.name;

    for (i = 0; i < ARRAY_SIZE(modeset_debug_entry); i++) {
        if (!strcmp(name, modeset_debug_entry[i].name))
            return single_open(file, modeset_debug_entry[i].show_func, inode->i_private);
    }

    return -EINVAL;
}

static const struct file_operations drm_modeset_fops = {
    .owner = THIS_MODULE,
    .open    = drm_modeset_debug_open,
    .read    = seq_read,
    .llseek   = seq_lseek,
    .release = single_release,
};

int drm_modeset_debugfs_init(struct drm_device *drm, struct dentry *root)
{
    uint8_t i;

    if (!drm->primary || !drm->primary->debugfs_root) {
        DRM_DEBUG("No debugfs root available\n");
        return -ENODEV;
    }

    for (i = 0; i < ARRAY_SIZE(modeset_debug_entry); i++) {
        debugfs_create_file(modeset_debug_entry[i].name, 0444, root, drm, &drm_modeset_fops);
    }

    /* Create rotation control file */
    debugfs_create_file("rotation", 0644, root, drm, &drm_modeset_rotation_fops);

    return 0;
}
EXPORT_SYMBOL_GPL(drm_modeset_debugfs_init);

