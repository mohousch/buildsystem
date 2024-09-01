#include "core.h"
#include "stv6110x.h"
#include "ix7306.h"
#include "stv090x.h"
#include <linux/platform_device.h>
#include <asm/system.h>
#include <asm/io.h>
#include <linux/dvb/dmx.h>
#include <linux/proc_fs.h>

#include <pvr_config.h>

short paramDebug = 0;
int bbgain = -1;

int tunerType = STV6110X;
static char *tuner = "stv6110x";

static struct core *core[MAX_DVB_ADAPTERS];

static struct stv090x_config tt1600_stv090x_config =
{
#if defined(FORTIS_HDBOX)
	.device = STV0903,
	.demod_mode = STV090x_DUAL /*STV090x_SINGLE*/,
	.xtal = 8000000,
	.ts1_mode = STV090x_TSMODE_DVBCI /*STV090x_TSMODE_SERIAL_CONTINUOUS*/,
	.ts2_mode = STV090x_TSMODE_NOTSET,
	.repeater_level = STV090x_RPTLEVEL_16,
#elif defined(UFS912) \
 || defined(HS7110) \
 || defined(HS7810A) \
 || defined(HS7420) \
 || defined(HS7429) \
 || defined(HS7119) \
 || defined(HS7819) \
 || defined(ATEMIO520) \
 || defined(ATEMIO530) \
 || defined(SPARK)
	.device = STX7111,
	.demod_mode = STV090x_DUAL,
	.xtal = 30000000,
	.ts1_mode = STV090x_TSMODE_DVBCI,
	.ts2_mode = STV090x_TSMODE_SERIAL_CONTINUOUS,
	.repeater_level = STV090x_RPTLEVEL_64,
#else
#warning Architecture not supported
#endif
	.clk_mode = STV090x_CLK_EXT,
	.address = 0x68,
	.ref_clk = 16000000,
	.ts1_clk = 0, /* these regs are not set in orig */
	.ts2_clk = 0, /* these regs are not set in orig */

#if !defined(SPARK)
	.tuner_bbgain = 10,
	.adc1_range = STV090x_ADC_1Vpp,
	.adc2_range = STV090x_ADC_2Vpp,
	.diseqc_envelope_mode = false,
#endif
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
};

static const struct ix7306_config bs2s7hz7306a_config =
{
	.name = "Sharp BS2S7HZ7306A",
	.addr = 0x60,
	.step_size = IX7306_STEP_1000,
	.bb_lpf = IX7306_LPF_12,
	.bb_gain = IX7306_GAIN_2dB,
};

static struct dvb_frontend *frontend_init(struct core_config *cfg, int i)
{
	struct tuner_devctl *ctl = NULL;
	struct dvb_frontend *frontend = NULL;
	printk(KERN_INFO "[stv090x] %s >\n", __FUNCTION__);
#if defined(UFS912) \
 || defined(HS7110) \
 || defined(HS7810A) \
 || defined(HS7420) \
 || defined(HS7429) \
 || defined(HS7119) \
 || defined(HS7819) \
 || defined(ATEMIO520) \
 || defined(ATEMIO530) \
 || defined(SPARK)
	frontend = stv090x_attach(&tt1600_stv090x_config, cfg->i2c_adap, STV090x_DEMODULATOR_0, STV090x_TUNER1);
#else
	if (i == 0)
	{
		frontend = stv090x_attach(&tt1600_stv090x_config, cfg->i2c_adap, STV090x_DEMODULATOR_0, STV090x_TUNER1);
	}
	else
	{
		/* Dagobert commented: is this correct? second tuner uses demod0 ??? */
		frontend = stv090x_attach(&tt1600_stv090x_config, cfg->i2c_adap, STV090x_DEMODULATOR_0, STV090x_TUNER2);
	}
#endif
	if (frontend)
	{
		printk("[stv090x] %s: attached frontend\n", __FUNCTION__);
		switch (tunerType)
		{
#if !defined(FORTIS_HDBOX)
			case SHARP7306:
			{
				ctl = dvb_attach(ix7306_attach, frontend, &bs2s7hz7306a_config, cfg->i2c_adap);
				break;
			}
#endif
			case STV6110X:
			default:
			{
				ctl = dvb_attach(stv6110x_attach, frontend, &stv6110x_config, cfg->i2c_adap);
			}
		}
		if (ctl)
		{
			printk("[stv090x] %s: %s attached\n", __FUNCTION__, tuner);
			tt1600_stv090x_config.tuner_init = ctl->tuner_init;
			tt1600_stv090x_config.tuner_set_mode = ctl->tuner_set_mode; //warning: incompatible pointer type
			tt1600_stv090x_config.tuner_set_frequency = ctl->tuner_set_frequency;
			tt1600_stv090x_config.tuner_get_frequency = ctl->tuner_get_frequency;
			tt1600_stv090x_config.tuner_set_bandwidth = ctl->tuner_set_bandwidth;
			tt1600_stv090x_config.tuner_get_bandwidth = ctl->tuner_get_bandwidth;
			tt1600_stv090x_config.tuner_set_bbgain = ctl->tuner_set_bbgain;
			tt1600_stv090x_config.tuner_get_bbgain = ctl->tuner_get_bbgain;
			tt1600_stv090x_config.tuner_set_refclk = ctl->tuner_set_refclk;
			tt1600_stv090x_config.tuner_get_status = ctl->tuner_get_status;
		}
		else
		{
			printk(KERN_INFO "[stv090x] %s: error attaching %s\n", __FUNCTION__, tuner);
			goto error_out;
		}
	}
	else
	{
		printk(KERN_INFO "[stv090x] %s: error attaching frontend\n", __FUNCTION__);
		goto error_out;
	}
	return frontend;
error_out:
	printk("[stv090x] Frontend registration failed!\n");
	if (frontend)
	{
		dvb_frontend_detach(frontend);
	}
	return NULL;
}

static struct dvb_frontend *init_stv090x_device(struct dvb_adapter *adapter, struct plat_tuner_config *tuner_cfg, int i)
{
	struct dvb_frontend *frontend;
	struct core_config *cfg;
	printk("[stv090x] > (bus = %d) %s\n", tuner_cfg->i2c_bus, __FUNCTION__);
	cfg = kmalloc(sizeof(struct core_config), GFP_KERNEL);
	if (cfg == NULL)
	{
		printk("[stv090x] kmalloc failed\n");
		return NULL;
	}
	/* initialize the config data */
	cfg->tuner_enable_pin = NULL;
	cfg->i2c_adap = i2c_get_adapter(tuner_cfg->i2c_bus);
	printk("[stv090x] I2C adapter = 0x%0x\n", cfg->i2c_adap);
	cfg->i2c_addr = tuner_cfg->i2c_addr;
	printk("[stv090x] I2C addr = 0x%02x\n", cfg->i2c_addr);
#if !defined(SPARK)
	printk("[stv090x] tuner enable = %d.%d\n", tuner_cfg->tuner_enable[0], tuner_cfg->tuner_enable[1]);
	cfg->tuner_enable_pin = stpio_request_pin(tuner_cfg->tuner_enable[0], tuner_cfg->tuner_enable[1], "tuner enable", STPIO_OUT);
	if (cfg->tuner_enable_pin == NULL)
	{
		printk("[stv090x] Failed to allocate resources (tuner enable pin)\n");
		goto error;
	}
#endif
	if (cfg->i2c_adap == NULL)
	{
		printk("[stv090x] failed to allocate resources (i2c adapter)\n");
		goto error;
	}
	cfg->tuner_enable_act = tuner_cfg->tuner_enable[2];
	if (cfg->tuner_enable_pin != NULL)
	{
		stpio_set_pin(cfg->tuner_enable_pin, !cfg->tuner_enable_act);
		stpio_set_pin(cfg->tuner_enable_pin, cfg->tuner_enable_act);
#if defined(UFS912) \
 || defined(HS7110) \
 || defined(HS7810A) \
 || defined(HS7420) \
 || defined(HS7429) \
 || defined(HS7119) \
 || defined(HS7819) \
 || defined(ATEMIO520) \
 || defined(ATEMIO530)
		/* give the tuner some time to power up. trial fix for tuner
		 * not available after boot on some boxes.
		 *
		 */
		msleep(250);
#endif
	}
	frontend = frontend_init(cfg, i);
	if (frontend == NULL)
	{
		printk("[stv090x] frontend init failed!\n");
		goto error;
	}
	printk(KERN_INFO "[stv090x] %s: Call dvb_register_frontend (adapter = 0x%x)\n", __FUNCTION__, (unsigned int) adapter);
	if (dvb_register_frontend(adapter, frontend))
	{
		printk("[stv090x] frontend registration failed !\n");
		if (frontend->ops.release)
		{
			frontend->ops.release(frontend);
		}
		goto error;
	}
	return frontend;
error:
	if (cfg->tuner_enable_pin != NULL)
	{
		printk("[stv090x] freeing tuner enable pin\n");
		stpio_free_pin(cfg->tuner_enable_pin);
	}
	kfree(cfg);
	return NULL;
}

struct plat_tuner_config tuner_resources[] =
{
#if defined(FORTIS_HDBOX)
	[0] = {
		.adapter = 0,
		.i2c_bus = 0,
		.i2c_addr = 0x68,
		.tuner_enable = {2, 2, 1},
	},
	[1] = {
		.adapter = 0,
		.i2c_bus = 1,
		.i2c_addr = 0x68,
		.tuner_enable = {2, 4, 1},
	},
#elif defined(UFS912)
	[0] = {
		.adapter = 0,
		.i2c_bus = 3,
		.i2c_addr = 0x68,
		.tuner_enable = {2, 4, 1},
	},
#elif defined(HS7110) \
 || defined(HS7810A) \
 || defined(HS7420) \
 || defined(HS7429) \
 || defined(HS7119) \
 || defined(HS7819) \
 || defined(ATEMIO520) \
 || defined(ATEMIO530)
	[0] = {
		.adapter = 0,
		.i2c_bus = 3,
		.i2c_addr = 0x68,
		.tuner_enable = {3, 3, 1},
	},
#elif defined(SPARK)
	[0] = {
		.adapter = 0,
		.i2c_bus = 3,
		.i2c_addr = 0x68,
	},
#else
#warning Architecture not supported
#endif
};

void stv090x_register_frontend(struct dvb_adapter *dvb_adap)
{
	int i = 0;
	int vLoop = 0;
	printk(KERN_INFO "[stv090x] %s: stv090x DVB: 0.11 \n", __FUNCTION__);
	core[i] = (struct core *) kmalloc(sizeof(struct core), GFP_KERNEL);
	if (!core[i])
	{
		return;
	}
	memset(core[i], 0, sizeof(struct core));
	core[i]->dvb_adapter = dvb_adap;
	dvb_adap->priv = core[i];
	printk("[stv090x] tuner = %d\n", ARRAY_SIZE(tuner_resources));
	for (vLoop = 0; vLoop < ARRAY_SIZE(tuner_resources); vLoop++)
	{
		if (core[i]->frontend[vLoop] == NULL)
		{
			printk("[stv090x] %s: init tuner %d\n", __FUNCTION__, vLoop);
			core[i]->frontend[vLoop] = init_stv090x_device(core[i]->dvb_adapter, &tuner_resources[vLoop], vLoop);
		}
	}
	printk(KERN_INFO "[stv090x] %s: <\n", __FUNCTION__);
	return;
}

EXPORT_SYMBOL(stv090x_register_frontend);

int __init stv090x_init(void)
{
	if ((tuner[0] == 0) || (strcmp("stv6110x", tuner) == 0))
	{
		tunerType = STV6110X;
	}
	else if (strcmp("sharp7306", tuner) == 0)
	{
		tunerType = SHARP7306;
	}
	printk("[stv090x] module stv090x loaded, tuner: %s\n", tuner);
	return 0;
}

static void __exit stv090x_exit(void)
{
	printk("[stv090x] module stv090x unloaded\n");
}

module_init(stv090x_init);
module_exit(stv090x_exit);

module_param(paramDebug, short, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
MODULE_PARM_DESC(paramDebug, "Debug Output 0=disabled >0=enabled(debuglevel)");

module_param(bbgain, int, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
MODULE_PARM_DESC(bbgain, "default=-1 (use default config = 10");

module_param(tuner, charp, 0);
MODULE_PARM_DESC(tuner, "tuner type: stv6110x, sharp7306 (default stv6110x");

MODULE_DESCRIPTION("Tunerdriver");
MODULE_AUTHOR("Manu Abraham; adapted by TDT");
MODULE_LICENSE("GPL");
