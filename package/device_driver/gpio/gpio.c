/*
 *  gpio.c
 *  
 *  (C) 2024.08.06 <hkdywg@163.com>
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

#define DEVICE_NAME "test_gpio"
//#define IRQ_USE_TASKLET
#define IRQ_USE_WORKQUE


struct gpio_demo_priv
{
	int irq;
	int gpio;
#ifdef IRQ_USE_WORKQUE
	struct work_struct wq;
	struct timer_list timer;
#endif
};
static void tasklet_process_func(unsigned long data);
DECLARE_TASKLET(gpio_tasklet, tasklet_process_func, 0);

/* gpio irq handler */
static irqreturn_t gpio_demo_handle(int irq, void *dev_id)
{
	struct gpio_demo_priv *pdata;
	printk("irq %d handle\n", irq);
#ifdef IRQ_USE_TASKLET
	tasklet_schedule(&gpio_tasklet);
#endif
#ifdef IRQ_USE_WORKQUE
	pdata = (struct gpio_demo_priv*)dev_id;	
	/* set timeout*/
	pdata->timer.expires = jiffies + msecs_to_jiffies(1000);
	add_timer(&pdata->timer);
#endif
	return IRQ_HANDLED;
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

static void timer_isr(struct timer_list *unused)
{
	struct gpio_demo_priv *pdata;

	pdata = container_of(unused, struct gpio_demo_priv, timer);
	/* wakeup work queue task */
	schedule_work(&pdata->wq);
}

static int gpio_demo_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	struct gpio_demo_priv *priv;
	int value, gpio, irq;
	int ret;

	pr_debug("start runing probe function~!\n");
	priv = kzalloc(sizeof(*priv), GFP_KERNEL);
	if(!priv) {
		printk("ERROR: No free memory\n");
		ret = -ENOMEM;
		goto err_alloc;
	}
	printk("of node name is %s\n", np->name);
	of_property_read_u32(np, "test-pro", &value);
	printk("of node name is %s, test-pro is %d\n", np->name, value);

	gpio = of_get_named_gpio(np, "gpios", 0);
	if(gpio < 0) {
		printk("Unable to get gpio from dts ,err code is %d\n", gpio);
		ret = -EINVAL;
		goto err_gpio;
	}
#if 0
	gpio_direction_output(gpio, 0);

	gpio_set_value(gpio, 1);

	value = gpio_get_value(gpio);
	printk("gpio-%d: %d\n", gpio, value);
#endif

	irq = gpio_to_irq(gpio);

#ifdef IRQ_USE_WORKQUE
	/* Init work queue */
	INIT_WORK(&priv->wq, irq_work_queue);

	/* Emulate interrupt via timer */
	timer_setup(&priv->timer, timer_isr, 0);
#endif

	priv->irq = irq;
	priv->gpio = gpio;

	/* register irq function and pass priv data as paramter to isr func */
	ret = request_irq(irq, gpio_demo_handle, IRQF_TRIGGER_HIGH, DEVICE_NAME, priv);
	if(ret < 0) {
		printk("Can't request irq %d\n", irq);
		ret = -EINVAL;
		goto err_irq;
	}
	printk("gpio-%d irq: %d\n", gpio, irq);

	platform_set_drvdata(pdev, priv);

	return 0;

err_irq:
err_gpio:
	kfree(priv);

err_alloc:
	return ret;

}

static int gpio_demo_remove(struct platform_device *pdev)
{
	struct gpio_demo_priv *priv = platform_get_drvdata(pdev);

	free_irq(priv->irq, NULL);
#ifdef IRQ_USE_TASKLET
	tasklet_kill(&gpio_tasklet);
#endif
#ifdef IRQ_USE_WORKQUE
	del_timer(&priv->timer);
#endif

	kfree(priv);

	return 0;
}

static const struct of_device_id gpio_demo_of_match[] = {
	{ .compatible = "LinuxLab, GPIO", },
	{  }
};

MODULE_DEVICE_TABLE(of, gpio_demo_of_match);


static struct platform_driver gpio_demo_driver = {
	.probe = gpio_demo_probe,
	.remove = gpio_demo_remove,
	.driver = {
		.owner = THIS_MODULE,
		.name = DEVICE_NAME,
		.of_match_table = gpio_demo_of_match,
	},
};

module_platform_driver(gpio_demo_driver);


MODULE_LICENSE("GPL");
MODULE_AUTHOR("yinwg <hkdywg@163.com>");
MODULE_DESCRIPTION("gpio platform driver");

