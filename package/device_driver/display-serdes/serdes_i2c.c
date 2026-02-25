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

static struct serdes *g_serdes_ser_split[MAX_NUM_SERDES_SPLIT];

int serdes_write_init_seq(struct serdes *serdes, struct serdes_init_seq *seq)
{
    struct device *dev = serdes->dev;
    int i, ret = 0;
    unsigned int def;

    if (!seq)
        return 0;

    for(i = 0; i < seq->reg_seq_cnt; i++) {
        if(seq->reg_sequence[i].reg == 0xffff) {
            udelay(seq->reg_sequence[i].def);
            continue;
        }

        ret = serdes_reg_write(serdes,
                        seq->reg_sequence[i].reg,
                        seq->reg_sequence[i].def);
        if(ret < 0) {
            dev_warn(dev, "%s failed to write reg %04x, ret %d, again now\n",
                     dev_name(serdes->dev),
                     seq->reg_sequence[i].reg, ret);
            ret = serdes_reg_write(serdes,
                            seq->reg_sequence[i].reg,
                            seq->reg_sequence[i].def);
        }
        serdes_reg_read(serdes, seq->reg_sequence[i].reg, &def);
        if((def !=  seq->reg_sequence[i].def) || (ret < 0)) {
            /* if read value != write value then write again */
            dev_err(dev, "read %04x %04x != %04x\n",
                    seq->reg_sequence[i].reg,
                    def, seq->reg_sequence[i].def);
            ret = serdes_reg_write(serdes,
                            seq->reg_sequence[i].reg,
                            seq->reg_sequence[i].def);
        }
    }

    return ret;
}
EXPORT_SYMBOL_GPL(serdes_write_init_seq);

int serdes_i2c_set_sequence(struct serdes *serdes)
{
    struct device *dev = serdes->dev;
    int ret;

    ret = serdes_write_init_seq(serdes, serdes->serdes_init_seq);

    dev_info(dev, "serdes %s sequence_init\n", serdes->chip_data->name);

    return ret;
}

void serdes_i2c_shutdown(struct i2c_client *client)
{
    struct device *dev = &client->dev;
    struct serdes *serdes = dev_get_drvdata(dev);

    serdes_device_shutdown(serdes);
}
EXPORT_SYMBOL_GPL(serdes_i2c_shutdown);

static void serdes_reg_check_work_free(struct serdes *serdes)
{
    kthread_cancel_delayed_work_sync(&serdes->reg_check_work);
    kthread_destroy_worker(serdes->kworker);
}

int serdes_i2c_remove(struct i2c_client *client)
{
    struct device *dev = &client->dev;
    struct serdes *serdes = dev_get_drvdata(dev);

    if(serdes->use_reg_check_work)
        serdes_reg_check_work_free(serdes);

    if(serdes->use_delay_work) {
        cancel_delayed_work_sync(&serdes->mfd_delay_work);
        destroy_workqueue(serdes->mfd_wq);
    }

    return 0;
}
EXPORT_SYMBOL_GPL(serdes_i2c_remove);

static int serdes_i2c_prepare(struct device *dev)
{
    struct serdes *serdes = dev_get_drvdata(dev);

    atomic_set(&serdes->flag_early_suspend, 1);

    return 0;
}

static void serdes_i2c_complete(struct device *dev)
{
    struct serdes *serdes = dev_get_drvdata(dev);

    if (serdes->chip_data->serdes_type == TYPE_SER)
        serdes_i2c_set_sequence(serdes);

    atomic_set(&serdes->flag_early_suspend, 0);
}

static int serdes_i2c_suspend(struct device *dev)
{
    struct serdes *serdes = dev_get_drvdata(dev);

    serdes_device_suspend(serdes);

    SERDES_DBG_MFD("%s: name=%s\n", __func__, dev_name(serdes->dev));
    return 0;
}

static int serdes_i2c_poweroff(struct device *dev)
{
    struct serdes *serdes = dev_get_drvdata(dev);

    serdes_device_poweroff(serdes);

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

static int serdes_i2c_set_sequence_backup(struct serdes *serdes)
{
    struct device *dev = serdes->dev;
    int ret;

    ret = serdes_write_init_seq(serdes, serdes->serdes_backup_seq);

    dev_info(dev, "serdes %s sequence_backup\n", serdes->chip_data->name);

    return ret;
}

static int serdes_i2c_backup_register(struct serdes *serdes)
{
    int i;

    for(i = 0; i < serdes->serdes_backup_seq->reg_seq_cnt; i++) {
        if(serdes->serdes_backup_seq->reg_sequence[i].reg == 0xffff)
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

	return 0;
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

int serdes_of_parse_init_seq(struct device *dev, const struct device_node *np,
                             const char *propname, struct serdes_init_seq *seq)
{
    struct reg_sequence *reg_sequence;
    u16 *buf, *d;
    unsigned int i, cnt;
    const void *data;
    int len;

    if(!seq)
        return -EINVAL;

    data = of_get_property(np, propname, &len);
    if(!data)
        return -EINVAL;

    buf = devm_kmemdup(dev, data, len, GFP_KERNEL);
    if(!buf)
        return -ENOMEM;

    d = buf;
    cnt = len / 4;
    seq->reg_seq_cnt = cnt;

    seq->reg_sequence = devm_kcalloc(dev, cnt, sizeof(struct reg_sequence), GFP_KERNEL);
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
EXPORT_SYMBOL_GPL(serdes_of_parse_init_seq);

static int serdes_get_init_seq(struct serdes *serdes)
{
    struct device *dev = serdes->dev;
    struct device_node *np = dev->of_node;
    int err, ret = 0;

    serdes->serdes_init_seq = devm_kzalloc(dev, sizeof(*serdes->serdes_init_seq),
                                           GFP_KERNEL);
    if(!serdes->serdes_init_seq)
        return -ENOMEM;

    err = serdes_of_parse_init_seq(dev, np, "serdes-init-sequence", serdes->serdes_init_seq);
    if(err) {
        dev_err(dev, "Failed to parse serdes-init-sequence\n");
        return err;
    }

    serdes->serdes_backup_seq = devm_kzalloc(dev, sizeof(*serdes->serdes_backup_seq),
                                             GFP_KERNEL);
    if(!serdes->serdes_backup_seq)
        return -ENOMEM;

    /* Try to parse backup sequence, fallback to init sequence if not found (matching original behavior) */
    err = serdes_of_parse_init_seq(dev, np, "serdes-init-sequence-backup", serdes->serdes_backup_seq);
    if(err) {
         err = serdes_of_parse_init_seq(dev, np, "serdes-init-sequence", serdes->serdes_backup_seq);
    }
    
    if(err) {
        dev_err(dev, "Failed to parse serdes-backup-seq\n");
        return err;
    }

    return 0;
}

int serdes_i2c_probe(struct i2c_client *client,
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
        return ret;
    }

    of_property_read_u32(dev->of_node, "id-serdes-bridge-split",
                 &serdes->id_serdes_bridge_split);
    if ((serdes->id_serdes_bridge_split < MAX_NUM_SERDES_SPLIT) && (serdes->type == TYPE_SER)) {
        g_serdes_ser_split[serdes->id_serdes_bridge_split] = serdes;
        SERDES_DBG_MFD("%s: %s-%s g_serdes_split[%d]=0x%p\n", __func__,
                   dev_name(serdes->dev), serdes->chip_data->name,
                   serdes->id_serdes_bridge_split, serdes);
    }

    of_property_read_u32(dev->of_node, "reg-hw", &serdes->reg_hw);
    of_property_read_u32(dev->of_node, "reg", &serdes->reg_use);
    of_property_read_u32(dev->of_node, "link", &serdes->link_use);
    of_property_read_u32(dev->of_node, "id-serdes-panel-split", &serdes->id_serdes_panel_split);
    if ((serdes->id_serdes_panel_split) && (serdes->type == TYPE_DES)) {
        serdes->g_serdes_bridge_split = g_serdes_ser_split[serdes->id_serdes_panel_split];
        SERDES_DBG_MFD("%s: id=%d p=0x%p\n", __func__,
                   serdes->id_serdes_panel_split, serdes->g_serdes_bridge_split);
    }

    if (serdes->reg_hw) {
        SERDES_DBG_MFD("%s: %s start change i2c address from 0x%x to 0x%x\n",
                   __func__, dev->of_node->name, serdes->reg_hw, serdes->reg_use);

        if (!serdes->route_enable) {
            ret = serdes_set_i2c_address(serdes, serdes->reg_hw,
                             serdes->reg_use, serdes->link_use);
            if (ret)
                dev_err(dev, "%s failed to set addr\n", serdes->chip_data->name);
        }
    }

    if (serdes->chip_data->check_ops && serdes->chip_data->check_ops->identify) {
        ret = serdes->chip_data->check_ops->identify(serdes);
        if (ret)
            return dev_err_probe(dev, ret, "Failed to identify device\n");
    }

    if(serdes->chip_data->serdes_type == TYPE_SER) {
        if(serdes->chip_data->chip_init)
            serdes->chip_data->chip_init(serdes);
        ret = serdes_i2c_set_sequence(serdes);
        if (ret)
             return dev_err_probe(dev, ret, "Failed to set sequence\n");
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
EXPORT_SYMBOL_GPL(serdes_i2c_probe);

const struct dev_pm_ops serdes_pm_ops = {
    .prepare = serdes_i2c_prepare,
    .complete = serdes_i2c_complete,
    .suspend = serdes_i2c_suspend,
    .resume = serdes_i2c_resume,
    .poweroff = serdes_i2c_poweroff,
};
EXPORT_SYMBOL_GPL(serdes_pm_ops);

MODULE_AUTHOR("weigenyin");
MODULE_DESCRIPTION("The AIM951X Serializer driver");
MODULE_LICENSE("GPL");

