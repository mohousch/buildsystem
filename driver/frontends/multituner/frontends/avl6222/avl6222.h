/*
 * @brief avl6222.c
 *
 * based on code from avl2108 from:
 *
 * @author Pedro Aguilar <pedro@duolabs.com>
 *
 * @brief Availink avl2108 - DVBS/S2 Satellite demod driver with Sharp BS2S7HZ6360 tuner
 *
 * Copyright (C) 2009-2010 Duolabs Spa
 *               2011 adapted by konfetti for use with ufs922, hs9510 and hs8200
 *
 * based on avl6222 code from:
 * @author Ramon-Tomislav Rebersak <ramon.rebersak@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef _AVL6222_H
#define _AVL6222_H

#include <linux/dvb/frontend.h>
#include <linux/version.h>
#include <linux/stm/pio.h>
#include <linux/mutex.h>
#include "dvb_frontend.h"
#include "avl6222_platform.h"
#include "equipment.h"

/*---------------------------------------------------------------------
 * Debug stuff
 *---------------------------------------------------------------------*/

#define format_addr(X, Y) \
	do { \
		Y[0] =(u8)((X) >> 16); \
		Y[1] =(u8)((X) >> 8); \
		Y[2] =(u8)(X); \
	} while (0)

/*< Create a formatted 16 bit array */
#define format_16(X, Y) \
	do { \
		Y[0] =(u8)((X) >> 8); \
		Y[1] =(u8)((X) & 0xFF); \
	} while (0)

/*< Create a formatted 32 bit array */
#define format_32(X, Y) \
	do { \
		Y[0] =(u8)((X) >> 24); \
		Y[1] =(u8)((X) >> 16); \
		Y[2] =(u8)((X) >> 8); \
		Y[3] =(u8)((X) & 0xFF); \
	} while (0)

/*---------------------------------------------------------------------
 * Definitions
 *---------------------------------------------------------------------*/

#define AVL6222_ADDITIONAL_DELAY 1
#define AVL6222_ENABLE_MPEG_BUS  1

#define AVL6222_DEMOD_FW "dvb-fe-avl6222.fw" /*< Demod fw used for locking */

#define I2C_MAX_READ  64
#define I2C_MAX_WRITE 64

#define cError1 (1 << 0)
#define cError2 (1 << 1)
#define cError3 (1 << 2)
#define cError4 (1 << 3)
#define cError5 (1 << 4)
#define cError6 (1 << 5)
#define cError7 (1 << 6)
#define cError8 (1 << 7)

/* Error codes */
#define AVL6222_OK            0  /*< No error */
#define AVL6222_ERROR_GENERIC 1  /*< Generic error */
#define AVL6222_ERROR_I2C     2  /*< i2c bus failed */
#define AVL6222_ERROR_TIMEOUT 4  /*< Operation failed in a given time period */
#define AVL6222_ERROR_PREV    8  /*< Still working on a previous command */
#define AVL6222_ERROR_MEM    32  /*< Not enough memory for finishing the current job */


/*****************************
 * Data type handling
 *****************************/

static inline u16 extract_16(const u8 *buf)
{
	u16 data;
	data = buf[0];
	data = (u16)(data << 8) + buf[1];
	return data;
}

static inline u32 extract_32(const u8 *buf)
{
	unsigned int data;
	data = buf[0];
	data = (data << 8) + buf[1];
	data = (data << 8) + buf[2];
	data = (data << 8) + buf[3];
	return data;
}

/*---------------------------------------------------------------------
 * globals
 *---------------------------------------------------------------------*/

struct avl6222_pllconf
{
	u16 m_r1;
	u16 m_r2;
	u16 m_r3;
	u16 m_r4;
	u16 m_r5;
	u16 ref_freq;
	u16 demod_freq;
	u16 fec_freq;
	u16 mpeg_freq;
};

/*---------------------------------------------------------------------
 * DVBAPI stuff
 *---------------------------------------------------------------------*/

struct avl6222_config
{
	int tuner_no;
	struct stpio_pin *tuner_enable_pin;

	u8 demod_address; /*< the demodulator's i2c address */
	u8 tuner_address; /*< the tuner's i2c address */

	u16 demod_freq;  /*< Demod clock in 10kHz units */
	u16 fec_freq;  /*< FEC clock in 10kHz units */
	u16 mpeg_freq;  /*< MPEG clock in 10kHz units */

	u16 i2c_speed_khz;
	u32 agc_polarization;
	u16 max_lpf;

	u32 mpeg_mode;
	u16 mpeg_serial;
	u16 mpeg_clk_mode;

	u32 pll_config;

	u32 tuner_active_lh;

	u32 lpf;
	u8  lock_mode;
	u8  iq_swap;
	u8  auto_iq_swap;

	u16 agc_tri;
	u16 mpeg_tri;

	u32 usedTuner;
	u32 usedLNB;

	u32 lnb[6];
};

// --------------------------------------------------------------------

struct avl6222_diseqc_tx_status
{
	u8 tx_done;
	u8 tx_fifo_cnt;
};

// --------------------------------------------------------------------

struct avl6222_ver_info
{
	u8  major;
	u8  minor;
	u16 build;
	u8  patch_major;
	u8  patch_minor;
	u16 patch_build;
};

// --------------------------------------------------------------------

struct avl6222_tuning
{
	u32 frequency;
	u32 symbol_rate;
	u8 lock_mode;
	u8 auto_iq_swap;
	u8 delsys;
	u8 iq_swap;
	fe_modulation_t delivery;
	u8 inversion_val;
	u8 rolloff_val;
	u8 pilot_val;
	fe_pilot_t pilot;
	fe_rolloff_t rolloff;
	fe_spectral_inversion_t inversion;
};

// --------------------------------------------------------------------

struct avl6222_state
{
	struct i2c_adapter    *i2c;
	struct avl6222_config *config;
	struct dvb_frontend   frontend;
	unsigned char         boot_done;
	unsigned char         diseqc_status;
	void                  *lnb_priv;
	struct equipment_s    equipment;

	/* mutex */
	struct mutex          lock;
	struct mutex          protect;
};

/*---------------------------------------------------------------------
 * Prototypes / Exports
 *---------------------------------------------------------------------*/

u16 avl6222_cpu_halt(struct dvb_frontend *fe);
u16 avl6222_get_version(struct avl6222_state *state, struct avl6222_ver_info *version);
int avl6222_set_tone(struct dvb_frontend *fe, fe_sec_tone_mode_t tone);
int avl6222_set_voltage(struct dvb_frontend *fe, fe_sec_voltage_t voltage);

extern int stv6110a_attach(struct dvb_frontend *fe, void *demod_priv, struct equipment_s *equipment, u32 mclk, u32 max_lpf);

extern void *lnb_pio_attach(u32 *lnb, struct equipment_s *equipment);
extern void *lnbh221_attach(u32 *lnb, struct equipment_s *equipment);
extern void *lnb_a8293_attach(u32 *lnb, struct equipment_s *equipment);

#endif /* _AVL6222_H */
// vim:ts=4
