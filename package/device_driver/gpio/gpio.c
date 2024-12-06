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

#define DEVICE_NAME "test_gpio"

struct gpio_demo_priv
{
	int irq;
	int gpio;
};

static irqreturn_t gpio_demo_handle(int irq, void *dev_id)
{
	printk("irq %d handle\n", irq);
	return IRQ_HANDLED;
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

	ret = request_irq(irq, gpio_demo_handle, IRQF_TRIGGER_FALLING, DEVICE_NAME, NULL);
	if(ret < 0) {
		printk("Can't request irq %d\n", irq);
		ret = -EINVAL;
		goto err_irq;
	}
	printk("gpio-%d irq: %d\n", gpio, irq);

	priv->irq = irq;
	priv->gpio = gpio;
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

