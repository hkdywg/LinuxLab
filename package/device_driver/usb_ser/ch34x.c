/*
 *  ch34x.c
 *  
 *  (C) 2025.03.25 <hkdywg@163.com>
 *
 *  This program is free software; you can redistribute it and/r modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 * */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/usb.h>
#include <linux/usb/serial.h>

#define CH34X_VENDOR_ID    0x1A86
#define CH340_PRODUCT_ID   0x7523
#define CH341_PRODUCT_ID   0x5523

/* flags for IO-Bits */
#define CH34X_BIT_DTR   (1 << 5)
#define CH34X_BIT_RTS   (1 << 6)

/* status returned in third interrupt answer byte, inverted in data from irq */
#define CH34X_BIT_CTS   0x01
#define CH34X_BIT_DSR   0x02
#define CH34X_BIT_RI    0x04
#define CH34X_BIT_DCD   0x08
#define CH34X_BITS_MODEM_STAT   0x0F /* all bits */

#define DEFAULT_BAUD_RATE   9600

struct ch34x_private {
    spinlock_t lock;    /* access lock */
    unsigned baud_rate;
    u8 mcr;
    u8 msr;
    u8 lcr;
};

static int ch34x_control_out(struct usb_device *dev, u8 request,
                             u16 value, u16 index)
{
    int ret;
}

static int ch34x_set_handshake(struct usb_device *dev, u8 control)
{
    return ch34x_control_out(dev, CH34X_REQ_MODEM_CTRL, ~control, 0);
}

static int ch34x_get_status(struct usb_device *dev, struct ch34x_private *priv)
{
    char buffer[2];
    int ret;

    ret = ch34x_control_in(dev, CH34X_REQ_READ_REG, 0x0706, 0, buffer, 2);
    if(ret < 0)
        return ret;
    spin_lock_irqsave(&priv->lock, flags);
    priv->msr = (~buffer[0]) & CH34X_MODEM_STAT;
    spin_unlock_irqrestore(&priv->lock, flags);

    return ret;
}

static int ch34x_configure(struct usb_device *dev, struct ch34x_private *priv)
{
    char buffer[2];
    int ret;

    ret = ch34x_control_in(dev, CH34X_REQ_READ_VERSION, 0, 0, buffer, 2);
    if(ret < 0)
        return ret;

    ret = ch34x_control_out(dev, CH34X_REQ_SERIAL_INIT, 0, 0);
    if(ret < 0)
        return ret;

    ret = ch34x_set_handshake(dev, priv->mcr);
    return ret;
}

static int ch34x_port_probe(struct usb_serial_port *port)
{
    struct ch34x_private *priv;
    int ret;

    priv = kzalloc(sizeof(struct ch34x_private), GFP_KERNEL);
    if(!priv)
        return -ENOMEM;
    spin_lock_init(priv->lock);
    priv->baud_rate = DEFAULT_BAUD_RATE;
    priv->lcr = CH34X_LCR_ENABLE_RX | CH34X_LCR_ENABLE_TX | CH34X_LCR_CS8;

    ret = ch34x_configure(port->serial->dev, priv);
    if(ret < 0)
        goto error;

    usb_set_serial_port_data(port, priv);
    return 0;

error:
    kfree(priv);
    return ret;
}

static int ch34x_port_remove(struct usb_serial_port *port)
{
    struct ch34x_private *priv = usb_get_serial_port_data(port);    
    kfree(priv);

    return 0;
}

static int ch34x_carrier_raised(struct usb_serial_port *port)
{
    struct ch34x_private *priv = usb_get_serial_port_data(port);    
    if(priv->msr & CH34X_BIT_DCD)
        return 1;

    return  0;
}

static void ch34x_dtr_rts(struct usb_serial_port *port, int on)
{
    struct ch34x_private *priv = usb_get_serial_port_data(port);    
    unsigned long flags;

    /* drop DTR and RTS */
    spin_lock_irqsave(&priv->lock, flags);
    if(on)
        priv->mcr |= CH34X_BIT_RTS | CH34X_BIT_DTR;
    else
        priv->mcr &= ~(CH34X_BIT_RTS | CH34X_BIT_DTR);
    spin_unlock_irqrestore(&priv->lock, flags);
    ch34x_set_handshake(port->serial->dev, priv->mcr);
}

static void ch34x_close(struct usb_serial_port *port)
{
    usb_serial_generic_close(port);
    usb_kill_urb(port->interrupt_in_urb);
}


static int ch34x_open(struct tty_struct *tty, struct usb_serial_port *port)
{
    struct ch34x_private *priv = usb_get_serial_port_data(port);
    int ret;

    if(tty) 
        ch34x_set_termios(tty, port, NULL);

    /* submit interrupt urb */
    ret = ubs_submit_urb(port->interrupt_in_urb, GFP_KERNEL);
    if(ret) {
        dev_err(&port->dev, "%s - failed submit interrupt urb, error %d\n",
                __func__, ret);
        return ret;
    }

    ret = ch34x_get_status(port->serial->dev, priv);
    if(ret < 0) {
        dev_error(&port->dev, "failed to read modem status: %d\n", ret);
        goto err_kill_interrupt_urb;
    }
    
    ret = usb_serial_generic_open(tty, port);
    if(ret)
        goto err_kill_interrupt_urb;

    return 0;

err_kill_interrupt_urb:
    usb_kill_urb(port->interrupt_in_urb);

    return ret;
}

static struct usb_device_id ch34x_id_table[] = {
    { USB_DEVICE(CH34X_VENDOR_ID, CH340_PRODUCT_ID) },
    { USB_DEVICE(CH34X_VENDOR_ID, CH341_PRODUCT_ID) },
};

MODULE_DEVICE_TABLE(usb, ch34x_id_table);

static struct usb_serial_driver ch34x_device = {
    .driver = {
        .owner = THIS_MODULE,
        .name  = "ch34x", 
    },
    .id_table = ch34x_id_table,
    .num_ports      = 1,
    .open           = ch34x_open,
    .close          = ch34x_close,
    .set_termios    = ch34x_set_termios,
    .dtr_rts        = ch34x_dtr_rts,
    .break_ctl      = ch34x_break_ctl,
    .tiocmget       = ch34x_tiocmget,
    .tiocmset       = ch34x_tiocmset,
    .read_int_callback = ch34x_read_int_callback,
    .port_probe     = ch34x_port_probe,
    .port_remove    = ch34x_port_remove,
    .reset_resume   = ch34x_reset_resume,
};

static struct usb_serial_driver *const ch34x_driver[] = {
    &ch34x_device, NULL
};

module_usb_serial_driver(ch34x_driver, ch34x_id_table);

MODULE_DESCRIPTION("CH34x USB to serial adaptor driver");
MODULE_AUTHOR("yinwg hkdywg@163.com");
MODULE_LICENSE("GPL");
