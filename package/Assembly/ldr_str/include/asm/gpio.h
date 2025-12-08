#ifndef _GPIO_H_
#define _GPIO_H_

#include "asm/base.h"

#define GPFSEL1             (BASE+0x00200004)
#define GPSET0              (BASE+0x0020001C)
#define GPCLR0              (BASE+0x00200028)
#define GPPUD               (BASE+0x00200094)
#define GPPUDCLK0           (BASE+0x00200098)
#define GPIO_PUP_PDN_CNTRL_REG0 (BASE+0x002000E4)

#endif
