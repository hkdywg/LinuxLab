/*
 *  vkms_drv.c
 *
 *  brif
 *  	vkms of drm driver
 *  
 *  (C) 2026.02.25 <hkdywg@163.com>
 *
 *  This program is free software; you can redistribute it and/r modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 * */

#include <drm/drm_drv.h>
#include <drm/drm_atomic_helper.h>
#include <drm/drm_crtc_helper.h>
#include <drm/drm_fb_cma_helper.h>
#include <drm/drm_gem_cma_helper.h>
#include <linux/hrtimer.h>
#include <drm/drm_vblank.h>
#include <linux/platform_device.h>
#include <drm/drm_probe_helper.h>
#include <drm/drm_fb_helper.h>

static unsigned int vblank_interval_ns = 16666667;
module_param(vblank_interval_ns, uint, 0644);
MODULE_PARM_DESC(vblank_interval_ns, "VBlank interval in nanoseconds (default: 16666667 ns for ~60Hz)");

struct vkms_device {
    struct drm_device drm;
    struct drm_plane primary;
    struct drm_crtc crtc;
    struct drm_encoder encoder;
    struct drm_connector connector;
    struct hrtimer vblank_hrtimer;
};

static void vkms_plane_atomic_update(struct drm_plane *plane,
                        struct drm_plane_state *old_state)
{

}

static enum hrtimer_restart vkms_vblank_simulate(struct hrtimer *timer)
{
    struct vkms_device *vkms = container_of(timer, struct vkms_device, vblank_hrtimer);

    drm_crtc_handle_vblank(&vkms->crtc);
    hrtimer_forward_now(&vkms->vblank_hrtimer, vblank_interval_ns);

    return HRTIMER_RESTART;
}

static int vkms_enable_vblank(struct drm_crtc *crtc)
{
    struct vkms_device *vkms = container_of(crtc, struct vkms_device, crtc);

    hrtimer_init(&vkms->vblank_hrtimer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
    vkms->vblank_hrtimer.function = &vkms_vblank_simulate;
    hrtimer_start(&vkms->vblank_hrtimer, vblank_interval_ns, HRTIMER_MODE_REL);

    return 0;
}

static void vkms_disable_vblank(struct drm_crtc *crtc)
{
    struct vkms_device *vkms = container_of(crtc, struct vkms_device, crtc);

    hrtimer_cancel(&vkms->vblank_hrtimer);
}

static void vkms_crtc_atomic_flush(struct drm_crtc *crtc,
                        struct drm_crtc_state *old_crtc_state)
{
    unsigned long flags;

    if(crtc->state->event) {
        spin_lock_irqsave(&crtc->dev->event_lock, flags);
        drm_crtc_send_vblank_event(crtc, crtc->state->event);
        spin_unlock_irqrestore(&crtc->dev->event_lock, flags);

        crtc->state->event = NULL;
    }
}

static void vkms_crtc_atomic_enable(struct drm_crtc *crtc,
                            struct drm_crtc_state *old_state)
{
    drm_crtc_vblank_on(crtc);
}

static void vkms_crtc_atomic_disable(struct drm_crtc *crtc,
                            struct drm_crtc_state *old_state)
{
    drm_crtc_vblank_off(crtc);
}

static void vkms_connector_destroy(struct drm_connector *connector)
{
    drm_connector_unregister(connector);
    drm_connector_cleanup(connector);
}

static int vkms_conn_get_modes(struct drm_connector *connector)
{
    int count;

    count = drm_add_modes_noedid(connector, 1920, 1080);
    drm_set_preferred_mode(connector, 1024, 768);

    return count;
}

static const struct drm_plane_funcs vkms_plane_funcs = {
    .update_plane = drm_atomic_helper_update_plane,
    .disable_plane = drm_atomic_helper_disable_plane,
    .destroy = drm_plane_cleanup,
    .reset = drm_atomic_helper_plane_reset,
    .atomic_duplicate_state = drm_atomic_helper_plane_duplicate_state,
    .atomic_destroy_state = drm_atomic_helper_plane_destroy_state,
};

static const struct drm_plane_helper_funcs vkms_primary_helper_funcs = {
    .atomic_update = vkms_plane_atomic_update,
};

static const struct drm_crtc_funcs vkms_crtc_funcs = {
    .set_config = drm_atomic_helper_set_config,
    .destroy = drm_crtc_cleanup,
    .page_flip = drm_atomic_helper_page_flip,
    .reset = drm_atomic_helper_crtc_reset,
    .atomic_duplicate_state = drm_atomic_helper_crtc_duplicate_state,
    .atomic_destroy_state = drm_atomic_helper_crtc_destroy_state,
    .enable_vblank = vkms_enable_vblank,
    .disable_vblank = vkms_disable_vblank,
};

static const struct drm_crtc_helper_funcs vkms_crtc_helper_funcs = {
    .atomic_flush = vkms_crtc_atomic_flush,
    .atomic_enable = vkms_crtc_atomic_enable,
    .atomic_disable = vkms_crtc_atomic_disable,
};

static const struct drm_encoder_funcs vkms_encoder_funcs = {
    .destroy = drm_encoder_cleanup
};

static const struct drm_connector_funcs vkms_connector_funcs = {
    .fill_modes = drm_helper_probe_single_connector_modes,
    .destroy = vkms_connector_destroy,
    .reset = drm_atomic_helper_connector_reset,
    .atomic_duplicate_state = drm_atomic_helper_connector_duplicate_state,
    .atomic_destroy_state = drm_atomic_helper_connector_destroy_state,
};

static const struct  drm_connector_helper_funcs vkms_conn_helper_funcs = {
    .get_modes = vkms_conn_get_modes,
};

static void vkms_plane_init(struct vkms_device *vkms)
{
    struct drm_device *drm = &vkms->drm;
    struct drm_plane *primary = &vkms->primary;

    static const u32 vkms_formats[] = {
        DRM_FORMAT_XRGB8888,
    };

    drm_universal_plane_init(drm, primary, 0, &vkms_plane_funcs,
                vkms_formats, ARRAY_SIZE(vkms_formats), NULL, DRM_PLANE_TYPE_PRIMARY, NULL);
    drm_plane_helper_add(primary, &vkms_primary_helper_funcs);
}

static void vkms_crtc_init(struct vkms_device *vkms)
{
    struct drm_device *drm = &vkms->drm;
    struct drm_crtc *crtc = &vkms->crtc;
    struct drm_plane *primary = &vkms->primary;

    drm_crtc_init_with_planes(drm, crtc, primary, NULL,
                        &vkms_crtc_funcs, NULL);
    drm_crtc_helper_add(crtc, &vkms_crtc_helper_funcs);
}

static void vkms_encoder_init(struct vkms_device *vkms)
{
    struct drm_device *drm = &vkms->drm;
    struct drm_encoder *encoder = &vkms->encoder;

    drm_encoder_init(drm, encoder, &vkms_encoder_funcs,
                DRM_MODE_ENCODER_VIRTUAL, NULL);
    encoder->possible_crtcs = 1;
}

static void vkms_connector_init(struct vkms_device *vkms)
{
    struct drm_device *drm = &vkms->drm;
    struct drm_encoder *encoder = &vkms->encoder;
    struct drm_connector *connector = &vkms->connector;

    drm_connector_init(drm, connector, &vkms_connector_funcs, DRM_MODE_CONNECTOR_VIRTUAL);
    drm_connector_helper_add(connector, &vkms_conn_helper_funcs);
    drm_connector_register(connector);
    drm_connector_attach_encoder(connector, encoder);
}

/************* init plane/crtc/encoder/connector *************/
void vkms_output_init(struct vkms_device *vkms)
{
    vkms_plane_init(vkms);
    vkms_crtc_init(vkms);
    vkms_encoder_init(vkms);
    vkms_connector_init(vkms);
}

/********************** drm core **************************/
DEFINE_DRM_GEM_CMA_FOPS(vkms_fops);

static struct drm_driver vkms_driver = {
    .driver_features = DRIVER_MODESET | DRIVER_ATOMIC | DRIVER_GEM,
    .fops = &vkms_fops,
    .dumb_create = drm_gem_cma_dumb_create,
    .gem_vm_ops  = &drm_gem_cma_vm_ops,
    .gem_free_object_unlocked = drm_gem_cma_free_object,
    .name = "vkms",
    .desc = "Virtual Kernel Mode Setting",
    .date = "20260225",
    .major = 1,
    .minor = 0,
};

static const struct drm_mode_config_funcs vkms_mode_funcs = {
    .atomic_check = drm_atomic_helper_check,
    .atomic_commit = drm_atomic_helper_commit,
};

static void vkms_modeset_init(struct vkms_device *vkms)
{
    struct drm_device *drm = &vkms->drm;

    drm_mode_config_init(drm);

    drm->mode_config.funcs = &vkms_mode_funcs;
    drm->mode_config.min_width = 32;
    drm->mode_config.min_height = 32;
    drm->mode_config.max_width = 8192;
    drm->mode_config.max_height = 8192;

    vkms_output_init(vkms);
    drm_mode_config_reset(drm);
}

static int vkms_probe(struct platform_device *pdev)
{
    struct vkms_device *vkms;
    int ret;

    vkms = devm_kzalloc(&pdev->dev, sizeof(*vkms), GFP_KERNEL);
    if (!vkms)
        return -ENOMEM;

    platform_set_drvdata(pdev, vkms);

    drm_dev_init(&vkms->drm, &vkms_driver, &pdev->dev);
    vkms->drm.irq_enabled = true;
    ret = drm_vblank_init(&vkms->drm, 1);
    if (ret)
        goto err_vblank_init;

    vkms_modeset_init(vkms);
    ret = drm_dev_register(&vkms->drm, 0);
    if (ret)
        goto err_drm_dev_register;

    drm_fbdev_generic_setup(&vkms->drm, 32);

    return 0;

err_drm_dev_register:
err_vblank_init:
    drm_dev_put(&vkms->drm);
    return ret;
}

static int vkms_remove(struct platform_device *pdev)
{
    struct vkms_device *vkms = platform_get_drvdata(pdev);

    drm_dev_unregister(&vkms->drm);
    drm_dev_put(&vkms->drm);
    return 0;
}

static struct platform_device vkms_pdev = {
    .name = "vkms",
    .id = -1,
};

static struct platform_driver vkms_platform_driver = {
    .probe = vkms_probe,
    .remove = vkms_remove,
    .driver = {
        .name = "vkms",
        .owner = THIS_MODULE,
    },
};

static int __init vkms_driver_init(void)
{
    int ret;

    ret = platform_device_register(&vkms_pdev);
    if (ret)
        return ret;

    ret = platform_driver_register(&vkms_platform_driver);
    if (ret) {
        platform_device_unregister(&vkms_pdev);
        return ret;
    }

    return 0;
}

static void __exit vkms_driver_exit(void)
{
    platform_driver_unregister(&vkms_platform_driver);
    platform_device_unregister(&vkms_pdev);
}

module_init(vkms_driver_init);
module_exit(vkms_driver_exit);


MODULE_LICENSE("GPL");
MODULE_AUTHOR("yinwg <hkdywg@163.com>");
MODULE_DESCRIPTION("virtual kms driver");
