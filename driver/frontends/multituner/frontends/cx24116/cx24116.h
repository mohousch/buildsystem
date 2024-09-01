/*
  Conexant cx24116/cx24118 - DVBS/S2 Satellite demod/tuner driver

  Copyright (C) 2006 Steven Toth <stoth@hauppauge.com>
  Copyright (C) 2006 Georg Acher (acher (at) baycom (dot) de) for Reel Multimedia
                     Added Diseqc, auto pilot tuning and hack for old DVB-API

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifndef _CX24116_H_
#define _CX24116_H_

#include "dvb_frontend.h"
#include "dvbdev.h"
#include "demux.h"
#include "dvb_demux.h"
#include "dmxdev.h"
#include "dvb_filter.h"
#include "dvb_net.h"
#include "dvb_ca_en50221.h"

#include <linux/dvb/frontend.h>
#include "dvb_frontend.h"

#include <linux/proc_fs.h>

#include <linux/dvb/version.h>

#include "cx24116_platform.h"
#include "equipment.h"

/* The Demod/Tuner can't easily provide these, we cache them */
struct cx24116_tuning
{
	u32 			frequency;
	u32 			symbol_rate;
	fe_spectral_inversion_t inversion;
#if DVB_API_VERSION < 5
	enum dvbfe_fec 		fec;
	enum dvbfe_modulation	modulation;

	enum dvbfe_delsys 	delivery;
	enum dvbfe_rolloff 	rolloff;

	int		 	pilot; // 0: off, 1: on (only used for S2)
#else
	fe_code_rate_t		fec;
	fe_delivery_system_t    delsys;
	fe_modulation_t         modulation;
	fe_pilot_t              pilot;
	fe_rolloff_t 	        rolloff;
#endif

	/* Demod values */
	u8 			fec_val;
	u8 			fec_mask;
	u8 			inversion_val;
	u8			fec_numb;
	u8			U1[6];

#if DVB_API_VERSION >= 5
	u8                      pilot_val;
	u8                      rolloff_val;
#endif
};

struct cx24116_config
{
	struct i2c_adapter	*i2c_adap; /* i2c bus of the tuner */
	u8                      i2c_addr; /* i2c address of the tuner */
	u8                      i2c_bus;

	struct stpio_pin	*tuner_enable_pin;
	u32                     tuner_active_lh;

	u32                     useUnknown;
	u32                     usedLNB;
	char                    *fw_name;
	u32                     fastDelay;

	u32                     lnb[6];

};

enum cmds
{
	CMD_SET_VCO     = 0x10,
	CMD_TUNEREQUEST = 0x11,
	CMD_MPEGCONFIG  = 0x13,
	CMD_TUNERINIT   = 0x14,
	CMD_BANDWIDTH   = 0x15,
	CMD_GETAGC      = 0x19,
	CMD_LNBCONFIG   = 0x20,
	CMD_LNBSEND     = 0x21,
	CMD_SET_TONEPRE = 0x22,
	CMD_SET_TONE    = 0x23,
	CMD_UPDFWVERS   = 0x35,
	CMD_TUNERSLEEP  = 0x36,
	CMD_AGCCONTROL  = 0x3b,
	CMD_U1          = 0x16,
	CMD_U2          = 0x17,
	CMD_MAX         = 0xFF
};

/* Basic commands that are sent to the firmware */
struct cx24116_cmd
{
	enum cmds id;
	u8        len;
	u8        args[0x1e];
};

struct cx24116_state
{
	struct dvb_frontend_ops ops;
	struct dvb_frontend     frontend;

	struct cx24116_config   *config;

	struct cx24116_tuning 	dcur;
	struct cx24116_tuning 	dnxt;

	struct semaphore	fw_load_sem;

	struct cx24116_cmd 	dsec_cmd;

	int                     not_responding;

	void                    *lnb_priv;
	struct equipment_s       equipment;
};

extern void *lnb_pio_attach(u32 *lnb, struct equipment_s *equipment);
extern void *lnbh221_attach(u32 *lnb, struct equipment_s *equipment);

#endif // _CX24116_H_
// vim:ts=4