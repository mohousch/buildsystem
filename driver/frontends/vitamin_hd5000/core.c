#include "core.h"
#include "stv6110x.h"
#include "stv090x.h"
#include <linux/platform_device.h>
#include <asm/system.h>
#include <asm/io.h>
#include <linux/dvb/dmx.h>
#include <linux/proc_fs.h>

#include <pvr_config.h>

short paramDebug = 0;
int bbgain = -1;

static struct core *core[MAX_DVB_ADAPTERS];

static struct stv090x_config vitamin_hd5000_stv090x_config =
{
	.device = STX7111,
	.demod_mode = STV090x_SINGLE,
	.clk_mode = STV090x_CLK_EXT,
	.xtal = 30000000,
	.address = 0x68,
	.ref_clk = 16000000,
	.ts1_mode = STV090x_TSMODE_DVBCI, //STV090x_TSMODE_PARALLEL_PUNCTURED,
	.ts2_mode = STV090x_TSMODE_SERIAL_PUNCTURED, //STV090x_TSMODE_PARALLEL_PUNCTURED, //STV090x_TSMODE_DVBCI, //STV090x_TSMODE_NOTSET,
	.ts1_clk = 0, /* diese regs werden in orig nicht gesetzt */
	.ts2_clk = 0, /* diese regs werden in orig nicht gesetzt */
	.repeater_level = STV090x_RPTLEVEL_64,

	.tuner_bbgain = 10,
	.adc1_range = STV090x_ADC_1Vpp,//1Vpp
	.adc2_range = STV090x_ADC_2Vpp,

	.diseqc_envelope_mode = false,

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

static struct dvb_frontend *frontend_init(struct core_config *cfg, int i)
{
	struct tuner_devctl *ctl = NULL;
	struct dvb_frontend *frontend = NULL;
	printk(KERN_INFO "%s >\n", __FUNCTION__);
	frontend = stv090x_attach(&vitamin_hd5000_stv090x_config, cfg->i2c_adap, STV090x_DEMODULATOR_0, STV090x_TUNER1);
	if (frontend)
	{
		printk("%s: DEMOD attached\n", __FUNCTION__);
		ctl = dvb_attach(stv6110x_attach, frontend, &stv6110x_config, cfg->i2c_adap);
		if (ctl)
		{
			printk("%s: TUNER attached\n", __FUNCTION__);
			vitamin_hd5000_stv090x_config.tuner_init = ctl->tuner_init;
			vitamin_hd5000_stv090x_config.tuner_set_mode = ctl->tuner_set_mode;
			vitamin_hd5000_stv090x_config.tuner_set_frequency = ctl->tuner_set_frequency;
			vitamin_hd5000_stv090x_config.tuner_get_frequency = ctl->tuner_get_frequency;
			vitamin_hd5000_stv090x_config.tuner_set_bandwidth = ctl->tuner_set_bandwidth;
			vitamin_hd5000_stv090x_config.tuner_get_bandwidth = ctl->tuner_get_bandwidth;
			vitamin_hd5000_stv090x_config.tuner_set_bbgain = ctl->tuner_set_bbgain;
			vitamin_hd5000_stv090x_config.tuner_get_bbgain = ctl->tuner_get_bbgain;
			vitamin_hd5000_stv090x_config.tuner_set_refclk = ctl->tuner_set_refclk;
			vitamin_hd5000_stv090x_config.tuner_get_status = ctl->tuner_get_status;
		}
		else
		{
			printk(KERN_INFO "%s: error attaching TUNER\n", __FUNCTION__);
			goto error_out;
		}
	}
	else
	{
		printk(KERN_INFO "%s: error attaching\n", __FUNCTION__);
		goto error_out;
	}
	return frontend;
error_out:
	printk("core: Frontend registration failed!\n");
	if (frontend)
		dvb_frontend_detach(frontend);
	return NULL;
}

static struct dvb_frontend *init_stv090x_device(struct dvb_adapter *adapter, struct plat_tuner_config *tuner_cfg, int i)
{
	struct dvb_frontend *frontend;
	struct core_config *cfg;
	unsigned int reg = 0;
	printk("> (bus = %d) %s\n", tuner_cfg->i2c_bus, __FUNCTION__);
	cfg = kmalloc(sizeof(struct core_config), GFP_KERNEL);
	if (cfg == NULL)
	{
		printk("stv090x: kmalloc failed\n");
		return NULL;
	}
	/* initialize the config data */
	cfg->tuner_enable_pin = NULL;
	cfg->i2c_adap = i2c_get_adapter(tuner_cfg->i2c_bus);
	printk("i2c adapter = 0x%0x\n", cfg->i2c_adap);
	cfg->i2c_addr = tuner_cfg->i2c_addr;
	printk("i2c addr = %02x\n", cfg->i2c_addr);
	if (cfg->i2c_adap == NULL)
	{
		printk("[stv090x] failed to allocate resources (i2c adapter)\n");
		goto error;
	}
	/* SYS_CFG25[0] - FE900_SOFT_RESET */
	ctrl_outl(1, 0xfe001164); /* reset ON */
	msleep(250);
	ctrl_outl(0, 0xfe001164); /* reset OFF */
	msleep(250);
	frontend = frontend_init(cfg, i);
	if (frontend == NULL)
	{
		printk("[stv090x] frontend init failed!\n");
		goto error;
	}
	printk(KERN_INFO "%s: Call dvb_register_frontend (adapter = 0x%x)\n",
		   __FUNCTION__, (unsigned int) adapter);
	if (dvb_register_frontend(adapter, frontend))
	{
		printk("[stv090x] frontend registration failed !\n");
		if (frontend->ops.release)
			frontend->ops.release(frontend);
		goto error;
	}
	return frontend;
error:
	kfree(cfg);
	return NULL;
}

struct plat_tuner_config tuner_resources[] =
{
	[0] = {
		.adapter = 0,
		.i2c_bus = 3,
		.i2c_addr = 0x68,
	},
};

void stv090x_register_frontend(struct dvb_adapter *dvb_adap)
{
	int i = 0;
	int vLoop = 0;
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
				init_stv090x_device(core[i]->dvb_adapter, &tuner_resources[vLoop], vLoop);
		}
	}
	printk(KERN_INFO "%s: <\n", __FUNCTION__);
	return;
}

EXPORT_SYMBOL(stv090x_register_frontend);

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

module_param(bbgain, int, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
MODULE_PARM_DESC(bbgain, "default=-1 (use default config = 10");

MODULE_DESCRIPTION("STi7111+STV6110 DVB-S2 Frontend Driver");
MODULE_AUTHOR("ADPT");
MODULE_LICENSE("GPL");

