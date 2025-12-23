/*
* maxim_max96781.c 
*	driver for max96781
*
* @copyright Copyright (c) 2022 Jiangsu New Vision Automotive Electronics Co.，Ltd. All rights reserved.
*
* Author: weigenyin <weigenyin@zjautomotive.com>
*
*/
#include "../display_serdes_core.h"
#include "maxim_max96781.h"

static int max96781_pinctrl_set_mux(struct serdes *serdes,
                        unsigned int function, unsigned int group)
{
    struct serdes_pinctrl *pinctrl = serdes->pinctrl;
    struct function_desc *func;
    struct group_desc *grp;
    int i;

    func = pinmux_generic_get_function(pinctrl->pctl, function);
    if(!func)
        return -EINVAL;

    grp = pinctrl_generic_get_group(pinctrl->pctl, group);
    if(!grp)
        return -EINVAL;

    SERDES_DBG_CHIP("%s: serdes chip %s func = %s data = %p group = %s data = %p, num_pin = %d",
            __func__, serdes->chip_data->name,
            func->name, func->data, grp->name, grp->data, grp->num_pins);

    if(func->data) {
        struct serdes_function_data *data = func->data;

        for(i = 0; i < grp->num_pins; i++) {
            serdes_set_bits(serdes,
                            GPIO_A_REG(grp->pins[i] - pinctrl->pin_base),
                            GPIO_OUT_DIS,
                            FIELD_PREP(GPIO_OUT_DIS, data->gpio_out_dis));
            serdes_set_bits(serdes,
                            GPIO_B_REG(grp->pins[i] - pinctrl->pin_base),
                            OUT_TYPE,
                            FIELD_PREP(OUT_TYPE, 1));
            if(data->gpio_tx_en)
                serdes_set_bits(serdes,
                                GPIO_B_REG(grp->pins[i] - pinctrl->pin_base),
                                GPIO_TX_ID,
                                FIELD_PREP(GPIO_TX_ID, data->gpio_tx_id));
            if(data->gpio_rx_en)
                serdes_set_bits(serdes,
                                GPIO_C_REG(grp->pins[i] - pinctrl->pin_base),
                                GPIO_TX_ID,
                                FIELD_PREP(GPIO_RX_ID, data->gpio_rx_id));
            serdes_set_bits(serdes,
                            GPIO_D_REG(grp->pins[i] - pinctrl->pin_base),
                            GPIO_TX_EN_A | GPIO_RX_EN_A,
                            FIELD_PREP(GPIO_TX_EN_A, data->gpio_tx_en_a) |
                            FIELD_PREP(GPIO_RX_EN_A, data->gpio_rx_en_a) |
                            FIELD_PREP(GPIO_IO_RX_EN, data->gpio_io_rx_en));
        }
    }

    return 0;
}

static int max96781_pinctrl_config_get(struct serdes *serdes,
                        unsigned int pin, unsigned long *config)
{
    enum pin_config_param param = pinconf_to_config_param(*config);
    unsigned int gpio_a_reg, gpio_b_reg;
    u16 arg;

    serdes_reg_read(serdes, GPIO_A_REG(pin), &gpio_a_reg);
    serdes_reg_read(serdes, GPIO_B_REG(pin), &gpio_b_reg);

    SEDES_DBG_CHIP("%s: serdes chip %s pin = %d param = %d\n", __func__,
                   serdes->chip_data->name, pin, param);

    switch(param) {
    case PIN_CONFIG_DRIVE_OPEN_DRAIN:
        if(FIELD_GET(OUT_TYPE, gpio_b_reg))
            return -EINVAL;
        break;
    case PIN_CONFIG_DRIVE_PUSH_PULL:
        if(!FIELD_GET(OUT_TYPE, gpio_b_reg))
            return -EINVAL;
        break;
    case PIN_CONFIG_BIAS_DISABLE:
        if(FIELD_GET(PULL_UPDN_SEL, gpio_b_reg) != 0)
            return -EINVAL;
        break;
    case PIN_CONFIG_BIAS_PULL_UP:
        if(FIELD_GET(PULL_UPDN_SEL, gpio_b_reg) != 1)
            return -EINVAL;
        switch(FIELD_GET(REG_CFG, gpio_a_reg)) {
        case 0:
            arg = 40000;
            break;
        case 1:
            arg = 10000;
            break;
        }
        break;
    case PIN_CONFIG_BIAS_PULL_DOWN:
        if(FIELD_GET(PULL_UPDN_SEL, gpio_b_reg) != 2)
            return -EINVAL;
        switch(FIELD_GET(REG_CFG, gpio_a_reg)) {
        case 0:
            arg = 40000;
            break;
        case 1:
            arg = 10000;
            break;
        }
        break;
    case PIN_CONFIG_OUTPUT:
        if(FIELD_GET(GPIO_OUT_DIS, gpio_a_reg))
            return -EINVAL;
        arg = FIELD_GET(GPIO_OUT, gpio_a_reg);
        break;
    default:
        return -EOPNOTSUPP;
    }

    *config = pinconf_to_config_packed(param, arg);

    return 0;
}

static struct serdes_chip_pinctrl_ops max96781_pinctrl_ops = {
    .pin_config_get = max96781_pinctrl_config_get,
    .pin_config_set = max96781_pinctrl_config_set,
    .set_mux        = max96781_pinctrl_set_mux,
};

static int max96781_gpio_direction_input(struct serdes *serdes, int gpio)
{
    return 0;
}

static int max96781_gpio_direction_ouput(struct serdes *serdes, int gpio, int value)
{
    return 0;
}

static int max96781_gpio_get_level(struct serdes *serdes, int gpio)
{
    return 0;
}

static int max96781_gpio_set_level(struct serdes *serdes, int gpio, int value)
{
    return 0;
}

static int max96781_gpio_set_config(struct serdes *serdes, int gpio, unsigned long config)
{
    return 0;
}

static int max96781_gpio_to_irq(struct serdes *serdes, int gpio)
{
    return 0;
}

static struct serdes_chip_gpio_ops max96781_gpio_ops = {
    .direction_input        = max96781_gpio_direction_input,
    .direction_output       = max96781_gpio_direction_ouput,
    .get_level              = max96781_gpio_get_level,
    .set_level              = max96781_gpio_set_level,
    .set_config             = max96781_gpio_set_config,
    .to_irq                 = max96781_gpio_to_irq,
};

static const struct check_reg_data max96781_important_reg[10] = {
    {
        "MAX96781 LINK LOCK",
        { 0x0013, (1 << 3) },
    },
    {
        "MAX96781 LINKA LOCK",
        { 0x002A, (1 << 3) },
    },
    {
        "MAX96781 X PLCK DET",
        { 0x0102, (1 << 7) },
    },
    {
        "MAX96781 Y PLCK DET",
        { 0x0112, (1 << 7) },
    },
};

static int max96781_check_reg(struct serdes *serdes)
{
    int i, ret;
    unsigned int val;

    for(i = 0; i < ARRAY_SIZE(max96781_important_reg); i++) {
        if(!max96781_important_reg[i].seq.reg)
            break;

        ret = serdes_read_reg(serdes, max96781_important_reg[i].seq.reg, &val);
        if(!ret && !(val & max96781_important_reg[i].seq.def)
           && (!atomic_read(&serdes->flag_early_suspend)))
            dev_info(serdes->dev, "warning %s %s reg[0x%x] = 0x%x\n", __func__,
                     max96781_important_reg[i].name,
                     max96781_important_reg[i].seq.reg, val);
            
    }

    return 0;
}

static struct serdes_check_reg_ops max96781_check_reg_ops = {
    .check_reg = max96781_check_reg,
};

static int  max96781_pm_suspend(struct serdes *serdes)
{
    return 0;
}

static int  max96781_pm_resume(struct serdes *serdes)
{
    return 0;
}

static struct serdes_chip_pm_ops max96781_pm_ops = {
    .suspend = max96781_pm_suspend,
    .resume  = max96781_pm_resume,
};

static int max96781_irq_lock_handle(struct serdes *serdes)
{
    return IRQ_HANDLE;
}

static int max96781_irq_err_handle(struct serdes *serdes)
{
    return IRQ_HANDLE;
}

static struct serdes_chip_irq_ops max96781_irq_ops = {
    .lock_handle = max96781_irq_lock_handle,
    .err_handle  = max96781_irq_err_handle,
};


struct serdes_chip_data serdes_max96781_data = {
    .name               = "max96781",
    .serdes_type        = TYPE_SER,
    .serdes_id          = MAXIM_ID_MAX96781,
    .connector_type     = DRM_MODE_CONNECTOR_LVDS,
    .regmap_config      = &max96781_regmap_config,
    .pinctrl_config     = &max96781_pinctrl_config,
    .bridge_ops         = &max96781_bridge_ops,
    .pinctrl_ops        = &max96781_pinctrl_ops,
    .gpio_ops           = &max96781_gpio_ops,
    .check_ops          = &max96781_check_reg_ops,
    .pm_ops             = &max96781_pm_ops,
    .irq_ops            = &max96781_irq_ops,
};
EXPORT_SYMBOL_GPL(serdes_max96781_data);

MODULE_LICENSE("GPL");
