#include "core.h"
/* Demodulators */
#include "stv090x.h"

/* Tuners */
#include "stb6100.h"
#include "stb6100_cfg.h"

#include <linux/platform_device.h>
#include <asm/system.h>
#include <asm/io.h>
#include <linux/dvb/dmx.h>
#include <linux/proc_fs.h>
#include <pvr_config.h>

#define I2C_ADDR_STB6100_1 (0x60)
#define I2C_ADDR_STB6100_2 (0x63)
#define I2C_ADDR_STV090X (0x68)

static struct core *core[MAX_DVB_ADAPTERS];

static struct stv090x_config stv090x_config_1 =
{
	.device = STV0900,
	.demod_mode = STV090x_DUAL/*STV090x_SINGLE*/,
	.clk_mode = STV090x_CLK_EXT,

	.xtal = 27000000,
	.address = I2C_ADDR_STV090X,

	.ts1_mode = STV090x_TSMODE_SERIAL_CONTINUOUS,
	.ts2_mode = STV090x_TSMODE_SERIAL_CONTINUOUS,
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

static struct stv090x_config stv090x_config_2 =
{
	.device = STV0900,
	.demod_mode = STV090x_DUAL,
	.clk_mode = STV090x_CLK_EXT,

	.xtal = 27000000,
	.address = I2C_ADDR_STV090X,

	.ts1_mode = STV090x_TSMODE_SERIAL_CONTINUOUS,
	.ts2_mode = STV090x_TSMODE_SERIAL_CONTINUOUS,
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

static struct stb6100_config stb6100_config_1 =
{
	.tuner_address = I2C_ADDR_STB6100_1,
	.refclock = 27000000
};

static struct stb6100_config stb6100_config_2 =
{
	.tuner_address = I2C_ADDR_STB6100_2,
	.refclock = 27000000
};

static struct dvb_frontend *frontend_init(struct core_config *cfg, int i)
{
	struct dvb_frontend *frontend = NULL;
	printk(KERN_INFO "%s frontend_init >\n", __FUNCTION__);
	if (i == 0)
		frontend = dvb_attach(stv090x_attach, &stv090x_config_1, cfg->i2c_adap, STV090x_DEMODULATOR_0);
	else
		frontend = dvb_attach(stv090x_attach, &stv090x_config_2, cfg->i2c_adap, STV090x_DEMODULATOR_1);
	if (frontend)
	{
		printk("%s: stv090x attached\n", __FUNCTION__);
		if (i == 0)
		{
			if (dvb_attach(stb6100_attach, frontend, &stb6100_config_1, cfg->i2c_adap) == 0)
			{
				printk(KERN_INFO "error attaching stb6100_1\n");
				goto error_out;
			}
			else
			{
				printk("fe_core : stb6100_1 attached\n");
				stv090x_config_1.tuner_get_frequency = stb6100_get_frequency;
				stv090x_config_1.tuner_set_frequency = stb6100_set_frequency;
				stv090x_config_1.tuner_set_bandwidth = stb6100_set_bandwidth;
				stv090x_config_1.tuner_get_bandwidth = stb6100_get_bandwidth;
				stv090x_config_1.tuner_get_status = frontend->ops.tuner_ops.get_status;
			}
		}
		else
		{
			if (dvb_attach(stb6100_attach, frontend, &stb6100_config_2, cfg->i2c_adap) == 0)
			{
				printk(KERN_INFO "error attaching stb6100_2\n");
				goto error_out;
			}
			else
			{
				printk("fe_core : stb6100_2 attached\n");
				stv090x_config_2.tuner_get_frequency = stb6100_get_frequency;
				stv090x_config_2.tuner_set_frequency = stb6100_set_frequency;
				stv090x_config_2.tuner_set_bandwidth = stb6100_set_bandwidth;
				stv090x_config_2.tuner_get_bandwidth = stb6100_get_bandwidth;
				stv090x_config_2.tuner_get_status = frontend->ops.tuner_ops.get_status;
			}
		}
	}
	else
	{
		printk(KERN_INFO "%s: error attaching stv090x\n", __FUNCTION__);
		goto error_out;
	}
	return frontend;
error_out:
	printk("core: Frontend registration failed!\n");
	if (frontend)
		dvb_frontend_detach(frontend);
	return NULL;
}

static struct dvb_frontend *
init_fe_device(struct dvb_adapter *adapter,
			   struct plat_tuner_config *tuner_cfg, int i)
{
	struct fe_core_state *state;
	struct dvb_frontend *frontend;
	struct core_config *cfg;
	printk("> (bus = %d) %s\n", tuner_cfg->i2c_bus, __FUNCTION__);
	cfg = kmalloc(sizeof(struct core_config), GFP_KERNEL);
	if (cfg == NULL)
	{
		printk("fe-core: kmalloc failed\n");
		return NULL;
	}
	/* initialize the config data */
	cfg->i2c_adap = i2c_get_adapter(tuner_cfg->i2c_bus);
	printk("i2c adapter = 0x%0x\n", cfg->i2c_adap);
	cfg->i2c_addr = tuner_cfg->i2c_addr;
	if (cfg->i2c_adap == NULL)
	{
		printk("fe-core: failed to allocate resources (%s)\n",
			   (cfg->i2c_adap == NULL) ? "i2c" : "STPIO error");
		kfree(cfg);
		return NULL;
	}
	frontend = frontend_init(cfg, i);
	if (frontend == NULL)
	{
		printk("No frontend found !\n");
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

struct plat_tuner_config tuner_resources[] =
{

	[0] = {
		.adapter = 0,
		.i2c_bus = 0,
	},
	[1] = {
		.adapter = 0,
		.i2c_bus = 0,
	},
};

void fe_core_register_frontend(struct dvb_adapter *dvb_adap)
{
	int i = 0;
	int vLoop = 0;
	printk(KERN_INFO "%s: Adb_Box frontend core\n", __FUNCTION__);
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

MODULE_DESCRIPTION("Tunerdriver");
MODULE_AUTHOR("Team Ducktales mod B4Team & freebox");
MODULE_LICENSE("GPL");
