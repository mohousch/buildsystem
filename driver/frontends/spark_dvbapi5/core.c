#include "core.h"
#include <linux/platform_device.h>
#include <asm/system.h>
#include <asm/io.h>
#include <linux/version.h>
#include <linux/dvb/dmx.h>
#include <linux/proc_fs.h>
#include <linux/stm/pio.h>

#include <pvr_config.h>

#include "vz7903.h" //tuner 7903
short paramDebug = 0;

static struct core *core[MAX_DVB_ADAPTERS];

static struct stv090x_config stv090x_config =
{
	.device = STX7111,
	.demod_mode = STV090x_DUAL,
	.clk_mode = STV090x_CLK_EXT,
	.xtal = 30000000,
	.address = 0x68,
	.ref_clk = 16000000,
	.ts1_mode = STV090x_TSMODE_DVBCI,
	.ts2_mode = STV090x_TSMODE_SERIAL_CONTINUOUS,
	.ts1_clk = 0,
	.ts2_clk = 0,
	.repeater_level = STV090x_RPTLEVEL_64,

	.adc1_range = STV090x_ADC_1Vpp,
	.agc_rf1_inv = 1,
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
	.addr = (0xc0 >> 1),
	.refclk = 16000000,
};

static const struct ix7306_config bs2s7hz7306a_config =
{
	.name = "Sharp BS2S7HZ7306A",
	.addr = (0xc0 >> 1),
	.step_size = IX7306_STEP_1000,
	.bb_lpf = IX7306_LPF_10,
	.bb_gain = IX7306_GAIN_0dB,
};

static struct vz7903_config vz7903_i2cConfig =
{
	.I2cAddr = 0x60,
	.DemodI2cAddr = 0x68,
};

void frontend_find_TunerDevice(enum tuner_type *ptunerType, struct i2c_adapter *i2c,
							   struct dvb_frontend *frontend)
{
	int identify = 0;
	identify = tuner_Sharp7903_Identify(frontend, &vz7903_i2cConfig, (void *)i2c);
	if (identify == 0)
	{
		printk("tuner_Sharp7903 is Identified\n");
		*ptunerType = VZ7903;
		return;
	}
	identify = tuner_Sharp7306_Identify(frontend, &bs2s7hz7306a_config, i2c);
	if (identify == 0)
	{
		printk("tuner_Sharp7306 is Identified\n");
		*ptunerType = SHARP7306;
		return;
	}
	//add new tuner identify here
	*ptunerType = TunerUnknown;
	printk("device unknown\n");
}

static struct dvb_frontend *frontend_init(struct core_config *cfg, int i)
{
	struct stv6110x_devctl *ctl;
	struct dvb_frontend *frontend = NULL;
	enum tuner_type tunerType = TunerUnknown;
	printk(KERN_INFO "%s >\n", __FUNCTION__);
	frontend = stv090x_attach(&stv090x_config, cfg->i2c_adap, STV090x_DEMODULATOR_0, STV090x_TUNER1);
	frontend_find_TunerDevice(&tunerType, cfg->i2c_adap, frontend);
	if (frontend)
	{
		printk("%s: attached\n", __FUNCTION__);
		switch (tunerType)
		{
			case SHARP7306:
				if (dvb_attach(ix7306_attach, frontend, &bs2s7hz7306a_config, cfg->i2c_adap))
				{
					printk("%s: IX7306 attached\n", __FUNCTION__);
					//stv090x_config.xtal = 4000000;
					stv090x_config.agc_rf1_inv = 1;
					stv090x_config.tuner_set_frequency = ix7306_set_frequency;
					stv090x_config.tuner_get_frequency = ix7306_get_frequency;
					stv090x_config.tuner_set_bandwidth = ix7306_set_bandwidth;
					stv090x_config.tuner_get_bandwidth = ix7306_get_bandwidth;
					stv090x_config.tuner_get_status = frontend->ops.tuner_ops.get_status;
				}
				else
				{
					printk(KERN_INFO "%s: error attaching IX7306\n", __FUNCTION__);
					goto error_out;
				}
				break;
			case VZ7903:
				if (dvb_attach(vz7903_attach, frontend, &vz7903_i2cConfig, cfg->i2c_adap))
				{
					printk("%s: vz7903_attach\n", __FUNCTION__);
					stv090x_config.agc_rf1_inv = 1;
					// stv090x_config.tuner_init = nim_vz7903_init;
					stv090x_config.tuner_set_frequency = vz7903_set_frequency;
					stv090x_config.tuner_get_frequency = vz7903_get_frequency;
					stv090x_config.tuner_set_bandwidth = vz7903_set_bandwidth;
					stv090x_config.tuner_get_bandwidth = vz7903_get_bandwidth;
					stv090x_config.tuner_get_status = vz7903_get_status;
				}
				else
				{
					printk(KERN_INFO "%s: error vz7903_attach\n", __FUNCTION__);
					goto error_out;
				}
				break;
			case STV6110X:
			default:
				ctl = dvb_attach(stv6110x_attach, frontend, &stv6110x_config, cfg->i2c_adap);
				if (ctl)
				{
					printk("%s: stv6110x attached\n", __FUNCTION__);
					stv090x_config.agc_rf1_inv = 0;
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
				}
				else
				{
					printk(KERN_INFO "%s: error attaching stv6110x\n", __FUNCTION__);
					goto error_out;
				}
				break;
		}
	}
	else
	{
		printk(KERN_INFO "%s: error attaching\n", __FUNCTION__);
		goto error_out;
	}
	if (frontend->ops.init)
		frontend->ops.init(frontend);
	return frontend;
error_out:
	printk("core: Frontend registration failed!\n");
	if (frontend)
		dvb_frontend_detach(frontend);
	return NULL;
}

static void frontend_term(struct dvb_frontend *frontend)
{
	printk(KERN_INFO "%s >\n", __FUNCTION__);
	dvb_frontend_detach(frontend);
	printk(KERN_INFO "%s <\n", __FUNCTION__);
}

static struct dvb_frontend *
init_stv090x_device(struct dvb_adapter *adapter,
					struct plat_tuner_config *tuner_cfg, int i)
{
	struct fe_core_state *state;
	struct dvb_frontend *frontend;
	struct core_config *cfg;
	printk("> (bus = %d) %s\n", tuner_cfg->i2c_bus, __FUNCTION__);
	cfg = kmalloc(sizeof(struct core_config), GFP_KERNEL);
	if (cfg == NULL)
	{
		printk("stv090x: kmalloc failed\n");
		return NULL;
	}
	memset(cfg, 0, sizeof(struct core_config));
	/* initialize the config data */
	cfg->i2c_adap = i2c_get_adapter(tuner_cfg->i2c_bus);
	printk("i2c adapter = 0x%0x\n", (unsigned int)cfg->i2c_adap);
	core[0]->pCfgCore = cfg;
	cfg->i2c_addr = tuner_cfg->i2c_addr;
	printk("i2c addr = %02x\n", cfg->i2c_addr);
	printk("tuner enable = %d.%d\n", tuner_cfg->tuner_enable[0], tuner_cfg->tuner_enable[1]);
	frontend = frontend_init(cfg, i);
	if (frontend == NULL)
	{
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

static void term_stv090x_device(struct dvb_frontend *frontend)
{
	//struct core_state *state;
	struct core_config *cfg;
	printk("> %s\n", __FUNCTION__);
	dvb_unregister_frontend(frontend);
	frontend_term(frontend);
	cfg = core[0]->pCfgCore;
	//YWI2C_CloseAddress(cfg->hI2CHandle);
	if (cfg->i2c_adap)
	{
		i2c_put_adapter(cfg->i2c_adap);
		cfg->i2c_adap = NULL;
	}
	kfree(cfg);
	core[0]->pCfgCore = NULL;
	printk("< %s\n", __FUNCTION__);
}

struct plat_tuner_config tuner_resources[] =
{
	[0] =
	{
		.adapter = 0,
		.i2c_bus = 3,
		.i2c_addr = 0x68,
	},
};

int stv090x_register_frontend(struct dvb_adapter *dvb_adap)
{
	int i = 0;
	int vLoop = 0;
	printk(KERN_INFO "%s: stv090x DVB: 0.11 \n", __FUNCTION__);
	core[i] = (struct core *) kmalloc(sizeof(struct core), GFP_KERNEL);
	if (!core[i])
		return 0;
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
				init_stv090x_device(core[i]->dvb_adapter, &tuner_resources[vLoop], vLoop);
		}
	}
	printk(KERN_INFO "%s: <\n", __FUNCTION__);
	return 0;
}

EXPORT_SYMBOL(stv090x_register_frontend);

void stv090x_unregister_frontend(struct dvb_adapter *dvb_adap)
{
	int i = 0;
	int vLoop = 0;
	printk(KERN_INFO "%s: stv090x DVB: 0.11 >\n", __FUNCTION__);
	for (vLoop = 0; vLoop < ARRAY_SIZE(tuner_resources); vLoop++)
	{
		printk("%s: term tuner %d\n", __FUNCTION__, vLoop);
		if (core[i]->frontend[vLoop])
		{
			term_stv090x_device(core[i]->frontend[vLoop]);
		}
	}
	core[i] = dvb_adap->priv;
	if (!core[i])
		return;
	kfree(core[i]);
	printk(KERN_INFO "%s: <\n", __FUNCTION__);
	return;
}

EXPORT_SYMBOL(stv090x_unregister_frontend);

int __init stv090x_init(void)
{
	printk("stv090x loaded\n");
	return 0;
}

static void __exit stv090x_exit(void)
{
	printk("stv090x unloaded\n");
}

module_init(stv090x_init);
module_exit(stv090x_exit);

module_param(paramDebug, short, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
MODULE_PARM_DESC(paramDebug, "Debug Output 0=disabled >0=enabled(debuglevel)");

MODULE_DESCRIPTION("Tunerdriver");
MODULE_AUTHOR("Manu Abraham; adapted by TDT");
MODULE_LICENSE("GPL");
