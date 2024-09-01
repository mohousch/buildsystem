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

#ifndef CEC_INTERNAL_H_
#define CEC_INTERNAL_H_

#define CEC_IRQ_7111                143
#define CEC_IRQ_7105                157

#ifdef STx7105
#define CEC_IRQ                157
#endif
#ifdef STx7111
#define CEC_IRQ                143
#endif

#define CEC_STATUS_RECV_BTF 128
#define CEC_STATUS_RECV_ERR 64
#define CEC_STATUS_RECV_EOMSG 32
#define CEC_STATUS_RECV_SOMSG 16
#define CEC_STATUS_SEND_BTF 8
#define CEC_STATUS_SEND_ERR 4
#define CEC_STATUS_SEND_EOMSG 2
#define CEC_STATUS_SEND_SOMSG 1

#define CEC_ERROR_SEND_BTF          64
#define CEC_ERROR_ON_LINE          32
#define CEC_ERROR_ACK     16
#define CEC_ERROR_START         8
#define CEC_ERROR_RECV_BTF          4
#define CEC_ERROR_PERIOD    2
#define CEC_ERROR_TIMING    1

#define CEC_MAX_DATA_LEN 15
#define RECV_BUF_SIZE (CEC_MAX_DATA_LEN + 1)
#define SEND_BUF_SIZE (CEC_MAX_DATA_LEN + 1)

void cec_write_data(u32 value);
u32 cec_read_data(void);
void cec_start_sending(unsigned char isPing);
void cec_end_sending(void);
u8 cec_get_status(void);
u8 cec_get_error(void);
void cec_acknowledge(void);
void cec_acknowledge_eom(void);
void cec_set_own_address(u32 own_address);

void str_status(unsigned char status);
void str_error(unsigned char error);

int  cec_internal_init(void);
void cec_internal_exit(void);

#endif
