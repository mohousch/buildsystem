#include "core.h"
/* Demodulators */
#include "stv0297.h"

/* Tuners */
#include "mt2060.h"

#include <linux/platform_device.h>
#include <asm/system.h>
#include <asm/io.h>
#include <linux/dvb/dmx.h>
#include <linux/proc_fs.h>
#include <pvr_config.h>

#define I2C_ADDR_STV0297 (0x38 >> 1)
#define I2C_ADDR_MT2060 (0xc0 >> 1)

static struct core *core[MAX_DVB_ADAPTERS];

static struct stv0297_config stv0297_config;
static struct mt2060_config mt2060_config;

static u8 stv0297_inittab[] = //init MMP-5800c
{
	0x80, 0x01,
	0x80, 0x00,
	0x81, 0x01,
	0x81, 0x00,
	0x84, 0x01,
	0x84, 0x00,
	0x83, 0x10,
	0x83, 0x00,
	0x02, 0x00,
	0x30, 0xB3,
	0x31, 0x65,
	0x32, 0xB3,
	0x33, 0x00,
	0x34, 0x29,
	0x35, 0x78,
	0x36, 0x80,
	0x40, 0x1A,
	0x41, 0x90,
	0x42, 0x3E,
	0x43, 0x00,
	0x44, 0xFF,
	0x49, 0x04,
	0x52, 0x30,
	0x53, 0x08,
	0x59, 0x08,
	0x5A, 0x3E,
	0x5B, 0x04,
	0x37, 0x20,
	0x61, 0x49,
	0x62, 0x0A,
	0x6A, 0x02,
	0x70, 0xFF,
	0x71, 0x04,
	0x85, 0x80,
	0x86, 0x40,
	0x87, 0x7A,
	0x88, 0x00,
	0x89, 0x00,
	0x90, 0x01,
	0xA0, 0x82,
	0xB0, 0x91,
	0xB1, 0x0B,
	0xC0, 0x4B,
	0xC1, 0x10,
	0xC2, 0x12,
	0xDE, 0x00,
	0xDF, 0x00,
	0xDF, 0x01,
	0x31, 0x00,
	0x41, 0x00,
	0x42, 0x3C,
	0x43, 0x40,
	0xff, 0xff,
};

static struct stv0297_config stv0297_config =
{
	.demod_address = I2C_ADDR_STV0297,
	.inittab = stv0297_inittab,
	.invert = 0,
	.stop_during_read = 1,
};

static struct mt2060_config mt2060_config =
{
	.i2c_address = I2C_ADDR_MT2060,
	.clock_out = 0,
};

static struct dvb_frontend *frontend_init(struct core_config *cfg, int i)
{
	struct dvb_frontend *frontend = NULL;
	printk(KERN_INFO "%s frontend_init >\n", __FUNCTION__);
	if (i > 0)
		return NULL;
	frontend = dvb_attach(stv0297_attach, &stv0297_config, cfg->i2c_adap);
	if (frontend)
	{
		printk("fe_core : stv0297 attached OK \n");
		if (dvb_attach(mt2060_attach, frontend, &mt2060_config, cfg->i2c_adap, 1220) == 0)
		{
			printk(KERN_INFO "error attaching mt2060\n");
			goto error_out;
		}
		printk("fe_core : mt2060 attached OK \n");
	}
	else
	{
		printk(KERN_INFO "%s: error attaching stv0297\n", __FUNCTION__);
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
};

void fe_core_register_frontend(struct dvb_adapter *dvb_adap)
{
	int i = 0;
	int vLoop = 0;
	printk(KERN_INFO "%s: frontend core\n", __FUNCTION__);
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
	printk("fe_core_init\n");
	return 0;
}

static void __exit fe_core_exit(void)
{
	printk("frontend core unloaded\n");
}

module_init(fe_core_init);
module_exit(fe_core_exit);

MODULE_DESCRIPTION("STV0297_MT2060_Frontend_core");
MODULE_AUTHOR("-=Mario=-");
MODULE_LICENSE("GPL");
