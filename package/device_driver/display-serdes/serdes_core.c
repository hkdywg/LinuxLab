/*
* serdes_core.c 
*	core define for mfd display arch
*
* @copyright Copyright (c) 2022 Jiangsu New Vision Automotive Electronics Co.，Ltd. All rights reserved.
*
* Author: weigenyin <weigenyin@zjautomotive.com>
*
*/

static const struct mfd_cell serdes_max96781_devs[] = {
    {
        .name = "serdes-pinctrl",
        .of_compatible = "maxim,max96781-pinctrl",
    },
    {
        .name = "serdes-bridge",
        .of_compatible = "maxim,max96781-bridge",
    },
};

static const struct mfd_cell serdes_ds90uh981_devs[] = {
    {
        .name = "serdes-pinctrl",
        .of_compatible = "ti,ds90uh981-pinctrl",
    },
    {
        .name = "serdes-bridge",
        .of_compatible = "ti,ds90uh981-bridge",
    },
};

static const struct mfd_cell serdes_ds90uh983_devs[] = {
    {
        .name = "serdes-pinctrl",
        .of_compatible = "ti,ds90uh983-pinctrl",
    },
    {
        .name = "serdes-bridge",
        .of_compatible = "ti,ds90uh983-bridge",
    },
};

static const struct mfd_cell serdes_aim951x_devs[] = {
    {
        .name = "serdes-pinctrl",
        .of_compatible = "aim,aim951x-pinctrl",
    },
    {
        .name = "serdes-bridge",
        .of_compatible = "aim,aim951x-bridge",
    },
};

int serdes_reg_read(struct serdes *serdes, unsigned int reg, unsigned int *val)
{
    int ret;

    ret = regmap_read(serdes->regmap, reg, val);

    SERDES_DBG_I2C("%s %s %s Read Reg%04x %04x ret = %d\n", __func__, dev_name(serdes->dev),
                   serdes->chip_data->name, reg, *val, ret);

    return ret;
}
EXPORT_SYMBOL_GPL(serdes_reg_read);

int serdes_bulk_read(struct serdes *serdes, unsigned int reg,
                     int count, u16 *buf)
{
    int i = 0, ret = 0;

    ret = regmap_bulk_read(serdes->regmap, reg, buf, count);
    for(i = 0; i < count; i++) {
        SERDES_DBG_I2C("%s %s %s Read Reg%04x %04x ret = %d\n", __func__, dev_name(serdes->dev),
                       serdes->chip_data->name, reg + i, buf[i], ret);
    }

    return ret;
}
EXPORT_SYMBOL_GPL(serdes_bulk_read);

int serdes_reg_write(struct serdes *serdes, unsigned int reg, unsigned int val)
{
    int ret;
    
    ret = regmap_write(serdes->regmap, reg, val);
    SERDES_DBG_I2C("%s %s %s Write Reg%04x %04x ret = %d\n", __func__, dev_name(serdes->dev),
                   serdes->chip_data->name, reg, val, ret);

    return ret;
}
EXPORT_SYMBOL_GPL(serdes_reg_write);


int serdes_multi_reg_write(struct serdes *serdes, const struct reg_sequence *regs,
                           int num_regs)
{
    int i, ret;
    
    SERDES_DBG_I2C("%s %s %s num = %d\n", __func__, dev_name(serdes->dev),
                   serdes->chip_data->name, num_regs);

    ret = regmap_multi_reg_write(serdes->regmap, regs, num_regs);
    for(i = 0; i < num_regs; i++)
        SERDES_DBG_I2C("serdes %s Write Reg%04x %04x ret = %d\n", serdes->chip_data->name, 
                        regs[i].reg, regs[i].def, ret);

    return ret;
}
EXPORT_SYMBOL_GPL(serdes_multi_reg_write);

int serdes_bulk_write(struct serdes *serdes, unsigned int reg, int count, void *src)
{
    u16 *buf = src;
    int i, ret;
    
    SERDES_DBG_I2C("%s %s %s num = %d\n", __func__, dev_name(serdes->dev),
                   serdes->chip_data->name, num_regs);
    mutex_lock(&serdes->io_lock);
    for(i = 0; i < num_regs; i++) {
        ret = regmap_write(serdes->regmap, reg + i, buf[i]);
        SERDES_DBG_I2C("%s %s %s Write Reg%04x %04x ret = %d\n", __func__, dev_name(serdes->dev),
                       serdes->chip_data->name, reg + i, buf[i], ret);
        if(ret != 0) {
            mutex_unlock(&serdes->io_lock);
            return ret;
        }
    }
    mutex_unlock(&serdes->io_lock);

    return ret;
}
EXPORT_SYMBOL_GPL(serdes_bulk_write);

int serdes_set_bit(struct serdes *serdes, unsigned int reg, 
            unsigned int mask, unsigned int val)
{
    int ret;

    SERDES_DBG_I2C("%s %s %s Write Reg%04x %04x mask = %04x\n", __func__, dev_name(serdes->dev),
                   serdes->chip_data->name, reg, mask, val);
    ret = regmap_update_bits(serdes->regmap, reg, mask, val);

    return ret;
}
EXPORT_SYMBOL_GPL(serdes_set_bit);

int serdes_device_init(struct serdes *serdes)
{
    struct serdes_chip_data *chip_data = serdes->chip_data;
    const struct mfd_cell *serdes_devs = NULL;
    int ret, mfd_num;

    switch(chip_data->serdes_id) {
    case MAXIM_ID_MAX96781:
        serdes_devs = serdes_max96781_devs;
        mfd_num = ARRAY_SIZE(serdes_max96781_devs);
        break;
    case TI_ID_DS90UH981:
        serdes_devs = serdes_ds90uh981_devs;
        mfd_num = ARRAY_SIZE(serdes_ds90uh981_devs);
        break;
    case TI_ID_DS90UH983:
        serdes_devs = serdes_ds90uh983_devs;
        mfd_num = ARRAY_SIZE(serdes_ds90uh983_devs);
        break;
    case AIM_ID_AIM951X:
        serdes_devs = serdes_aim951x_devs;
        mfd_num = ARRAY_SIZE(serdes_aim951x_devs);
        break;
    default:
        dev_info(serdes->dev, "unknown device\n");
        break;
    }

    ret = devm_mfd_add_devices(serdes->dev, PLATFORM_DEVID_AUTO, serdes_devs,
                        mfd_num, NULL, 0, NULL);
    if(ret != 0) {
        dev_err(serdes->dev, "Failed to add serdes children\n");
        return ret;
    }

    return 0;
}
EXPORT_SYMBOL_GPL(serdes_device_init);

int serdes_set_pinctrl_default(struct serdes *serdes)
{
    int ret = 0;

    if((!IS_ERR(serdes->pinctrl_node)) && (!IS_ERR(serdes->pins_init))) {
        ret = pinctrl_select_state(serdes->pinctrl_node, serdes->pins_init);
        if(ret)
            dev_err(serdes->dev, "could not set init pins\n");
        SERDES_DBG_MFD("%s: name = %s init\n", __func__, dev_name(serdes->dev));
    }

    return ret;
}
EXPORT_SYMBOL_GPL(serdes_set_pinctrl_default);

int serdes_set_pinctrl_sleep(struct serdes *serdes)
{
    int ret = 0;

    if((!IS_ERR(serdes->pinctrl_node)) && (!IS_ERR(serdes->pins_sleep))) {
        ret = pinctrl_select_state(serdes->pinctrl_node, serdes->pins_sleep);
        if(ret)
            dev_err(serdes->dev, "could not set init pins\n");
        SERDES_DBG_MFD("%s: name = %s init\n", __func__, dev_name(serdes->dev));
    }

    return ret;
}
EXPORT_SYMBOL_GPL(serdes_set_pinctrl_sleep);

int serdes_device_resume(struct serdes *serdes)
{
    int ret = 0;

    if(!IS_ERR(serdes->vpower)) {
        ret = regulator_enable(serdes->vpower);
        if(ret) {
            dev_err(serdes->dev, "failed to enable vpower regulator\n");
            return ret;
        }
    }

    return ret;
}
EXPORT_SYMBOL_GPL(serdes_device_resume);

int serdes_device_suspend(struct serdes *serdes)
{
    int ret = 0;

    if(!IS_ERR(serdes->vpower)) {
        ret = regulator_disable(serdes->vpower);
        if(ret) {
            dev_err(serdes->dev, "failed to disable vpower regulator\n");
            return ret;
        }
    }

    return ret;
}
EXPORT_SYMBOL_GPL(serdes_device_suspend);

void serdes_device_poweroff(struct serdes *serdes)
{
    int ret = 0;

    if((!IS_ERR(serdes->pinctrl_node)) && (!IS_ERR(serdes->pins_sleep))) {
        ret = pinctrl_select_state(serdes->pinctrl_node, serdes->pins_sleep);
        if(ret) 
            dev_err(serdes->dev, "could not set sleep pins\n");
    }

    if(!IS_ERR(serdes->vpower)) {
        ret = regulator_disable(serdes->vpower);
        if(ret) {
            dev_err(serdes->dev, "failed to disable vpower regulator\n");
        }
    }
}
EXPORT_SYMBOL_GPL(serdes_device_poweroff);

int serdes_device_shutdown(struct serdes *serdes)
{
    int ret = 0;

    if(!IS_ERR(serdes->vpower)) {
        ret = regulator_disable(serdes->vpower);
        if(ret) {
            dev_err(serdes->dev, "failed to disable vpower regulator\n");
            return ret;
        }
    }

    return 0;
}

MODULE_LICENSE("GPL");
