/*
 * Phoenix-RTOS
 * 
 * Phoenix server
 *
 * BSP2 message dispatcher
 *
 * Copyright 2004 Pawel Pisarczyk
 *
 * This file is part of Phoenix-RTOS.
 *
 * Phoenix-RTOS is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Phoenix-RTOS kernel is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Phoenix-RTOS kernel; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef _DISPATCH_H_
#define _DISPATCH_H_
#include "msg.h"

typedef enum {
	SERIAL,
	PIPE,
	UDP,
	USB_VYBRID,
} dmode_t;

/* Function reads and dispatches messages */
extern int dispatch(char *dev_addr, dmode_t mode, unsigned int speed_port, char *sysdir);
extern int (*msg_send)(int fd, msg_t *msg, u16 seq);
extern int (*msg_recv)(int fd, msg_t *msg, int *state);

extern int usb_vybrid_dispatch(char* kernel, char* loadAddr, char* jump_addr);


#endif