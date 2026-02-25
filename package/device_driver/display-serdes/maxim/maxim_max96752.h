/*
* maxim_max96752.h
*   register define for max96752
*
* @copyright Copyright (c) 2022 Jiangsu New Vision Automotive Electronics Co.，Ltd. All rights reserved.
*
* Author: weigenyin <weigenyin@zjautomotive.com>
*
*/

#ifndef __MFD_SERDES_MAXIM_MAX96752_H__
#define __MFD_SERDES_MAXIM_MAX96752_H__

#define GPIO_A_REG(gpio)	(0x0200 + ((gpio) * 3))
#define GPIO_B_REG(gpio)	(0x0201 + ((gpio) * 3))
#define GPIO_C_REG(gpio)	(0x0202 + ((gpio) * 3))


/* 0200h */
#define RES_CFG			BIT(7)
#define RSVD			BIT(6)
#define TX_COMP_EN		BIT(5)
#define GPIO_OUT		BIT(4)
#define GPIO_IN			BIT(3)
#define GPIO_RX_EN		BIT(2)
#define GPIO_TX_EN		BIT(1)
#define GPIO_OUT_DIS	BIT(0)

/* 0201h */
#define PULL_UPDN_SEL	GENMASK(7, 6)
#define OUT_TYPE		BIT(5)
#define GPIO_TX_ID		GENMASK(4, 0)

/* 0202h */
#define OVR_RES_CFG		BIT(7)
#define GPIO_RX_ID		GENMASK(4, 0)

enum link_mode {
	DUAL_LINK,
	LINKA,
	LINKB,
	SPLITTER_MODE,
};

enum max96752_gpio_list {
    MAXIM_MAX96752_GPIO0 = 0,
    MAXIM_MAX96752_GPIO1,
    MAXIM_MAX96752_GPIO2,
    MAXIM_MAX96752_GPIO3,
    MAXIM_MAX96752_GPIO4,
    MAXIM_MAX96752_GPIO5,
    MAXIM_MAX96752_GPIO6,
    MAXIM_MAX96752_GPIO7,
    MAXIM_MAX96752_GPIO8,
    MAXIM_MAX96752_GPIO9,
    MAXIM_MAX96752_GPIO10,
    MAXIM_MAX96752_GPIO11,
    MAXIM_MAX96752_GPIO12,
    MAXIM_MAX96752_GPIO13,
    MAXIM_MAX96752_GPIO14,
    MAXIM_MAX96752_GPIO15,
};

#endif
