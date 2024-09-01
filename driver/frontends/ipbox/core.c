#include "core.h"

#include <linux/platform_device.h>
#include <asm/system.h>
#include <asm/io.h>
#include <linux/dvb/dmx.h>
#include <linux/proc_fs.h>
#include <linux/version.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,23)
#include <linux/stm/pio.h>
#else
#include <linux/stpio.h>
#endif

#include "pvr_config.h"

enum
{
	NIM_NOTFOUND = 0,
	NIM_DVBS2_SAMSUNG_V1, // DNBU10511IST ver4.0
	NIM_DVBT_SAMSUNG_V1, //
	NIM_DVBC_PHILIPS_V1,
};

// ----------------- DVB-S2 tuner -----------------------------
#include "stv6110x.h"
#include "stv090x.h"

static struct stv090x_config stv090x_config =
{
	.device = STV0903,
	.demod_mode = STV090x_DUAL/*STV090x_SINGLE*/,
	.clk_mode = STV090x_CLK_EXT,

	.xtal = 8000000,
	.address = 0x68,

	.ts1_mode = STV090x_TSMODE_DVBCI/*STV090x_TSMODE_SERIAL_CONTINUOUS*/,
	.ts2_mode = STV090x_TSMODE_DVBCI/*STV090x_TSMODE_SERIAL_CONTINUOUS*/,
	.ts1_clk = 0,
	.ts2_clk = 0,

	.lnb_enable = NULL,
	.lnb_vsel = NULL,

	.repeater_level = STV090x_RPTLEVEL_16,

	.tuner_init = NULL,
	.tuner_set_mode = NULL,
	.tuner_set_frequency = NULL,
	.tuner_get_frequency = NULL,
	.tuner_set_bandwidth = NULL,
	.tuner_get_bandwidth = NULL,
	.tuner_set_bbgain = NULL,
	.tuner_get_bbgain = NULL,
	.tuner_set_refclk = NULL,
	.tuner_get_status = NULL,
};

static struct stv6110x_config stv6110x_config =
{
	.addr = 0x60,
	.refclk = 16000000,
	.clk_div = 2,
};

// ----------------- DVB-T tuner -----------------------------
#include "zl10353.h"
#include "mxl5007t.h"
static struct zl10353_config zl10353_config =
{
	.demod_address = 0x1A >> 1,

	/* frequencies in units of 0.1kHz */
	//.adc_clock; /* default: 450560 (45.056 MHz) */
	.if2 = 45710, /* default: 361667 (36.1667 MHz) */

	/* set if no pll is connected to the secondary i2c bus */
	.no_tuner = 0,
	//.tun_address = 0xC0,

	/* set if parallel ts output is required */
	.parallel_ts = 1,

	/* set if i2c_gate_ctrl disable is required */
	.disable_i2c_gate_ctrl = 0,

	/* clock control registers (0x51-0x54) */
	//.clock_ctl_1 = 0, /* default: 0x46 */
	//.pll_0 = 0, /* default: 0x15 */
};
static struct mxl5007t_config mxl5007t_config =
{
	.if_diff_out_level = 0,
	.clk_out_amp = MxL_CLKOUT_AMP_0_94V,
	.xtal_freq_hz = MxL_XTAL_20_48_MHZ,
	.if_freq_hz = MxL_IF_4_57_MHZ,
	.invert_if = 0,
	.loop_thru_enable = 0,
	.clk_out_enable = 1,
};

#include "tda1002x.h"
static struct tda10023_config tda10023_config =
{
	.demod_address = 0xC,
	.invert = 0,
	.xtal = 28920000, // defaults: 28920000
	.pll_m = 8, // defaults: 8
	.pll_p = 4, // defaults: 4
	.pll_n = 1, // defaults: 1
	.output_mode = TDA10023_OUTPUT_MODE_PARALLEL_A,
	.deltaf = 0,
};
// -------------------- end of tuner definitions -------------

short paramDebug = 1;

static struct core *core[MAX_DVB_ADAPTERS];

enum { VOLTAGE_13 = 1, VOLTAGE_18 = 0 };
#if defined(IPBOX55)
enum { VOLTAGE_ON = 1, VOLTAGE_OFF = 0 }; // 55 new
#else
enum { VOLTAGE_ON = 0, VOLTAGE_OFF = 1 }; // 99XX, 55 old
#endif
static int pio_set_voltage(struct dvb_frontend *fe, fe_sec_voltage_t voltage)
{
	struct core_config *cfg = fe->sec_priv;
	//int id = stb710x->id;
	switch (voltage)
	{
		case SEC_VOLTAGE_13:
			printk("frontend: set_voltage_vertical \n");
			stpio_set_pin(cfg->lnb_enable, VOLTAGE_ON);
			stpio_set_pin(cfg->lnb_vsel, VOLTAGE_13);
			break;
		case SEC_VOLTAGE_18:
			printk("frontend: set_voltage_horizontal\n");
			stpio_set_pin(cfg->lnb_enable, VOLTAGE_ON);
			stpio_set_pin(cfg->lnb_vsel, VOLTAGE_18);
			break;
		case SEC_VOLTAGE_OFF:
			printk("frontend: set_voltage_off\n");
// stpio_set_pin (cfg->lnb_enable, VOLTAGE_OFF);
//		break;
		default:
			return -EINVAL;
	}
	return 0;
}

static struct dvb_frontend *frontend_init(struct core_config *cfg, int slot, int nimtype)
{
	struct stv6110x_devctl *ctl;
	struct dvb_frontend *frontend = NULL;
	printk(KERN_INFO "%s >\n", __FUNCTION__);
	switch (nimtype)
	{
		case NIM_DVBS2_SAMSUNG_V1:
			frontend = dvb_attach(stv090x_attach, &stv090x_config, cfg->i2c_adap, STV090x_DEMODULATOR_0);
			if (frontend)
			{
				printk("%s: demod attached\n", __FUNCTION__);
				ctl = dvb_attach(stv6110x_attach, frontend, &stv6110x_config, cfg->i2c_adap);
				if (ctl)
				{
					printk("%s: pll attached\n", __FUNCTION__);
					stv090x_config.tuner_init = ctl->tuner_init;
					stv090x_config.tuner_set_mode = ctl->tuner_set_mode;
					stv090x_config.tuner_set_frequency = ctl->tuner_set_frequency;
					stv090x_config.tuner_get_frequency = ctl->tuner_get_frequency;
					stv090x_config.tuner_set_bandwidth = ctl->tuner_set_bandwidth;
					stv090x_config.tuner_get_bandwidth = ctl->tuner_get_bandwidth;
					stv090x_config.tuner_set_bbgain = ctl->tuner_set_bbgain;
					stv090x_config.tuner_get_bbgain = ctl->tuner_get_bbgain;
					stv090x_config.tuner_set_refclk = ctl->tuner_set_refclk;
					stv090x_config.tuner_get_status = ctl->tuner_get_status;
#if 0
					stv090x_config.lnb_enable = cfg->lnb_enable;
					stv090x_config.lnb_vsel = cfg->lnb_vsel;
#endif
					/* override frontend ops */
					frontend->ops.set_voltage = pio_set_voltage;
					frontend->sec_priv = cfg;
				}
				else
				{
					printk(KERN_INFO "%s: error attaching pll\n", __FUNCTION__);
					if (frontend)
						dvb_frontend_detach(frontend);
					frontend = NULL;
				}
			}
			break;
		case NIM_DVBT_SAMSUNG_V1:
#if defined(IPBOX9900) || defined(IPBOX99)
			frontend = dvb_attach(zl10353_attach, &zl10353_config, cfg->i2c_adap);
			if (frontend)
			{
				printk("%s: demod attached\n", __FUNCTION__);
				frontend = dvb_attach(mxl5007t_attach, frontend, cfg->i2c_adap, 0xC0 >> 1, &mxl5007t_config);
				if (frontend)
				{
					printk("%s: pll attached\n", __FUNCTION__);
				}
				else
				{
					printk(KERN_INFO "%s: error attaching pll\n", __FUNCTION__);
				}
			}
#else
			printk(" WARN: DVB-T not supported, yet ;)\n");
			frontend = NULL;
#endif
			break;
		case NIM_DVBC_PHILIPS_V1:
#if defined(IPBOX9900) || defined(IPBOX99)
			frontend = dvb_attach(tda10023_attach, &tda10023_config, cfg->i2c_adap, 0);
			if (frontend)
			{
				printk("%s: demod attached\n", __FUNCTION__);
			}
			else
			{
				printk(KERN_INFO "%s: error attaching pll\n", __FUNCTION__);
				frontend = NULL;
			}
#else
			printk(" WARN: DVB-C not supported, yet ;)\n");
			frontend = NULL;
#endif
			break;
		default:
			printk(KERN_INFO "%s: unknown nim type\n", __FUNCTION__);
			if (frontend)
				dvb_frontend_detach(frontend);
			frontend = NULL;
			break;
	}
	return frontend;
}

// ------ NIM database ------------

#define NIM_SUPPORTED 3

struct nim_db_t
{
	int nim_type;
	unsigned char i2c_addr;
	unsigned char i2c_addr2; // if needed, otherwise 0x00
	char *brandname;
};

static struct nim_db_t nim_db[] =
{
	{
		NIM_DVBS2_SAMSUNG_V1,
		0xD0 >> 1, //0x68
		0x00,
		"DVB-S2 Samsung (stv0903+stv6110), v1.0",
	},
	{
		NIM_DVBT_SAMSUNG_V1,
		0x1A >> 1, //0x0D
		0x00,
		"DVB-T Samsung (zl10535+mlt5007t), v1.0",
	},
	{
		NIM_DVBC_PHILIPS_V1,
		0xC,
		0x00,
		"DVB-C Philips (TDA10023+MxL201RF), v1.0",
	},
};

// ------ end of NIM db ------------

//#define I2C_NIM_AUTODETECT
static int i2c_autodetect(struct i2c_adapter *adapter, unsigned char i2c_addr, unsigned char dev_addr)
{
	unsigned char buf[2] = { 0, 0 };
	struct i2c_msg msg[] =
	{
		{ .addr = i2c_addr, .flags = 0, .buf = &dev_addr, .len = 1 },
		{ .addr = i2c_addr, .flags = I2C_M_RD, .buf = &buf[0], .len = 1 }
	};
	int b;
#ifdef I2C_NIM_AUTODETECT
	printk("[%s] adr %02X, id %02x\n", __FUNCTION__, i2c_addr, adapter->id);
#endif
	b = i2c_transfer(adapter, msg, 1);
#ifdef I2C_NIM_AUTODETECT
	printk("[%s] i2c_transfer=%02X\n", __FUNCTION__, b);
#endif
	b |= i2c_transfer(adapter, msg + 1, 1);
#ifdef I2C_NIM_AUTODETECT
	printk("[%s] i2c_transfer=%02X\n", __FUNCTION__, b);
#endif
	if (b != 1)
		return -1;
#ifdef I2C_NIM_AUTODETECT
	printk("[%s] buf[0]=%02X\n", __FUNCTION__, buf[0]);
#endif
	return buf[0];
}

static struct dvb_frontend *
init_fe_device(struct dvb_adapter *adapter,
			   struct plat_tuner_config *tuner_cfg, int slot)
{
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,23)
	struct _core_state *state;
#else
	struct core_state *state;
#endif
	struct dvb_frontend *frontend;
	struct core_config *cfg;
	int nim, found = -1;
	printk("> (bus = %d) %s\n", tuner_cfg->i2c_bus, __FUNCTION__);
	cfg = kzalloc(sizeof(struct core_config), GFP_KERNEL);
	if (cfg == NULL)
	{
		printk("fe-core: kmalloc failed\n");
		return NULL;
	}
	/* initialize the config data */
	if ((cfg->i2c_adap = i2c_get_adapter(tuner_cfg->i2c_bus)) == NULL)
	{
		printk(" i2c-%d unavailable\n", tuner_cfg->i2c_bus);
		kfree(cfg);
		return NULL;
	}
	printk(" i2c adapter = 0x%0x\n", (unsigned int)cfg->i2c_adap);
	printk(" tuner reset = %d.%d\n", tuner_cfg->tuner_enable[0], tuner_cfg->tuner_enable[1]);
	cfg->tuner_reset_act = tuner_cfg->tuner_enable[2];
	cfg->tuner_reset_pin = stpio_request_pin(tuner_cfg->tuner_enable[0],
											 tuner_cfg->tuner_enable[1],
											 slot ? "TUNER1 RST" : "TUNER0 RST", STPIO_OUT);
	if (cfg->tuner_reset_pin == NULL)
	{
		printk("fe_core: failed to allocate nim resource: RESET\n");
		kfree(cfg);
		return NULL;
	}
	/* set to low */
	stpio_set_pin(cfg->tuner_reset_pin, !cfg->tuner_reset_act);
	/* Wait for everything to die */
	msleep(50);
	/* Pull it up out of Reset state */
	stpio_set_pin(cfg->tuner_reset_pin, cfg->tuner_reset_act);
	/* Wait for PLL to stabilize */
	msleep(250);
	/*
	 * PLL state should be stable now. Ideally, we should check
	 * for PLL LOCK status. But well, never mind!
	 */
	//
	// do autodetection
	//
	for (nim = 0; nim < NIM_SUPPORTED; nim++) //FIXME: NIM_SUPPORTED change to ARRAY_SIZE(nim_db)
	{
		if (i2c_autodetect(cfg->i2c_adap, nim_db[nim].i2c_addr /* Address */, 0x0) != -1)
		{
			printk("* Autodetection on NIM#%d: Found '%s'\n", nim, nim_db[nim].brandname);
			found = nim_db[nim].nim_type;
			cfg->i2c_addr = nim_db[nim].i2c_addr;
			printk(" i2c addr = %02x\n", cfg->i2c_addr);
			break;
		}
	}
	if (found < 0)
	{
		// no any known nim was detected
		stpio_free_pin(cfg->tuner_reset_pin);
		kfree(cfg);
		return NULL;
	}
	switch (found)
	{
		case NIM_DVBS2_SAMSUNG_V1:
			cfg->lnb_enable = stpio_request_pin(tuner_cfg->lnb_enable[0],
												tuner_cfg->lnb_enable[1],
												slot ? "LNB1 POWER" : "LNB0 POWER", STPIO_OUT);
			if (cfg->lnb_enable == NULL)
			{
				printk("fe_core: failed to allocate resource: LNB POWER\n");
				stpio_free_pin(cfg->tuner_reset_pin);
				kfree(cfg);
				return NULL;
			}
			cfg->lnb_vsel = stpio_request_pin(tuner_cfg->lnb_vsel[0],
											  tuner_cfg->lnb_vsel[1],
											  slot ? "LNB1 13/18" : "LNB0 13/18", STPIO_OUT);
			if (cfg->lnb_vsel == NULL)
			{
				printk("fe_core: failed to allocate resource: LNB 13/18\n");
				stpio_free_pin(cfg->tuner_reset_pin);
				stpio_free_pin(cfg->lnb_enable);
				kfree(cfg);
				return NULL;
			}
			break;
		default:
			break;
	}
	frontend = frontend_init(cfg, slot, found);
	if (frontend == NULL)
	{
		printk("No frontend found !\n");
		stpio_free_pin(cfg->tuner_reset_pin);
		if (cfg->lnb_enable)
			stpio_free_pin(cfg->lnb_enable);
		if (cfg->lnb_vsel)
			stpio_free_pin(cfg->lnb_vsel);
		kfree(cfg);
		return NULL;
	}
	printk(KERN_INFO "%s: Call dvb_register_frontend (adapter = 0x%x)\n",
		   __FUNCTION__, (unsigned int) adapter);
	if (dvb_register_frontend(adapter, frontend))
	{
		printk("%s: Frontend registration failed !\n", __FUNCTION__);
		if (frontend->ops.release)
			frontend->ops.release(frontend);
		return NULL;
	}
	state = frontend->demodulator_priv;
	return frontend;
}

static struct plat_tuner_config tuner_resources[] =
{

	[0] = {
		.adapter = 0,
		.i2c_bus = 0,
		.i2c_addr = 0x68,
		.tuner_enable = {4, 4, 1},
		.lnb_enable = {3, 4, 1},
		.lnb_vsel = {3, 2, 0},
	},
	[1] = {
		.adapter = 0,
		.i2c_bus = 2,
		.i2c_addr = 0x68,
		.tuner_enable = {4, 5, 1},
		.lnb_enable = {5, 3, 1},
		.lnb_vsel = {5, 2, 0},
	}
};

void fe_core_register_frontend(struct dvb_adapter *dvb_adap)
{
	int i = 0;
	int vLoop = 0;
	printk(KERN_INFO "%s: DVB: Sisyfos frontend core (w/ autodetect)\n", __FUNCTION__);
	core[i] = (struct core *) kmalloc(sizeof(struct core), GFP_KERNEL);
	if (!core[i])
		return;
	memset(core[i], 0, sizeof(struct core));
	core[i]->dvb_adapter = dvb_adap;
	dvb_adap->priv = core[i];
	printk("tuner = %d\n", ARRAY_SIZE(tuner_resources));
	for (vLoop = 0; vLoop < ARRAY_SIZE(tuner_resources); vLoop++)
	{
		if (core[i]->frontend[vLoop] == NULL)
		{
			printk("%s: init tuner %d\n", __FUNCTION__, vLoop);
			core[i]->frontend[vLoop] =
				init_fe_device(core[i]->dvb_adapter, &tuner_resources[vLoop], vLoop);
		}
	}
	printk(KERN_INFO "%s: <\n", __FUNCTION__);
	return;
}

EXPORT_SYMBOL(fe_core_register_frontend);

int __init fe_core_init(void)
{
	printk("frontend core loaded\n");
	return 0;
}

static void __exit fe_core_exit(void)
{
	printk("frontend core unloaded\n");
}

module_init(fe_core_init);
module_exit(fe_core_exit);

MODULE_DESCRIPTION("Tuner driver");
MODULE_AUTHOR("Sisyfos");
MODULE_LICENSE("GPL");
