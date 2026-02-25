# SPDX-License-Identifier: GPL-2.0
#
# Makefile for multifunction miscellaneous devices that just used for display
#

obj-$(CONFIG_MFD_SERDES_DISPLAY)     += maxim/ ti/
serdes-mfd-display-$(CONFIG_MFD_SERDES_DISPLAY) += serdes_core.o serdes_irq.o
obj-$(CONFIG_MFD_SERDES_DISPLAY) += serdes-mfd-display.o serdes_i2c.o serdes_bridge.o serdes_bridge_split.o serdes_panel.o serdes_panel_split.o serdes_gpio.o serdes_pinctrl.o
