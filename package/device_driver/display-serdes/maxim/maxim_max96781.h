/*
* maxim_max96781.h
*   register define for max96781	
*
* @copyright Copyright (c) 2022 Jiangsu New Vision Automotive Electronics Co.，Ltd. All rights reserved.
*
* Author: weigenyin <weigenyin@zjautomotive.com>
*
*/

#ifndef __SERDES_MAX96781_H_
#define __SERDES_MAX96781_H_

#include <linux/bitfield.h>

#define GPIO_A_REG(gpio)    (0x0200 + ((gpio) * 8))
#define GPIO_B_REG(gpio)    (0x0201 + ((gpio) * 8))
#define GPIO_C_REG(gpio)    (0x0202 + ((gpio) * 8))
#define GPIO_D_REG(gpio)    (0x0203 + ((gpio) * 8))
#define GPIO_E_REG(gpio)    (0x0204 + ((gpio) * 8))
#define GPIO_F_REG(gpio)    (0x0205 + ((gpio) * 8))
#define GPIO_G_REG(gpio)    (0x0206 + ((gpio) * 8))

/* 0005h */
#define PU_LF3              BIT(3)
#define PU_LF2              BIT(2)
#define PU_LF1              BIT(1)
#define PU_LF0              BIT(0)

/* 0010h */
#define RESET_ALL           BIT(7)
#define SLEEP               BIT(3)

/* 0011h */
#define CXTP_B              BIT(2)
#define CXTP_A              BIT(0)

/* 0013h */
#define LOCKED              BIT(3)
#define ERROR               BIT(2)

/* 0026h */
#define LF_0                GENMASK(2, 0)
#define LF_1                GENMASK(6, 4)

/* 0027h */
#define LF_2                GENMASK(2, 0)
#define LF_3                GENMASK(6, 4)

/* 0028h, 0032h */
#define LINK_EN             BIT(7)
#define TX_RATE             GENMASK(3, 2)

/* 0029h */
#define RESET_LINK          BIT(0)
#define RESET_ONESHOT       BIT(1)

/* 002Ah */
#define LINK_LOCKED         BIT(0)

/* 0076h */
#define DIS_REM_CC          BIT(7)

/* 0100h, 0110h */
#define VID_TX_EN           BIT(0)

/* 0102h, 0112h */
#define PCLKDET_A           BIT(7)
#define DRIF_ERR_A          BIT(6)
#define OVERFLOW_A          BIT(5)
#define FIFO_WARN_A         BIT(4)
#define LIM_HEART           BIT(2)

/* 0200h */
#define RES_CFG             BIT(7)
#define TX_COM_EN           BIT(5)
#define GPIO_OUT            BIT(4)
#define GPIO_IN             BIT(3)
#define GPIO_OUT_DIS        BIT(0)

/* 0201h */
#define PULL_UPDN_SEL       GENMASK(7, 6)
#define OUT_TYPE            BIT(5)
#define GPIO_TX_ID          GENMASK(4, 0)

/* 0202h */
#define OVR_RES_CFG         BIT(7)
#define IO_EDGE_RATE        GENMASK(6, 5)
#define GPIO_RX_ID          GENMASK(4, 0)

/* 0203h */
#define GPIO_IO_RX_EN       BIT(5)
#define GPIO_OUT_LGC        BIT(4)
#define GPIO_RX_EN_A        BIT(1)
#define GPIO_TX_EN_A        BIT(0)

/* 641Ah */
#define DPRX_TRAIN_STATE    GENMASK(7, 4)

/* 7000h */
#define LINK_ENABLE         BIT(0)

/* 7070h */
#define MAX_LANE_COUNT      GENMASK(7, 0)

/* 7074h */
#define MAX_LINK_RATE       GENMASK(7, 0)

#endif
