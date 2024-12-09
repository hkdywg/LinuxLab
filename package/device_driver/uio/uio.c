/*
 *  uio.c
 *  
 *  (C) 2024.12.09 <hkdywg@163.com>
 *
 *  This program is free software; you can redistribute it and/r modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 * */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/time.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/of_gpio.h>
#include <linux/of_platform.h>
#include <linux/delay.h>
#include <linux/timer.h>
#include <linux/workqueue.h>
#include <linux/uio_driver.h>

#define DEVICE_NAME "test_uio"
#define UIO_VERSION "1.0"
#define UIO_MEM_SIZE 1024

struct gpio_demo_priv
{
	int irq;
	int gpio;
#ifdef IRQ_USE_WORKQUE
	struct work_struct wq;
	struct timer_list timer;
#endif
};

static int uio_irq_stat = 1;

static void tasklet_process_func(unsigned long data);
DECLARE_TASKLET(gpio_tasklet, tasklet_process_func, 0);

/* gpio irq handler */
static irqreturn_t uio_irq_handle(int irq, struct uio_info *info)
{
	struct gpio_demo_priv *pdata;
	printk("irq %d handle\n", irq);
#ifdef IRQ_USE_TASKLET
	tasklet_schedule(&gpio_tasklet);
#endif
#ifdef IRQ_USE_WORKQUE
	pdata = (struct gpio_demo_priv*)info->priv;	
	/* set timeout*/
	pdata->timer.expires = jiffies + msecs_to_jiffies(1000);
	add_timer(&pdata->timer);
#endif
	return IRQ_HANDLED;
}

static int uio_irq_control(struct uio_info *info, s32 irq_on)
{
	/* enable irq again will cause a crash */
	if(irq_on && !uio_irq_stat) {
		enable_irq(info->irq);
		uio_irq_stat = 1;
	}
	else if(!irq_on && uio_irq_stat) {
		disable_irq_nosync(info->irq);
		uio_irq_stat = 0;
	}

	return 0;
}

/* tasklet handler */
static void tasklet_process_func(unsigned long data)
{
	printk("tasklet executed.\n");
}

static void irq_work_queue(struct work_struct *work)
{
	printk("work queue executed\n");
}

#ifdef IRQ_USE_WORKQUE
static void timer_isr(struct timer_list *unused)
{
	struct gpio_demo_priv *pdata;

	pdata = container_of(unused, struct gpio_demo_priv, timer);
	/* wakeup work queue task */
	schedule_work(&pdata->wq);
}
#endif

static int uio_demo_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	struct uio_info *uinfo;
	struct gpio_demo_priv *priv;
	int gpio;
	int ret;

	uinfo = kzalloc(sizeof(*uinfo), GFP_KERNEL);
	if(!uinfo) {
		printk("ERROR: No free memory\n");
		ret = -ENOMEM;
		goto err_alloc;
	}
	uinfo->mem[0].addr = (unsigned long)kzalloc(UIO_MEM_SIZE, GFP_KERNEL);
	if(!uinfo->mem[0].addr) {
		printk("ERROR: No free memory\n");
		ret = -ENOMEM;
		goto err_alloc;
	}
	priv = kzalloc(sizeof(*priv), GFP_KERNEL);
	if(!priv) {
		printk("ERROR: No free memory\n");
		ret = -ENOMEM;
		goto err_alloc;
	}

	gpio = of_get_named_gpio(np, "gpios", 0);
	if(gpio < 0) {
		printk("Unable to get gpio from dts ,err code is %d\n", gpio);
		ret = -EINVAL;
		goto err_free;
	}
	gpio_direction_input(gpio);

	uinfo->irq = gpio_to_irq(gpio);
	if(uinfo->irq < 0) {
		printk("Unable to get IRQ!\n");
		ret = -EINVAL;
		goto err_free;
	}

#ifdef IRQ_USE_WORKQUE
	/* Init work queue */
	INIT_WORK(&priv->wq, irq_work_queue);

	/* Emulate interrupt via timer */
	timer_setup(&priv->timer, timer_isr, 0);
#endif

	priv->irq = uinfo->irq;
	priv->gpio = gpio;

	/* setup UIO */
	uinfo->name = DEVICE_NAME;
	uinfo->version = UIO_VERSION;
	uinfo->irq_flags = IRQF_TRIGGER_HIGH | IRQF_SHARED;
	uinfo->handler = uio_irq_handle;
	uinfo->irqcontrol = uio_irq_control;

	uinfo->mem[0].memtype = UIO_MEM_LOGICAL;
	uinfo->mem[0].size = UIO_MEM_SIZE;
	uinfo->priv = (void *)priv;


	ret = uio_register_device(&pdev->dev, uinfo);
	if(ret < 0) {
		printk("Error: register uio\n");
		ret = -EINVAL;
		goto err_free;
	}
	platform_set_drvdata(pdev, uinfo);
	return 0;

err_free:
	if(priv)
		kfree(priv);
	if(uinfo)
		kfree(uinfo);

err_alloc:
	return ret;

}

static int uio_demo_remove(struct platform_device *pdev)
{
	struct uio_info *priv = platform_get_drvdata(pdev);

	uio_unregister_device(priv);
	kfree((void *)(unsigned long)priv->mem[0].addr);

#ifdef IRQ_USE_TASKLET
	tasklet_kill(&gpio_tasklet);
#endif
#ifdef IRQ_USE_WORKQUE
	del_timer(&priv->timer);
#endif

	kfree(priv);

	return 0;
}

static const struct of_device_id uio_demo_of_match[] = {
	{ .compatible = "LinuxLab, UIO", },
	{  }
};

MODULE_DEVICE_TABLE(of, uio_demo_of_match);


static struct platform_driver uio_demo_driver = {
	.probe = uio_demo_probe,
	.remove = uio_demo_remove,
	.driver = {
		.owner = THIS_MODULE,
		.name = DEVICE_NAME,
		.of_match_table = uio_demo_of_match,
	},
};

module_platform_driver(uio_demo_driver);


MODULE_LICENSE("GPL");
MODULE_AUTHOR("yinwg <hkdywg@163.com>");
MODULE_DESCRIPTION("gpio platform driver");

