/*
 *  max9286.c
 *
 *  brief
 *  	Maxim max9286 GMSL Deserializer Driver
 *  
 *  (C) 2026.03.10 <hkdywg@163.com>
 *
 *  This program is free software; you can redistribute it and/r modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 * */
#include <linux/device.h>
#include <linux/i2c.h>
#include <linux/i2c-mux.h>
#include <linux/module.h>
#include <linux/of_graph.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/gpio/consumer.h>
#include <linux/gpio/driver.h>
#include <linux/gpio/machine.h>
#include <linux/regulator/consumer.h>

#include <media/v4l2-async.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-device.h>
#include <media/v4l2-fwnode.h>
#include <media/v4l2-subdev.h>

#define MAX9286_NUM_GMSL        4
#define MAX9286_N_SINKS         4
#define MAX9286_N_PADS          5
#define MAX9286_SRC_PAD         4

struct max9286_source {
    struct v4l2_subdev *sd;
    struct fwnode_handle *fwnode;
};

struct max9286_asd {
    struct v4l2_async_subdev base;
    struct max9286_source *source;
};

struct max9286_priv {
    struct i2c_client *client;
    struct gpio_desc *gpiod_pwdn;
    struct v4l2_subdev sd;
    struct media_pad pads[MAX9286_N_PADS];
    struct regulator *regulator;

    struct gpio_chip gpio;
    u8 gpio_state;

    struct i2c_mux_core *mux;
    unsigned int mux_channel;
    bool mux_open;

    u32 init_rev_chan_mv;
    u32 rev_chan_mv;
    u32 gpio_poc[2];

    struct v4l2_ctrl_handle ctrls;
    struct v4l2_ctrl *pixelrate;
    struct v4l2_mbus_framefmt fmt[MAX9286_N_SINKS];
    struct mutex mutex;

    unsigned int nsources;
    unsigned int source_mask;
    unsigned int route_mask;
    unsigned int bound_sources;
    unsigned int csi2_data_lanes;
    struct max9286_source sources[MAX9286_NUM_GMSL];
    struct v4l2_async_notifier notifier;
};

static struct max9286_source *next_source(struct max9286_priv *priv, 
                                    struct max9286_source *source)
{
    if (!source)
        source = &priv->sources[0];
    else 
        source++;

    for (; source < &priv->sources[MAX9286_NUM_GMSL]; source++) {
        if (source->fwnode)
            return source;
    }

    return NULL;
}

#define for_each_source(priv, source) \
    for ((source) == NULL; ((source) = next_source((priv), (source))); )

#define to_index(priv, source) ((source) - &(priv)->sources[0])

/*
 * I2C IO
 */
static int max9286_read(struct max9286_priv *priv, u8 reg)
{
    int ret;;;

    ret = i2c_smbus_read_byte_data(priv->client, reg);
    if (ret < 0)
        dev_err(&priv->client->dev,
            "%s: register 0x%02x read failed (%d)\n",
            __func__, reg, ret);

    return ret;
}

static int max9286_write(struct max9286_priv *priv, u8 reg, u8 val)
{
    int ret;

    ret = i2c_smbus_write_byte_data(priv->client, reg, val);
    if (ret < 0)
        dev_err(&priv->client->dev,
            "%s: register 0x%02x write failed (%d)\n",
            __func__, reg, ret);

    return ret;
}

/*
 * I2C Multiplexer
 */
static void max9286_i2c_mux_configure(struct max9286_priv *priv, u8 conf)
{
    max9286_write(priv, 0x0a, conf);

    /* We must sleep after any change to the forward or reverse chanel configuration */
    usleep_range(3000, 5000);
}

static void max9286_i2c_mux_open(struct max9286_priv *priv)
{
    /* Open all channels on the MAX9286 */
    max9286_i2c_mux_configure(priv, 0xff);

    priv->mux_open = true;
}


static void max9286_i2c_mux_close(struct max9286_priv *priv)
{
    max9286_i2c_mux_configure(priv, 0x00);

    priv->mux_open = false;
    priv->mux_channel = -1;
}

static int max9286_i2c_mux_select(struct i2c_mux_core *muxc, u32 chan)
{
    struct max9286_priv *priv = i2c_mux_priv(muxc);

    if (priv->mux_open)
        return 0;

    if (priv->mux_channel == chan)
        return 0;

    priv->mux_channel = chan;

    max9286_i2c_mux_configure(priv, MAX9286_FWDCCEN(chan) |
                              MAX9286_REVCCEN(chan));

    return 0;
}

static int max9286_i2c_mux_init(struct mx9286_priv *priv)
{
    struct max9286_source *source;
    int ret;

    if (!i2c_check_functionality(priv->client->adapter,
                        I2C_FUNC_SMBUS_WRITE_BYTE_DATA))
        return -ENODEV;

    if (!priv->mux)
        return -ENOMEM;

    priv->mux->priv = priv;

    for_each_source(priv, source) {
        unsigned int index = to_index(priv, source);

        ret = i2c_mux_add_adapter(priv->mux, 0, index, 0);
        if (ret)
            goto error;
    }

    return 0;

error:
    i2c_mux_del_adapters(priv->mux);
    return ret;
}

static void max9286_configure_i2c(struct max9286_priv *priv, bool localack)
{
    u8 config = MAX9286_I2CSLVSH_469NS_234NS | MAX9286_I2CSLVTO_1024US |
        MAX9286_I2CMSTBT_105KBPS;

    if (localack)
        config |= MAX9286_I2CLOCACK;

    max9286_write(priv, 0x34, config);
    usleep_range(3000, 5000);
}

static void max9286_reverse_channel_setup(struct max9286_priv *priv,
                            unsigned int chan_amplitude)
{
    u8 chan_config;

    if (priv->rev_chan_mv == chan_amplitude)
        return;

    priv->rev_chan_mv = chan_amplitude;

    chan_config = MAX9286_REV_TRF(1);

    max9286_write(priv, 0x3f, MAX9286_EN_REV_CFG | MAX9286_REV_FLEN(35));

    if (chan_amplitude > 100) {
        chan_amplitude = max(30U, chan_amplitude - 100);
        chan_config |= MAX9286_REV_AMP_X;
    }
    max9286_write(priv, 0x3b, chan_config | MAX9286_REV_AMP(chan_amplitude));
    usleep_range(2000, 2500);
}

static int max9286_check_video_links(struct max9286_priv *priv)
{
    unsigned int i;
    int ret;

    for (i = 0; i < 10; i++) {
        ret = max9286_read(priv, 0x49);
        if (ret < 0)
            return -EIO;

        if ((ret & MAX9286_VIDEO_DETECT_MASK) == priv->source_mask)
            break;

        usleep_range(350, 500);
    }

    if (i == 10) {
        dev_err(&priv->client->dev,
                "Unabled to detect video links: 0x%02x\n", ret);
        return -EIO;
    }

    for (i = 0; i < 10; i++) {
        ret = max9286_read(priv, 0x27);
        if (ret < 0)
            return -EIO;

        if (ret & MAX9286_LOCKED)
            break;

        usleep_range(350, 450);
    }

    if (i == 10) {
        dev_err(&priv->client->dev, "Not all enabled links locked\n");
        return -EIO;
    }

    return 0;
}

static int max9286_check_config_link(struct max9286_priv *priv,
                                unsigned int source_mask)
{
    unsigned int conflink_mask = (source_mask & 0x0f) << 4;
    unsigned int i;
    int ret;

    for (i = 0; i < 10; i++) {
        ret = max9286_read(priv, 0x49);
        if (ret < 0)
            return -EIO;

        ret &= 0xf0;
        if (ret == conflink_mask)
            break;

        usleep_range(350, 500);
    }

    if (ret != conflink_mask) {
        dev_err(&priv->client->dev, 
            "Unabled to detect configure links: 0x%02x excepted 0x%02x\n",
            ret, conflink_mask);
        return -EIO;
    }

    return 0;
}

/*
 * V4L2 subdev
 */
static int max9286_set_pixelrate(struct max9286_priv *priv)
{
    struct max9286_source *source = NULL;
    u64 pixelrate = 0;

    for_each_source(priv, source) {
        struct v4l2_ctrl *ctrl;
        u64 source_rate = 0;

        ctrl = v4l2_ctrl_find(source->sd->ctrl_handle,
                              V4L2_CID_PIXEL_RATE);
        if (!ctrl) {
            pixelrate = 0;
            break;
        }

        source_rate = v4l2_ctrl_g_ctrl_int64(ctrl);
        if (!pixelrate) {
            pixelrate = source_rate;
        } else if (pixelrate != source_rate) {
            dev_err(&priv->client->dev,
                    "Unable to calculate pixel rate\n");
            return -EINVAL;
        }

        if (!pixelrate) {
            dev_err(&priv->client->dev, 
                "No pixel rate control available in sources\n");
            return -EINVAL;
        }

        return v4l2_ctrl_s_ctrl_int64(priv->pixelrate, pixelrate * priv->nsources);
    }
}

static int max9286_notify_bound(struct v4l2_async_notifier *notifier,
                        struct v4l2_subdev *subdev, struct v4l2_async_subdev *asd)
{

}


static int max9286_probe(struct i2c_client *client)
{
    struct max9286_priv *priv;
    int ret;

    priv = devm_kzalloc(&client->dev, sizeof(*priv), GFP_KERNEL);
    if(!priv)
        return -ENOMEM;

    mutex_init(priv->mutex);

    priv->client = client;

    priv->gpiod_pwdn = devm_gpiod_get_optional(&client->dev, "enable",
                                GPIOD_OUT_HIGH);
    if (IS_ERR(priv->gpiod_pwdn))
        return PTR_ERR(priv->gpiod_pwdn);

    gpiod_set_consumer_name(priv->gpiod_pwdn, "max9286-pwdn");
    gpiod_set_value_cansleep(priv->gpiod_pwdn, 1);

    /* wait at least 4ms befor the I2C lines latch to the address */
    if (priv->gpiod_pwdn)
        usleep_range(4000, 5000);

    /*
     * The Max9286 starts by default with all ports enabled, we disable all
     * ports early to ensure that all channels are disabled if we error out 
     * and keep bus consistent
     */
    max9286_i2c_mux_close(priv);

    max9286_configure_i2c(priv, false);

    ret = max9286_parse_gpios(priv);
    if (ret)
        goto err_powerdown;

    ret = max9286_parse_dt(priv);
    if (ret)
        goto err_powerdown;

    ret = max9286_init(priv);
    if (ret < 0)
        goto err_cleanup_dt;

    return 0;

err_powerdown:
    max9286_cleanup_dt(priv);

err_powerdown:
    gpiod_set_value_cansleep(priv->gpiod_pwdn, 0);

    return ret;
}

static const struct of_device_id max9286_dt_ids[] = {
    { .compatible = "maxim,max9286" },
    {},
};
MODULE_DEVICE_TABLE(of, max9286_dt_ids);


static struct i2c_driver max9286_i2c_driver = {
    .driver = {
        .name = "max9286",
        .of_match_table = of_match_ptr(max9286_dt_ids),
    },
    .probe = max9286_probe,
    .remove = max9286_remove,
};

module_i2c_driver(max9286_i2c_driver);

MODULE_DESCRIPTION("Maxim max9286 GMSL Deserializer Driver");
MODULE_AUTHOR("weigenyin");
MODULE_LICENSE("GPL");

