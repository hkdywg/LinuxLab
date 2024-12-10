/*
 *  input_key.c
 *  
 *  (C) 2024.12.09 <hkdywg@163.com>
 *
 *  This program is free software; you can redistribute it and/r modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 * */




/*
* Key
*
*                       | A
*                 Press | | Up
*                       V |
*                    +-------+
*                    |  Key  |
*                +---------------+
*/

#include <linux/init.h>
#include <linux/module.h>
#include <linux/time.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/of_gpio.h>
#include <linux/of_platform.h>
#include <linux/delay.h>
#include <linux/timer.h>
#include <linux/input.h>


#define DEVICE_NAME "key_input"
#define IRQ_USE_WORKQUE

struct input_key_priv
{
	int irq;
	int gpio;
	bool irq_state;
	struct spinlock lock;
	struct input_dev *input;
#ifdef IRQ_USE_WORKQUE
	struct work_struct wq;
	struct timer_list timer;
#endif
};



/* input key irq handler */
static irqreturn_t input_key_isr(int irq, void *dev_id)
{
	struct input_key_priv *pdata = (struct input_key_priv*)dev_id;
	printk("irq %d handle\n", irq);

#ifdef IRQ_USE_WORKQUE
	/* set timeout*/
	pdata->timer.expires = jiffies + msecs_to_jiffies(1000);
	add_timer(&pdata->timer);
#endif

	disable_irq_nosync(pdata->irq);
	spin_lock(&pdata->lock);
	pdata->irq_state = false;
	spin_unlock(&pdata->lock);

	return IRQ_HANDLED;
}

static void irq_work_queue(struct work_struct *work)
{
	static bool value = true;
	struct input_key_priv *pdata;

	pdata = container_of(work, struct input_key_priv, wq);
	if(value) {
		printk("Event: KEY_DOWN --> UP\n");
		input_report_key(pdata->input, KEY_DOWN, 0);
	} else {
		printk("Event: KEY_DOWN --> Press\n");
		input_report_key(pdata->input, KEY_DOWN, 1);
	}

	/* sync report */
	input_sync(pdata->input);

	value = !value;
}

static void timer_isr(struct timer_list *unused)
{
	struct input_key_priv *pdata;

	pdata = container_of(unused, struct input_key_priv, timer);
	/* wakeup work queue task */
	schedule_work(&pdata->wq);
	printk("run in timer isr %d", pdata->irq_state);
	if(!pdata->irq_state) {
		spin_lock(&pdata->lock);
		pdata->irq_state = true;
		spin_unlock(&pdata->lock);
		enable_irq(pdata->irq);
		printk("enable irq -----\n");
	}
}

static int input_key_open(struct input_dev *input)
{
	struct input_key_priv *pdata = input_get_drvdata(input);

	printk("Event: open input \n");
	return 0;
}

static void input_key_close(struct input_dev *input)
{
}

static int input_key_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	struct input_key_priv *priv;
	struct input_dev *input;
	int ret;

	priv = (struct input_key_priv*)kzalloc(sizeof(*priv), GFP_KERNEL);
	if(!priv) {
		printk("Error: no free memory\n");
		ret = -ENOMEM;
		goto err_alloc;
	}

	/* get irq number from dtb */
	priv->gpio = of_get_named_gpio(np, "key", 0);
	if(priv->gpio < 0) {
		printk("Error: unable to get key gpio from dtb\n");
		ret = -EINVAL;
		goto err_free;
	}
	gpio_direction_input(priv->gpio);
	priv->irq = gpio_to_irq(priv->gpio);
	if(priv->irq < 0) {
		printk("Error: unable to get irq\n");
		ret = -EINVAL;
		goto err_free;
	}

	/* create input device */
	input = devm_input_allocate_device(&pdev->dev);
	if(!input) {
		printk("Error: unable to alloc input device\n");
		ret = -ENOMEM;
		goto err_free;
	}

	/* setup key event */
	input->evbit[BIT_WORD(EV_KEY)] |= BIT_MASK(EV_KEY);
	/* setup key type */
	input->keybit[BIT_WORD(KEY_UP)] |= BIT_MASK(KEY_UP);
	input->keybit[BIT_WORD(KEY_DOWN)] |= BIT_MASK(KEY_DOWN);
	input->keybit[BIT_WORD(KEY_LEFT)] |= BIT_MASK(KEY_LEFT);
	input->keybit[BIT_WORD(KEY_RIGHT)] |= BIT_MASK(KEY_RIGHT);

	/* input information */
	input->name = DEVICE_NAME;
	input->phys = DEVICE_NAME "/input0";
	input->dev.parent = &pdev->dev;
	input->id.bustype = BUS_HOST;
	input->id.vendor = 0x9192;
	input->id.product = 0x1016;
	input->id.version = 0x1314;

	/* create /dev/input/eventx interface */
	input->open = input_key_open;
	input->close = input_key_close;
	/* Note! must setup befor input_register_device() */
	input_set_drvdata(input, priv);

	/* register input deice */
	ret = input_register_device(input);
	if(ret) {
		dev_err(&pdev->dev, "Unable to register input device\n");
		goto err_input_register;
	}

#ifdef IRQ_USE_WORKQUE
	/* Init work queue */
	INIT_WORK(&priv->wq, irq_work_queue);

	/* Emulate interrupt via timer */
	timer_setup(&priv->timer, timer_isr, 0);
#endif
	priv->irq_state = true;
	spin_lock_init(&priv->lock);
	ret = request_irq(priv->irq, input_key_isr, IRQF_TRIGGER_HIGH, DEVICE_NAME, (void *)priv);
	if(ret < 0) {
		printk("Error: request irq %d failed\n", priv->irq);
		goto err_irq;
	}
	priv->input = input;
	platform_set_drvdata(pdev, priv);
	printk("-------\n");
	return 0;
err_irq:
	input_unregister_device(input);
err_input_register:
	input_free_device(input);
err_free:
	kfree(priv);

err_alloc:
	return ret;
}

static int input_key_remove(struct platform_device *pdev)
{
	struct input_key_priv *priv = platform_get_drvdata(pdev);

	free_irq(priv->irq, (void *)priv);
	input_unregister_device(priv->input);
	input_free_device(priv->input);
	kfree(priv);
	platform_set_drvdata(pdev, NULL);

	return 0;
}

static const struct of_device_id input_key_of_match[] = {
	{ .compatible = "LinuxLab, input_key", },
	{  }
};

MODULE_DEVICE_TABLE(of, input_key_of_match);


static struct platform_driver input_key_driver = {
	.probe = input_key_probe,
	.remove = input_key_remove,
	.driver = {
		.owner = THIS_MODULE,
		.name = DEVICE_NAME,
		.of_match_table = input_key_of_match,
	},
};

module_platform_driver(input_key_driver);


MODULE_LICENSE("GPL");
MODULE_AUTHOR("yinwg <hkdywg@163.com>");
MODULE_DESCRIPTION("gpio platform driver");

