/*
 *
 * (c) 2010 konfetti, schischu
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#ifndef CEC_WORKER_C_
#define CEC_WORKER_C_

#include <linux/interrupt.h>

int cec_task(void *dummy);
irqreturn_t cec_interrupt(int irq, void *dev_id);
void sendMessage(unsigned int len, unsigned char buf[]);
//void sendMessageWithRetry(unsigned int len, unsigned char buf[], unsigned int retry);

void startTask(void);
void endTask(void);
void cec_worker_init(void);

#endif
