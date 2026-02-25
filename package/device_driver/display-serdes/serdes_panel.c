/*
* serdes_panel.c 
*   drm panel access for diffrent serdes chips
*
* @copyright Copyright (c) 2022 Jiangsu New Vision Automotive Electronics Co.，Ltd. All rights reserved.
*
* Author: weigenyin <weigenyin@zjautomotive.com>
*
*/

#include "display_serdes_core.h"

static inline struct serdes_panel *to_serdes_panel(struct drm_panel *panel)
{
    return container_of(panel, struct serdes_panel, panel);
}

static int serdes_panel_prepare(struct drm_panel *panel)
{
    struct serdes_panel  *serdes_panel = to_serdes_panel(panel);
    struct serdes *serdes = serdes_panel->parent;
    int ret = 0;

    if(serdes->chip_data->panel_ops && serdes->chip_data->panel_ops->init)
        ret = serdes->chip_data->panel_ops->init(serdes);

    if(serdes->chip_data->serdes_type == TYPE_DES)
        serdes_i2c_set_sequence(serdes);

    if(serdes->chip_data->panel_ops && serdes->chip_data->panel_ops->prepare)
        ret = serdes->chip_data->panel_ops->prepare(serdes);

    serdes_set_pinctrl_sleep(serdes);
    serdes_set_pinctrl_default(serdes);

    SERDES_DBG_MFD("%s: %s\n", __func__, serdes->chip_data->name);

    return ret;
}

static int serdes_panel_unprepare(struct drm_panel *panel)
{
    struct serdes_panel *serdes_panel = to_serdes_panel(panel);
    struct serdes *serdes = serdes_panel->parent;
    int ret = 0;

    if(serdes->chip_data->panel_ops && serdes->chip_data->panel_ops->unprepare)
        ret = serdes->chip_data->panel_ops->unprepare(serdes);

    serdes_set_pinctrl_sleep(serdes);

    SERDES_DBG_MFD("%s: %s\n", __func__, serdes->chip_data->name);

    return ret;
}

static int serdes_panel_enable(struct drm_panel *panel)
{

    struct serdes_panel *serdes_panel = to_serdes_panel(panel);
    struct serdes *serdes = serdes_panel->parent;
    int ret = 0;

    if(serdes->chip_data->panel_ops && serdes->chip_data->panel_ops->enable)
        ret = serdes->chip_data->panel_ops->enable(serdes);

    backlight_enable(serdes_panel->backlight);

    SERDES_DBG_MFD("%s: %s\n", __func__, serdes->chip_data->name);

    return ret;
}

static int serdes_panel_disable(struct drm_panel *panel)
{

    struct serdes_panel *serdes_panel = to_serdes_panel(panel);
    struct serdes *serdes = serdes_panel->parent;
    int ret = 0;

    if(serdes->chip_data->panel_ops && serdes->chip_data->panel_ops->disable)
        ret = serdes->chip_data->panel_ops->disable(serdes);

    backlight_disable(serdes_panel->backlight);

    SERDES_DBG_MFD("%s: %s\n", __func__, serdes->chip_data->name);

    return ret;
}

static int serdes_panel_get_modes(struct drm_panel *panel,
                        struct drm_connector *connector)
{
    struct serdes_panel *serdes_panel = to_serdes_panel(panel);
    struct serdes *serdes = serdes_panel->parent;
    struct drm_display_mode *mode;
    u32 bus_format = serdes_panel->bus_format;
    int ret = 1;

    connector->display_info.width_mm = serdes_panel->width_mm;
    connector->display_info.height_mm = serdes_panel->height_mm;
    drm_display_info_set_bus_formats(&connector->display_info, &bus_format, 1);

    mode = drm_mode_duplicate(connector->dev, &serdes_panel->mode);
    if (!mode) {
        dev_err(serdes->dev, "Failed to duplicate mode\n");
        return 0;
    }
    mode->width_mm = serdes_panel->width_mm;
    mode->height_mm = serdes_panel->height_mm;
    mode->type = DRM_MODE_TYPE_DRIVER | DRM_MODE_TYPE_PREFERRED;

    drm_mode_set_name(mode);
    drm_mode_probed_add(connector, mode);

    if(serdes->chip_data->panel_ops && serdes->chip_data->panel_ops->get_modes)
        ret = serdes->chip_data->panel_ops->get_modes(serdes);

    dev_info(serdes->dev, "%s wxh = %dx%d mode clock %u KHz, flags[0x%x]\n"
             "      H: %04d %04d %04d %04d\n"
             "      V: %04d %04d %04d %04d\n"
             "bus_format: 0x%x\n",
             panel->dev->of_node->name,
             serdes_panel->width_mm, serdes_panel->height_mm,
             mode->clock, mode->flags,
             mode->hdisplay, mode->hsync_start,
             mode->hsync_end, mode->htotal,
             mode->vdisplay, mode->vsync_start,
             mode->vsync_end, mode->vtotal,
             bus_format);

    return ret;
}

static const struct drm_panel_funcs serdes_panel_funcs = {
    .prepare = serdes_panel_prepare,
    .unprepare = serdes_panel_unprepare,
    .enable = serdes_panel_enable,
    .disable = serdes_panel_disable,
    .get_modes = serdes_panel_get_modes,
};

static void serdes_panel_timing_info(struct device *dev, struct display_timing *dt)
{
    /*
     *
     *				    Active Video
     * Video  ______________________XXXXXXXXXXXXXXXXXXXXXX_____________________
     *	  |<- sync ->|<- back ->|<----- active ----->|<- front ->|<- sync..
     *	  |	     |	 porch  |		     |	 porch	 |
     *
     * HSync _|¯¯¯¯¯¯¯¯¯¯|___________________________________________|¯¯¯¯¯¯¯¯¯
     *
     * VSync ¯|__________|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯|_________
    struct display_timing {
        struct timing_entry pixelclock;

        struct timing_entry hactive;		
        struct timing_entry hfront_porch;	
        struct timing_entry hback_porch;	
        struct timing_entry hsync_len;		

        struct timing_entry vactive;		
        struct timing_entry vfront_porch;	
        struct timing_entry vback_porch;	
        struct timing_entry vsync_len;		

        enum display_flags flags;
    };

    enum display_flags {
        DISPLAY_FLAGS_HSYNC_LOW		= BIT(0),
        DISPLAY_FLAGS_HSYNC_HIGH	= BIT(1),
        DISPLAY_FLAGS_VSYNC_LOW		= BIT(2),
        DISPLAY_FLAGS_VSYNC_HIGH	= BIT(3),

        DISPLAY_FLAGS_DE_LOW		= BIT(4),
        DISPLAY_FLAGS_DE_HIGH		= BIT(5),
        DISPLAY_FLAGS_PIXDATA_POSEDGE	= BIT(6),
        DISPLAY_FLAGS_PIXDATA_NEGEDGE	= BIT(7),
        DISPLAY_FLAGS_INTERLACED	= BIT(8),
        DISPLAY_FLAGS_DOUBLESCAN	= BIT(9),
        DISPLAY_FLAGS_DOUBLECLK		= BIT(10),
        DISPLAY_FLAGS_SYNC_POSEDGE	= BIT(11),
        DISPLAY_FLAGS_SYNC_NEGEDGE	= BIT(12),
    };

    struct timing_entry {
        u32 min;
        u32 typ;
        u32 max;
    };
    */
	/* timing info */
	dev_info(dev, "Parsing display timing from device tree:\n"
             "--------------------------------------------\n"
             "Pixel Clock:           %u Hz\n"
             "Horizontal Active:     %u px\n"
             "Horizontal Front Porch:%u px\n"
             "Horizontal Back Porch: %u px\n"
             "Horizontal Sync Length: %u px\n"
             "Vertical Active:       %u lines\n"
             "Vertical Front Porch:  %u lines\n"
             "Vertical Back Porch:   %u lines\n"
             "Vertical Sync Length:  %u lines\n",
             dt->pixelclock.typ,
             dt->hactive.typ, dt->hfront_porch.typ, dt->hback_porch.typ, dt->hsync_len.typ,
             dt->vactive.typ, dt->vfront_porch.typ, dt->vback_porch.typ, dt->vsync_len.typ);

    /* HSYNC */
    if (dt->flags & DISPLAY_FLAGS_HSYNC_HIGH)
        dev_info(dev, "  HSYNC: active-high\n");
    else if (dt->flags & DISPLAY_FLAGS_HSYNC_LOW)
        dev_info(dev, "  HSYNC: active-low\n");

    /* VSYNC */
    if (dt->flags & DISPLAY_FLAGS_VSYNC_HIGH)
        dev_info(dev, "  VSYNC: active-high\n");
    else if (dt->flags & DISPLAY_FLAGS_VSYNC_LOW)
        dev_info(dev, "  VSYNC: active-low\n");

}

static struct serdes_init_seq *serdes_of_get_init_seq(struct device *dev)
{
    struct device_node *np = dev->of_node;
    struct serdes_init_seq *seq;
    struct reg_sequence *reg_seq;
    int len, count, i;
    u32 *val;
    int ret;

    if (!of_find_property(np, "panel-init-sequence", &len))
        return NULL;

    count = len / sizeof(u32);
    if (count % 2 != 0) {
        dev_err(dev, "Invalid panel-init-sequence length\n");
        return NULL;
    }
    count /= 2;

    seq = devm_kzalloc(dev, sizeof(*seq), GFP_KERNEL);
    if (!seq)
        return NULL;

    reg_seq = devm_kcalloc(dev, count, sizeof(*reg_seq), GFP_KERNEL);
    if (!reg_seq)
        return NULL;

    val = kcalloc(count * 2, sizeof(u32), GFP_KERNEL);
    if (!val)
        return NULL;

    ret = of_property_read_u32_array(np, "panel-init-sequence", val, count * 2);
    if (ret) {
        kfree(val);
        return NULL;
    }

    for (i = 0; i < count; i++) {
        reg_seq[i].reg = val[i * 2];
        reg_seq[i].def = val[i * 2 + 1];
        reg_seq[i].delay_us = 0;
    }

    kfree(val);

    seq->reg_sequence = reg_seq;
    seq->reg_seq_cnt = count;

    return seq;
}

static int serdes_panel_parse_dt(struct serdes_panel *serdes_panel)
{
    struct device *dev = serdes_panel->dev;
    struct display_timing dt;
    struct videomode vm;
    int ret, len;
    const char *mapping;
    unsigned int panel_size[2] = {320, 180};

    serdes_panel->serdes_init_seq = serdes_of_get_init_seq(dev);
    if (serdes_panel->serdes_init_seq)
        dev_info(dev, "Found panel-init-sequence with %d items\n", 
                 serdes_panel->serdes_init_seq->reg_seq_cnt);

    serdes_panel->width_mm = panel_size[0];
    serdes_panel->height_mm = panel_size[1];
    
    if(of_find_property(dev->of_node, "panel-size", &len)) {
        len /= sizeof(unsigned int);
        if(len != 2) {
            dev_err(dev, "panel-size length is error, set 2 default\n");
            len = 2;
        }
        ret = of_property_read_u32_array(dev->of_node, "panel-size",
                                         panel_size, len);
        if(!ret) {
            serdes_panel->width_mm = panel_size[0];
            serdes_panel->height_mm = panel_size[1];
        }
    }

    ret = of_get_display_timing(dev->of_node, "panel-timing", &dt);
    if(ret < 0) {
        dev_err(dev, "%pOF:serdes no panel-timing node found\n", dev->of_node);
        return -ENODEV;
    }
    serdes_panel_timing_info(dev, &dt);

    ret = of_property_read_string(dev->of_node, "data-mapping", &mapping);
    if(ret < 0 ) {
        dev_err(dev, "%pOF: invalid of missing %s DT property\n", 
                dev->of_node, "data-mapping");
        return -ENODEV;
    }

    if (!strcmp(mapping, "jeida-18")) {
        serdes_panel->bus_format = MEDIA_BUS_FMT_RGB666_1X7X3_SPWG;
    } else if (!strcmp(mapping, "jeida-24")) {
        serdes_panel->bus_format = MEDIA_BUS_FMT_RGB888_1X7X4_JEIDA;
    } else if (!strcmp(mapping, "vesa-24")) {
        serdes_panel->bus_format = MEDIA_BUS_FMT_RGB888_1X7X4_SPWG;
    } else {
        dev_err(dev, "%pOF: invalid or missing %s DT property\n",
            dev->of_node, "data-mapping");
        return -EINVAL;
    }

    serdes_panel->data_mirror = of_property_read_bool(dev->of_node, "data-mirror");
    
    videomode_from_timing(&dt, &vm);
    drm_display_mode_from_videomode(&vm, &serdes_panel->mode);

    return 0;
}

static int serdes_panel_probe(struct platform_device *pdev)
{
    struct serdes *serdes = dev_get_drvdata(pdev->dev.parent);
    struct device *dev = &pdev->dev;
    struct serdes_panel *serdes_panel;
    struct device_node *np = NULL;
    int ret;

    serdes_panel = devm_kzalloc(dev, sizeof(*serdes_panel), GFP_KERNEL);
    if(!serdes_panel)
        return -ENOMEM;

    serdes->serdes_panel = serdes_panel;
    serdes_panel->dev = dev;
    serdes_panel->parent = dev_get_drvdata(dev->parent);
    platform_set_drvdata(pdev, serdes_panel);

    serdes_panel->regmap = dev_get_regmap(dev->parent, NULL);
    if(!serdes_panel->regmap)
        return dev_err_probe(dev, -ENODEV, "Failed o get serdes regmap\n");

    ret = serdes_panel_parse_dt(serdes_panel);
    if(ret)
        return dev_err_probe(dev, ret, "Failed to parser serdes\n");

    np = of_parse_phandle(dev->of_node, "backlight", 0);
    if(np) {
        serdes_panel->backlight = of_find_backlight_by_node(np);
        of_node_put(np);
        if(!serdes_panel->backlight)
            return dev_err_probe(dev, -EPROBE_DEFER, "Failed to get serdes backlight\n");
    }

    if(serdes_panel->parent->chip_data->connector_type) {
        drm_panel_init(&serdes_panel->panel, dev, &serdes_panel_funcs,
                    serdes_panel->parent->chip_data->connector_type);
    } else {
        drm_panel_init(&serdes_panel->panel, dev, &serdes_panel_funcs,
                    DRM_MODE_CONNECTOR_LVDS);
    }
    drm_panel_add(&serdes_panel->panel);

    dev_info(dev, "serdes %s-%s serdes_panel_probe successful\n",
        dev_name(serdes->dev), serdes->chip_data->name);

    return 0;
}

static int serdes_panel_remove(struct platform_device *pdev)
{
    struct serdes_panel *serdes_panel = platform_get_drvdata(pdev);
    struct backlight_device *backlight = serdes_panel->backlight;

    if(backlight)
        put_device(&backlight->dev);

    drm_panel_remove(&serdes_panel->panel);

    return 0;
}

static const struct of_device_id serdes_panel_of_match[] = {
    { .compatible =  "maxim,max96752-panel", },
    { .compatible =  "ti,ds90uh928-panel",   },
    { .compatible =  "ti,ds90uh968-panel",   },
    { .compatible =  "aim,aim916x-panel",    },
    { }
};

static struct platform_driver serdes_panel_driver = {
    .driver = {
        .name = "serdes-panel",
        .of_match_table = of_match_ptr(serdes_panel_of_match),
    },
    .probe = serdes_panel_probe,
    .remove = serdes_panel_remove,
};

static int __init serdes_panel_init(void)
{
    return platform_driver_register(&serdes_panel_driver);
}
device_initcall(serdes_panel_init);

static void __exit serdes_panel_exit(void)
{
    platform_driver_unregister(&serdes_panel_driver);
}
module_exit(serdes_panel_exit);

MODULE_AUTHOR("weigenyin");
MODULE_DESCRIPTION("drm panel interface for different serdes");
MODULE_LICENSE("GPL");
