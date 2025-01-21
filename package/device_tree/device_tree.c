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
#include <linux/of_address.h>

#define DEVICE_NAME  		"dts_demo"
#define RESOURCE_REG_NUM 	3

static const struct of_device_id dts_demo_of_match[] = {
	{ .compatible = "LinuxLab, dts_demo", },
	{ .compatible = "LinuxLab, sub_node", },
	{  }
};

MODULE_DEVICE_TABLE(of, dts_demo_of_match);

static int dts_demo_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	struct device *dev = &pdev->dev;
	struct device_node *child_node, *node;
	void __iomem *base[RESOURCE_REG_NUM];
	struct resource iomem[RESOURCE_REG_NUM];
	struct of_phandle_args args;
	struct property *node_prop;
	const phandle *handle;
	const __be32 *prop;
	const u32 *ret_p;
	u64 reg_size;
	const char *comp = NULL;
	int count, i, ret;
	bool bool_data;
	u8 u8_data, u8_array[4];
	u16 u16_data;
	u32 u32_data;


	/* count child number for current device node */
	count = of_get_child_count(np);
	dev_info(dev, "%s has %d child node\n", np->name, count);

	dev_info(dev, "%s child: \n", np->name);
	for_each_child_of_node(np, child_node)
		dev_info(dev, "	%s \n", child_node->name);

	dev_info(dev, "%s available child:\n", np->name);
	for_each_available_child_of_node(np, child_node)
		dev_info(dev, "	%s\n", child_node->name);

	/* find device node via compatible property */
	for_each_compatible_node(node, NULL, "LinuxLab, dts_demo")
		dev_info(dev, "Found %s by compatible property\n", node->full_name);

	/* find all device node via device node id */
	for_each_matching_node(node, dts_demo_of_match)
		dev_info(dev, "matching %s by match table\n", node->name);

	/* find device node via property name */
	for_each_node_with_property(node,"node-name")
		dev_info(dev, "found %s by property\n", node->name);

	node_prop = of_find_property(np, "reg", NULL);
	if(node_prop) {
		/* get iomap resource */
		for(i = 0; i < RESOURCE_REG_NUM; i++) {
			of_address_to_resource(np, i, &iomem[i]);
			base[i] = devm_ioremap_resource(dev, &iomem[i]);
			if(IS_ERR(base[i])) {
				dev_err(dev, "%d could not IO remap\n", i);
			}else 
				dev_info(dev, "LinuxLab: %#lx - %#lx\n", (unsigned long)iomem[i].start,
											  (unsigned long)iomem[i].end);
		}
	}
	
	/* read first phandle argument */
	count = of_count_phandle_with_args(np, "interrupts", "#interrupt-cells");
	if(count < 0) {
		dev_err(dev, "unable to parse phandle\n");
	} else {
		printk("number phandle: %#x\n", count);
		for(i = 0; i < count; i++) {
			ret = of_parse_phandle_with_args(np, "interrupts", "#interrupt-cells", i, &args);
			if(ret < 0)
				dev_err(dev, "failed to parse interrupt %d: %d\n", i, ret);
			else
				dev_info(dev, "interrupt %d: controller=%pOFn, specifier=[%u, %u]\n",
						 i, args.np, args.args[0],args.args[1]);
		}
	}

	/* find device by path */
	node = of_find_node_by_path("/vir_device");
	if(node) {
		of_property_read_string(node, "status", &comp);
		if(comp)
			dev_info(dev, "%s status: %s\n", node->name, comp);
	}
	/* find a phandle on current device_node */
	handle = of_get_property(np, "clocks",  NULL);
	if(handle) {
		/* find device node by phandle*/	
		node = of_find_node_by_phandle(be32_to_cpup(handle));
		if(node) {
			/* get #address-cells and #size-cells number */
			count = of_n_addr_cells(node);
			dev_info(dev, "%s #address-cells: %d\n", node->name, count);
			/* read  property from special device node */		
			prop = of_get_property(node, "clock-frequency", NULL);
			if(prop)
				dev_info(dev, "%s clock-frequency: %d\n", node->name, be32_to_cpup(prop));
			/* get first address from 'reg' property */
			ret_p = of_get_address(node, 0, &reg_size, NULL);
			if(ret_p) {
				dev_info(dev, "%s address[0]: %#llx size: %#llx\n", node->name, of_translate_address(node, ret_p), reg_size);
			}
			/* get first address from 'reg' property */
			ret_p = of_get_address(node, 1, &reg_size, NULL);
			if(ret_p) {
				dev_info(dev, "%s address[1]: %#llx size: %#llx\n", node->name, of_translate_address(node, ret_p), reg_size);
			}
			/* read config-data value */
			bool_data = of_property_read_bool(node, "config-data");
			of_property_read_u8(node, "config-data", &u8_data);
			of_property_read_u16(node, "config-data", &u16_data);
			of_property_read_u32(node, "config-data", &u32_data);
			dev_info(dev, "bool_data: %#x, u8_data: %#x, u16_data: %#x, u32_data: %#x\n", bool_data, u8_data, u16_data, u32_data);
			of_property_read_u8_array(node, "config-data", u8_array, 4);
			dev_info(dev,"config-data array: %#x %#x %#x %#x\n", u8_array[0], u8_array[1], u8_array[2], u8_array[3]);
			/* read string property on current device-node */
			ret = of_property_read_string(node, "compatible", &comp);
			dev_info(dev, "read compatible string: %s\n", comp);
		} 
	}


	return 0;
}

static int dts_demo_remove(struct platform_device *pdev)
{
	return 0;
}



static struct platform_driver dts_demo_driver = {
	.probe = dts_demo_probe,
	.remove = dts_demo_remove,
	.driver = {
		.owner = THIS_MODULE,
		.name = DEVICE_NAME,
		.of_match_table = dts_demo_of_match,
	},
};

module_platform_driver(dts_demo_driver);


MODULE_LICENSE("GPL");
MODULE_AUTHOR("yinwg <hkdywg@163.com>");
MODULE_DESCRIPTION("device tree demo platform driver");

