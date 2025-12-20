/*
* serdes_core.c 
*	core define for mfd display arch
*
* @copyright Copyright (c) 2022 Jiangsu New Vision Automotive Electronics Co.，Ltd. All rights reserved.
*
* Author: weigenyin <weigenyin@zjautomotive.com>
*
*/
#include "display_serdes_core.h"

static void serdes_i2c_shutdown(struct i2c_client *client)
{
    struct device *dev = &client->dev;
    struct serdes *serdes = dev_get_drvdata(dev);

    serdes_device_shutdown(serdes);
}

static void serdes_i2c_remove(struct i2c_client *client)
{
    struct device *dev = &client->dev;
    struct serdes *serdes = dev_get_drvdata(dev);

    if(serdes->use_reg_check_work)
        serdes_reg_check_work_free(serdes);

    if(serdes->use_delay_work) {
        cancel_delayed_work_sync(&serdes->mfd_delay_work);
        destroy_workqueue(serdes->mfd_wq);
    }
}

static int serdes_i2c_prepare(struct device *dev)
{
    struct serdes *serdes = dev_get_drvdata(dev);

    atomic_set(&serdes->flag_early_suspend, 1);

    return 0;
}

static int serdes_i2c_resume(struct device *dev)
{
    struct serdes *serdes = dev_get_drvdata(dev);

    if(serdes->chip_data->serdes_type == TYPE_OTHER)
        serdes_i2c_set_sequence(serdes);

    serdes_device_resume(serdes);

    return 0;
}

static int serdes_i2c_poweroff(struct device *dev)
{
    struct serdes *serdes = dev_get_drvdata(dev);

    serdes_device_poweroff(serdes);

    return 0;
}

static int serdes_i2c_set_sequence(struct serdes *serdes)
{
    struct device *dev = serdes->dev;
    int i, ret;
    unsigned int def;

    for(i = 0; i < serdes->serdes_init_seq->reg_seq_cnt; i++) {
        if(serdes->serdes_init_seq->reg_sequence[i].reg == 0xffff) {
            udelay(serdes->serdes_init_seq->reg_sequence[i].def);
            continue;
        }

        ret = serdes_reg_write(serdes,
                        serdes->serdes_init_seq->reg_sequence[i].reg,
                        serdes->serdes_init_seq->reg_sequence[i].def);
        if(ret < 0) {
            dev_warn(dev, "%s failed to write reg %04x, ret %d, again now\n",
                     dev_name(serdes->dev),
                     serdes->serdes_init_seq->reg_sequence[i].reg, ret);
            ret = serdes_reg_write(serdes,
                            serdes->serdes_init_seq->reg_sequence[i].reg,
                            serdes->serdes_init_seq->reg_sequence[i].def);
        }
        serdes_reg_read(serdes, serdes->serdes_init_seq->reg_sequence[i].reg, &def);
        if((def !=  serdes->serdes_init_seq->reg_sequence[i].def) || (ret < 0)) {
            /* if read value != write value then write again */
            dev_err(dev, "read %04x %04x != %04x\n",
                    serdes->serdes_init_seq->reg_sequence[i].reg,
                    def, serdes->serdes_init_seq->reg_sequence[i].def);
            ret = serdes_reg_write(serdes,
                            serdes->serdes_init_seq->reg_sequence[i].reg,
                            serdes->serdes_init_seq->reg_sequence[i].def);
        }
    }

    dev_info(dev, "serdes %s sequence_init\n", serdes->chip_data->name);

    return ret;
}

static int serdes_i2c_set_sequence_backup(struct serdes *serdes)
{
    struct device *dev = serdes->dev;
    int i, ret;
    unsigned int def;

    for(i = 0; i < serdes->serdes_backup_seq->reg_seq_cnt; i++) {
        if(serdes->serdes_backup_seq->reg_sequence[i].reg == 0xffff) {
            udelay(serdes->serdes_backup_seq->reg_sequence[i].def);
            continue;
        }

        ret = serdes_reg_write(serdes,
                        serdes->serdes_backup_seq->reg_sequence[i].reg,
                        serdes->serdes_backup_seq->reg_sequence[i].def);
        if(ret < 0) {
            dev_warn(dev, "%s failed to write reg %04x, ret %d, again now\n",
                     dev_name(serdes->dev),
                     serdes->serdes_backup_seq->reg_sequence[i].reg, ret);
            ret = serdes_reg_write(serdes,
                            serdes->serdes_backup_seq->reg_sequence[i].reg,
                            serdes->serdes_backup_seq->reg_sequence[i].def);
        }
        serdes_reg_read(serdes, serdes->serdes_backup_seq->reg_sequence[i].reg, &def);
        if((def !=  serdes->serdes_backup_seq->reg_sequence[i].def) || (ret < 0)) {
            /* if read value != write value then write again */
            dev_err(dev, "read %04x %04x != %04x\n",
                    serdes->serdes_backup_seq->reg_sequence[i].reg,
                    def, serdes->serdes_backup_seq->reg_sequence[i].def);
            ret = serdes_reg_write(serdes,
                            serdes->serdes_backup_seq->reg_sequence[i].reg,
                            serdes->serdes_backup_seq->reg_sequence[i].def);
        }
    }

    dev_info(dev, "serdes %s sequence_init\n", serdes->chip_data->name);

    return ret;
}

static serdes_i2c_backup_register(struct serdes *serdes)
{
    int i;

    for(i = 0; i < serdes->serdes_backup_seq->reg_seq_cnt; i++) {
        if(serdes->serdes_backup_seq->req_sequence[i].reg == 0xffff)
            continue;
        serdes_reg_read(serdes, serdes->serdes_backup_seq->reg_sequence[i].reg,
                        &serdes->serdes_backup_seq->reg_sequence[i].def);
    }

    return 0;
}

static int serdes_set_i2c_address(struct serdes *serdes, u32 reg_hw, 
                                  u32 reg_use, int link)
{
    int ret = 0;
    struct i2c_client *client_split;
    struct serdes *serdes_split = serdes->g_serdes_bridge_split;
    unsigned int def = 0;
    
    if(!serdes_split) {
        dev_info(serdes->dev, "%s serdes_split is null\n", __func__);
        return -EPROBE_DEFER;
    }

    client_split = to_i2c_client(serdes->regmap->dev);
    client_split->addr = serdes->reg_use;
    ret = serdes_reg_read(serdes, serdes->serdes_init_seq->reg_sequence[0].reg, &def);
    if(ret) {
        client_split->addr = serdes->reg_hw;
        dev_info(serdes->dev, "%s try to use addr 0x%x\n", __func__, serdes->reg_hw);
    }

    if(serdes_split->chip_data->split_ops && serdes_split->chip_data->split_ops->select)
        ret = serdes_split->chip_data->split_ops->select(serdes_split, link);

    if(serdes_split->chip_data->split_ops && serdes_split->chip_data->split_ops->set_i2c_addr)
        ret = serdes_split->chip_data->split_ops->set_i2c_addr(serdes, reg_use, link);

    if(serdes_split->chip_data->split_ops && serdes_split->chip_data->split_ops->select)
        ret = serdes_split->chip_data->split_ops->select(serdes_split, SER_SPLITTER_MODE);

    client_split->addr = serdes->reg_use;

    return ret;
}

static int serdes_i2c_check_register(struct serdes *serdes, int *flag)
{
    struct device *dev = serdes->dev;
    int ret = 0;
    unsigned int def = 0;
    unsigned int num = 0;

    get_random_bytes(&num, 1);
    if(num > serdes->serdes_backup_seq->reg_seq_cnt - 1)
        num = 0;

    if(serdes->serdes_backup_seq->reg_sequence[num].reg == 0xffff)
        return 0;

    ret = serdes_reg_read(serdes, serdes->serdes_backup_seq->reg_sequence[num].reg, &def);
    if((def != serdes->serdes_backup_seq->reg_sequence[num].def) || (ret < 0)) {
        dev_err(dev, "%s read %04x %04x != %04x\n", __func__,
                serdes->serdes_backup_seq->reg_sequence[num].reg,
                def, serdes->serdes_backup_seq->reg_sequence[num].def);
        *flag = 1;
        return ret;
    }

    if(serdes->chip_data->check_ops->check_reg)
        ret = serdes->chip_data->check_ops->check_reg(serdes);

    return ret;
}

static void serdes_reg_check_work(struct kthread_work *work)
{
    int flag = 0;
    struct serdes *serdes = container_of(work, struct serdes,
                                         reg_check_work.work);

    if(atomic_read(&serdes->flag_ser_init)) {
        serdes_i2c_backup_register(serdes);
        atomic_set(&serdes->flag_ser_init, 0);
    }
    
    serdes_i2c_check_register(serdes, &flag);
    if(flag) {
        if(serdes->chip_data->chip_init)
            serdes->chip_data->chip_init(serdes);
        serdes_i2c_set_sequence_backup(serdes);
        msleep(500);
    }
    kthread_queue_delayed_work(serdes->kworker, &serdes->reg_check_work,
                               msecs_to_jiffies(2000));
}

static int serdes_reg_check_work_setup(struct serdes *serdes)
{
    kthread_init_delayed_work(&serdes->reg_check_work,
                              serdes_reg_check_work);

    serdes->kworker = kthread_create_worker(0, "%s", dev_name(serdes->dev));
    if(IS_ERR(serdes->kworker))
        return PTR_ERR(serdes->kworker);
    mutex_init(&serdes->reg_check_lock);
    atomic_set(&serdes->flag_ser_init, 1);
    atomic_set(&serdes->flag_early_suspend, 0);
    kthread_queue_delayed_work(serdes->kworker, &serdes->reg_check_work,
                               msecs_to_jiffies(20000));
}

static void serdes_reg_check_work_free(struct serdes *serdes)
{
    kthread_cancel_delayed_work_sync(&serdes->reg_check_work);
    kthread_destroy_worker(serdes->kworker);
}

static void serdes_mfd_work(struct work_struct *work)
{
    struct serdes *serdes = container_of(work, struct serdes, mfd_delay_work.work);

    mutex_lock(&serdes->wq_lock);
    serdes_device_init(serdes);
    mutex_unlock(&serdes->wq_lock);
}

static const unsigned int serdes_cable[] = {
    EXTCON_JACK_VIDEO_OUT,
    EXTCON_NONE,
};

static int serdes_parse_init_seq(struct device *dev, const u16 *data,
                    int length, struct serdes_init_seq *seq)
{
    struct reg_sequence *reg_sequence;
    u16 *buf, *d;
    unsigned int i, cnt;

    if(!seq)
        return -EINVAL;

    buf = devm_kmemdup(dev, data, length, GFP_KERNEL);
    if(!buf)
        return -ENOMEM;

    d = buf;
    cnt = length / 4;
    seq->reg_seq_cnt = cnt;

    seq->reg_sequence = devm_kcalloc(dev, cnt, sizeof(struct reg_sequence), GPF_KERNEL);
    if(!seq->reg_sequence)
        return -ENOMEM;

    for(i = 0; i < cnt; i++) {
        reg_sequence = &seq->reg_sequence[i];
        reg_sequence->reg = get_unaligned_be16(&d[0]);
        reg_sequence->def = get_unaligned_be16(&d[1]);
        d += 2;
    }

    return 0;
}

static int serdes_get_init_seq(struct serdes *serdes)
{
    struct device *dev = serdes->dev;
    struct device_node *np = dev->of_node;
    const void *data;
    int err, len, ret;

    data = of_get_property(np, "serdes-init-sequence", &len);
    if(!data) {
        dev_err(dev, "Failed to get serdes-init-sequence\n");
        return -EINVAL;
    }

    serdes->serdes_init_seq = devm_kzalloc(dev, sizeof(*serdes->serdes_init_seq),
                                           GFP_KERNEL);
    if(!serdes->serdes_init_seq)
        return -ENOMEM;

    err = serdes_parse_init_seq(dev, data, len, serdes->serdes_init_seq);
    if(err) {
        dev_err(dev, "Failed to parse serdes-init-sequence\n");
        return err;
    }

    serdes->serdes_backup_seq = devm_kzalloc(dev, sizeof(*serdes->serdes_backup_seq),
                                             GFP_KERNEL);
    if(!serdes->serdes_backup_seq)
        return -ENOMEM;

    err = serdes_parse_init_seq(dev, data, len, serdes->serdes_backup_seq);
    if(err) {
        dev_err(dev, "Failed to parse serdes-backup-seq\n");
        return err;
    }

    if(serdes->chip_data->serdes_type == TYPE_SER) {
        if(serdes->chip_data->chip_init)
            serdes->chip_data->chip_init(serdes);
        ret = serdes_i2c_set_sequence(serdes);
    }

    return ret;
}

static int serdes_i2c_probe(struct i2c_client *client,
                    const struct i2c_device_id *id)
{
    struct device *dev = &client->dev;
    struct serdes *serdes;
    int ret;

    serdes = devm_kzalloc(&client->dev, sizeof(struct serdes), GFP_KERNEL);
    if(serdes == NULL)
        return -ENOMEM;

    serdes->dev = dev;
    serdes->chip_data = (struct serdes_chip_data *)of_device_get_match_data(dev);
    i2c_set_clientdata(client, serdes);

    dev_info(dev, "serdes %s probe start, id = %d\n", serdes->chip_data->name,
             serdes->chip_data->serdes_id);

    serdes->type = serdes->chip_data->serdes_type;
    serdes->regmap = devm_regmap_init_i2c(client, serdes->chip_data->regmap_config); 
    if(IS_ERR(serdes->regmap)) {
        ret = PTR_ERR(serdes->regmap);
        dev_err(serdes->dev, "Failed to allocate serdes register map: %d", ret);
        return ret;
    }

    serdes->reset_gpio = devm_gpiod_get_optional(dev, "reset", GPIOD_ASIS);
    if(IS_ERR(serdes->reset_gpio))
        return dev_err_probe(dev, PTR_ERR(serdes->reset_gpio),
                             "Failed to acquire serdes reset gpio\n");

    serdes->enable_gpio = devm_gpiod_get_optional(dev, "enable", GPIOD_ASIS);
    if(IS_ERR(serdes->enable_gpio))
        return dev_err_probe(dev, PTR_ERR(serdes->enable_gpio),
                             "Failed to acquire serdes enable gpio\n");

    serdes->vpower = devm_regulator_get_optional(dev, "vpower");
    if(IS_ERR(serdes->vpower)) {
        if(PTR_ERR(serdes->vpower) != -ENODEV)
            return PTR_ERR(serdes->vpower);
    } else {
        ret = regulator_enable(serdes->vpower);
        if(ret) {
            dev_err(dev, "Failed to enable vpower regulator\n");
            return ret;
        }
    }

    serdes->extcon = devm_extcon_dev_allocate(dev, serdes_cable);
    if(IS_ERR(serdes->extcon))
        return dev_err_probe(dev, PTR_ERR(serdes->extcon),
                             "Failed to allocate serdes extcon device\n");

    ret = devm_extcon_dev_register(dev, serdes->extcon);
    if(ret)
        return dev_err_probe(dev, ret, 
                             "Failed to register serdes extcon device\n");

    ret = serdes_get_init_seq(serdes);
    if(ret)
        dev_err(dev, "Failed to write serdes register with i2c\n");

    mutex_init(&serdes->io_lock);
    dev_set_drvdata(serdes->dev, serdes);
    ret = serdes_irq_init(serdes);
    if(ret != 0) {
        serdes_irq_exit(serdes);
        return ret;
    }

    serdes->use_delay_work = of_property_read_bool(dev->of_node, "user-delay-work");
    if(serdes->use_delay_work) {
        serdes->mfd_wq = alloc_ordered_workqueue("%s",
                                        WQ_MEM_RECLAIM | WQ_FREEZABLE,
                                        "serdes-mfd-wq");
        mutex_init(&serdes->wq_lock);
        INIT_DELAYED_WORK(&serdes->mfd_delay_work, serdes_mfd_work);
        queue_delayed_work(serdes->mfd_wq, &serdes->mfd_delay_work, msecs_to_jiffies(300));
    } else {
        serdes_device_init(serdes);
    }

    serdes->use_reg_check_work = of_property_read_bool(dev->of_node, "user-reg-check_work");
    if(serdes->use_reg_check_work) {
        serdes_reg_check_work_setup(serdes);
    }

    dev_info(dev, "serdes %s serdes_i2c_probe successful version %s\n",
             serdes->chip_data->name, "v1.0");

    return 0;
}


static const struct of_device_id serdes_of_match = {
    { .compatible = "maxim,max96781",   .data = &serdes_max96781_data   },
    { .compatible = "ti,ds90uh981",     .data = &serdes_ds90uh981_data  },
    { .compatible = "ti,ds90uh983",     .data = &serdes_ds90uh983_data  },
    { .compatible = "aim,aim951X",      .data = &serdes_aim951x_data    },
    {  }
};

static const struct dev_pm_ops serdes_pm_ops = {
    .prepare = serdes_i2c_prepare,
    .complete = serdes_i2c_complete,
    .suspend = serdes_i2c_suspend,
    .resume = serdes_i2c_resume,
    .poweroff = serdes_i2c_poweroff,
};

static struct i2c_driver serdes_i2c_driver = {
    .driver = {
        .name = "serdes",
        .pm = &serdes_pm_ops,
        .of_match_table = of_match_ptr(serdes_of_match),
    },
    .probe = serdes_i2c_probe,
    .shutdown = serdes_i2c_shutdown,
    .remove = (void *)serdes_i2c_remove,
};

static int __init serdes_i2c_init(void)
{
    int ret;

    ret = i2c_add_driver(&serdes_i2c_driver);
    if(ret != 0)
        pr_err("Failed to register serdes I2C driver: %d\n", ret);

    return ret;
}

subsys_initcall(serdes_i2c_init);

MODULE_AUTHOR("weigenyin");
MODULE_DESCRIPTION("The AIM951X Serializer driver");
MODULE_LICENSE("GPL");

