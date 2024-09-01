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

#ifdef STLINUX20
#include "dvb_new_frontend.h"
#endif

#define MAX_DVB_ADAPTERS 4
#define MAX_TUNERS_PER_ADAPTER 4

/* The Demod/Tuner can't easily provide these, we cache them */
struct cx24116_tuning
{
	u32 frequency;
	u32 symbol_rate;
	fe_spectral_inversion_t inversion;
#if DVB_API_VERSION < 5
	enum dvbfe_fec fec;
	enum dvbfe_modulation modulation;

	enum dvbfe_delsys delivery;
	enum dvbfe_rolloff rolloff;
#else
	fe_code_rate_t fec;
	fe_modulation_t modulation;
	fe_delivery_system_t delivery;
	fe_rolloff_t rolloff;
#endif

	int pilot; // 0: off, 1: on (only used for S2)

	/* Demod values */
	u8 fec_val;
	u8 fec_mask;
	u8 inversion_val;
	u8 fec_numb;
	u8 U1[6];
};

struct cx24116_config
{
	struct i2c_adapter *i2c_adap; /* i2c bus of the tuner */
	u8 i2c_addr; /* i2c address of the tuner */
	struct stpio_pin *tuner_enable_pin;
	struct stpio_pin *lnb_enable_pin;
	struct stpio_pin *lnb_vsel_pin;
	u8 tuner_enable_act; /* active state of the pin */
	u8 lnb_enable_act; /* active state of the pin */
	u8 lnb_vsel_act; /* active state of the pin */
};

struct cx24116_core
{
	struct dvb_adapter *dvb_adap;

	struct dvb_frontend *frontend[MAX_TUNERS_PER_ADAPTER];
};

enum cmds
{
	CMD_SET_VCO = 0x10,
	CMD_TUNEREQUEST = 0x11,
	CMD_MPEGCONFIG = 0x13,
	CMD_TUNERINIT = 0x14,
	CMD_BANDWIDTH = 0x15,
	CMD_GETAGC = 0x19,
	CMD_LNBCONFIG = 0x20,
	CMD_LNBSEND = 0x21,
	CMD_SET_TONEPRE = 0x22,
	CMD_SET_TONE = 0x23,
	CMD_UPDFWVERS = 0x35,
	CMD_TUNERSLEEP = 0x36,
	CMD_AGCCONTROL = 0x3b,
	CMD_U1 = 0x16,
	CMD_U2 = 0x17,
	CMD_MAX = 0xFF
};

/* Basic commands that are sent to the firmware */
struct cx24116_cmd
{
	enum cmds id;
	u8 len;
	u8 args[0x1e];
};

struct cx24116_state
{
	struct dvb_frontend_ops ops;
	struct dvb_frontend frontend;

	const struct cx24116_config *config;

	struct cx24116_tuning dcur;
	struct cx24116_tuning dnxt;

	struct semaphore fw_load_sem;
	/* FIXME: remove thread_id if not using loader thread */
	int thread_id;

	struct cx24116_cmd dsec_cmd;

	int not_responding;

#if defined(TUNER_PROCFS)
	struct proc_dir_entry *proc_tuner;
	u8 value[5];
#endif
};

extern void cx24116_register_frontend(struct dvb_adapter *dvb_adap);

#endif // _CX24116_H_
