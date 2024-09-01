#include "core.h"
#include "stv6110x.h"
#include "stv090x.h"
#include "ix7306.h"
#include "zl10353.h"
#include "../base/sharp6465.h"
#include "../base/tda1002x.h"
#include "../base/lg031.h"

#include <linux/platform_device.h>
#include <asm/system.h>
#include <asm/io.h>
#include <linux/dvb/dmx.h>
#include <linux/proc_fs.h>
#include <linux/version.h>
#include <pvr_config.h>

#define I2C_ADDR_STV090X (0xd0 >> 1)
#define I2C_ADDR_STV6110X (0xc0 >> 1)
#define I2C_ADDR_IX7306 (0xc0 >> 1)
#define I2C_ADDR_CE6353 (0x1e >> 1)
#define I2C_ADDR_SHARP6465 (0xc2 >> 1)
#define I2C_ADDR_TDA10023 (0x18 >> 1)
#define I2C_ADDR_LG031 (0xc6 >> 1)

/* On this framework, We can't support dual tuner,
 * so we can only use one. In order to support dual tuner,
 * we need add tuner type option to stx090x_config,
 * when this done, just remove the define TUNER_IX7306 in stv090x.h*/

#if defined(TUNER_IX7306)
#define CLK_EXT_IX7306 4000000
#define CLK_EXT_STV6110 16000000
#elif defined(TUNER_STB6110)
#define CLK_EXT_IX7306 8000000
#define CLK_EXT_STV6110 16000000
#else
#error "You must define tuner type..."
#endif

static char *demod1 = "stv090x";
static char *demod2 = "ce6353";
static char *tuner1 = "sharp7306";
static char *tuner2 = "sharp6465";

static int demodType1;
static int demodType2;
static int tunerType1;
static int tunerType2;

static struct core *core[MAX_DVB_ADAPTERS];

module_param(demod1, charp, 0);
module_param(demod2, charp, 0);
MODULE_PARM_DESC(demod1, "demodelator1 stv090x, ce6353, tda10023(default stv090x");// DVB-S2
MODULE_PARM_DESC(demod1, "demodelator2 stv090x, ce6353, tda10023(default ce6353"); // DVB-T

module_param(tuner1, charp, 0);
module_param(tuner2, charp, 0);
MODULE_PARM_DESC(tuner1, "tuner1 sharp7306, stv6110x, sharp6465, lg031(default sharp7306"); // DVB-S2
MODULE_PARM_DESC(tuner2, "tuner2 sharp7306, stv6110x, sharp6465, lg031(default sharp6465"); // DVB-T

enum
{
	STV090X,
	CE6353,
	TDA10023,
};

static char *demod_name[] =
{
	"stv090x",
	"ce6353",
	"tda10023",
};

enum
{
	SHARP7306,
	STV6110X,
	SHARP6465,
	LG031,
};

static char *tuner_name[] =
{
	"sharp7306",
	"stv6110x",
	"sharp6465",
	"lg031",
};

enum fe_ctl
{
	FE1_1318 = 0,
	FE1_RST = 1,
	FE1_LNB_EN = 2,
	FE1_1419 = 3,
	FE0_RST = 4,
	FE0_LNB_EN = 5,
	FE0_1419 = 6,
	FE0_1318 = 7,
};

static unsigned char fctl = 0;

struct stpio_pin *srclk; // shift clock
struct stpio_pin *rclk; // latch clock
struct stpio_pin *sda; // serial data

#define SRCLK_CLR() {stpio_set_pin(srclk, 0);}
#define SRCLK_SET() {stpio_set_pin(srclk, 1);}

#define RCLK_CLR() {stpio_set_pin(rclk, 0);}
#define RCLK_SET() {stpio_set_pin(rclk, 1);}

#define SDA_CLR() {stpio_set_pin(sda, 0);}
#define SDA_SET() {stpio_set_pin(sda, 1);}

void hc595_out(unsigned char ctls, int state)
{
	int i;
	if (state)
		fctl |= 1 << ctls;
	else
		fctl &= ~(1 << ctls);
	/*
	 * clear everything out just in case to
	 * prepare shift register for bit shifting
	 */
	SDA_CLR();
	SRCLK_CLR();
	udelay(10);
	for (i = 7; i >= 0; i--)
	{
		SRCLK_CLR();
		if (fctl & (1 << i))
		{
			SDA_SET();
		}
		else
		{
			SDA_CLR();
		}
		udelay(1);
		SRCLK_SET();
		udelay(1);
	}
	RCLK_CLR();
	udelay(1);
	RCLK_SET();
}

EXPORT_SYMBOL(hc595_out);

static struct stv090x_config stv090x_config =
{
	.device = STV0903,
	.demod_mode = STV090x_DUAL/*STV090x_SINGLE*/,
	.clk_mode = STV090x_CLK_EXT,

	.xtal = CLK_EXT_IX7306,
	.address = I2C_ADDR_STV090X,

	.ts1_mode = STV090x_TSMODE_DVBCI/*STV090x_TSMODE_PARALLEL_PUNCTURED*//*STV090x_TSMODE_SERIAL_CONTINUOUS*/,
	.ts2_mode = STV090x_TSMODE_DVBCI/*STV090x_TSMODE_PARALLEL_PUNCTURED*//*STV090x_TSMODE_SERIAL_CONTINUOUS*/,
	.ts1_clk = 0,
	.ts2_clk = 0,

	.fe_rst = 0,
	.fe_lnb_en = 0,
	.fe_1419 = 0,
	.fe_1318 = 0,

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
	.addr = I2C_ADDR_STV6110X,
	.refclk = CLK_EXT_STV6110,
	.clk_div = 2,
};

static const struct ix7306_config bs2s7hz7306a_config =
{
	.name = "Sharp BS2S7HZ7306A",
	.addr = I2C_ADDR_IX7306,
	.step_size = IX7306_STEP_1000,
	.bb_lpf = IX7306_LPF_12,
	.bb_gain = IX7306_GAIN_2dB,
};

static struct zl10353_config ce6353_config =
{
	.demod_address = I2C_ADDR_CE6353,
	.no_tuner = 1,
	.parallel_ts = 1,
};

static const struct sharp6465_config s6465_config =
{
	.name = "Sharp 6465",
	.addr = I2C_ADDR_SHARP6465,
	.bandwidth = BANDWIDTH_8_MHZ,

	.Frequency = 500000,
	.IF = 36167,
	.TunerStep = 16667,
};

static struct tda10023_config philips_tda10023_config =
{
	.demod_address = I2C_ADDR_TDA10023,
	.invert = 1,
};

static struct lg031_config lg_lg031_config =
{
	.addr = I2C_ADDR_LG031,
};

static struct dvb_frontend *frontend_get_by_type(struct core_config *cfg,
												 int iDemodType,
												 int iTunerType)
{
	struct stv6110x_devctl *ctl = NULL;
	struct dvb_frontend *frontend = NULL;
	switch (iDemodType)
	{
		case STV090X:
		{
			frontend = dvb_attach(stv090x_attach, &stv090x_config,
								  cfg->i2c_adap, STV090x_DEMODULATOR_0);
			if (frontend)
			{
				printk("%s: stv090x attached\n", __FUNCTION__);
				stv090x_config.fe_rst = cfg->tuner->fe_rst;
				stv090x_config.fe_1318 = cfg->tuner->fe_1318;
				stv090x_config.fe_1419 = cfg->tuner->fe_1419;
				stv090x_config.fe_lnb_en = cfg->tuner->fe_lnb_en;
				switch (iTunerType)
				{
					case SHARP7306:
					{
						if (dvb_attach(ix7306_attach, frontend, &bs2s7hz7306a_config, cfg->i2c_adap))
						{
							printk("%s: IX7306 attached\n", __FUNCTION__);
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
					}
					case STV6110X:
					default:
					{
						ctl = dvb_attach(stv6110x_attach, frontend, &stv6110x_config, cfg->i2c_adap);
						if (ctl)
						{
							printk("%s: stv6110x attached\n", __FUNCTION__);
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
			}
			else
			{
				printk(KERN_INFO "%s: error attaching stv090x\n", __FUNCTION__);
				goto error_out;
			}
			break;
		}
		case CE6353:
		{
			frontend = dvb_attach(zl10353_attach, &ce6353_config, cfg->i2c_adap);
			if (frontend)
			{
				printk("%s: ce6353 attached\n", __FUNCTION__);
				switch (iTunerType)
				{
					case SHARP6465:
					{
						if (dvb_attach(sharp6465_attach, frontend, &s6465_config, cfg->i2c_adap))
						{
							printk("%s: SHARP6465 attached\n", __FUNCTION__);
						}
						else
						{
							printk(KERN_INFO "%s: error attaching SHARP6465\n", __FUNCTION__);
							goto error_out;
						}
						break;
					}
					default:
					{
						printk(KERN_INFO "%s: error unknown tuner\n", __FUNCTION__);
						goto error_out;
					}
				}
			}
			else
			{
				printk(KERN_INFO "%s: error attaching ce6353\n", __FUNCTION__);
				goto error_out;
			}
			break;
		}
		case TDA10023:
		{
			frontend = dvb_attach(tda10023_attach, &philips_tda10023_config,
								  cfg->i2c_adap, 0x48);
			if (frontend)
			{
				printk("%s: tda10023 attached\n", __FUNCTION__);
				switch (iTunerType)
				{
					case LG031:
					{
						if (dvb_attach(lg031_attach, frontend, &lg_lg031_config, cfg->i2c_adap))
						{
							printk("%s: lg031 attached\n", __FUNCTION__);
						}
						else
						{
							printk(KERN_INFO "%s: error attaching lg031\n", __FUNCTION__);
							goto error_out;
						}
						break;
					}
					default:
					{
						printk(KERN_INFO "%s: error unknown tuner\n", __FUNCTION__);
						goto error_out;
					}
				}
			}
			else
			{
				printk(KERN_INFO "%s: error attaching tda10023\n", __FUNCTION__);
				goto error_out;
			}
			break;
		}
		default:
		{
			printk(KERN_INFO "%s: error unknown demod\n", __FUNCTION__);
			goto error_out;
		}
	}
	return frontend;
error_out:
	printk("core: Frontend registration failed!\n");
	if (frontend)
		dvb_frontend_detach(frontend);
	return NULL;
}

static struct dvb_frontend *frontend_init(struct core_config *cfg, int i)
{
	struct dvb_frontend *frontend = NULL;
	printk(KERN_INFO "%s >\n", __FUNCTION__);
	if (i == 0)
	{
		frontend = frontend_get_by_type(cfg, demodType1, tunerType1);
	}
	else
	{
		frontend = frontend_get_by_type(cfg, demodType2, tunerType2);
	}
	return frontend;
}

static struct dvb_frontend *init_fe_device(struct dvb_adapter *adapter,
										   struct tuner_config *tuner_cfg, int i)
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
	cfg->tuner = tuner_cfg;
	cfg->i2c_adap = i2c_get_adapter(tuner_cfg->i2c_bus);
	printk("i2c adapter = 0x%0x\n", (int)cfg->i2c_adap);
	if (cfg->i2c_adap == NULL)
	{
		printk("stv090x: failed to allocate i2c resources\n");
		kfree(cfg);
		return NULL;
	}
	/* set to low */
	hc595_out(tuner_cfg->fe_rst, 0);
	/* Wait for everything to die */
	msleep(50);
	/* Pull it up out of Reset state */
	hc595_out(tuner_cfg->fe_rst, 1);
	/* Wait for PLL to stabilize */
	msleep(250);
	/*
	 * PLL state should be stable now. Ideally, we should check
	 * for PLL LOCK status. But well, never mind!
	 */
	frontend = frontend_init(cfg, i);
	if (frontend == NULL)
	{
		printk("No frontend found !\n");
		return NULL;
	}
	printk(KERN_INFO "%s: Call dvb_register_frontend (adapter = 0x%x)\n",
		   __FUNCTION__, (unsigned int) adapter);
	frontend->id = i;
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

struct tuner_config tuner_resources[] =
{

	[0] = {
		.adapter = 0,
		.i2c_bus = 0,
		.fe_rst = FE0_RST,
		.fe_1318 = FE0_1318,
		.fe_1419 = FE0_1419,
		.fe_lnb_en = FE0_LNB_EN,
	},
	[1] = {
		.adapter = 0,
		.i2c_bus = 1,
		.fe_rst = FE1_RST,
		.fe_1318 = FE1_1318,
		.fe_1419 = FE1_1419,
		.fe_lnb_en = FE1_LNB_EN,
	},
};

void fe_core_register_frontend(struct dvb_adapter *dvb_adap)
{
	int i = 0;
	int vLoop = 0;
	printk(KERN_INFO "%s: Spider-Team plug and play frontend core\n", __FUNCTION__);
	core[i] = (struct core *) kmalloc(sizeof(struct core), GFP_KERNEL);
	if (!core[i])
		return;
	memset(core[i], 0, sizeof(struct core));
	core[i]->dvb_adapter = dvb_adap;
	dvb_adap->priv = core[i];
	printk("tuner = %d\n", ARRAY_SIZE(tuner_resources));
	srclk = stpio_request_pin(2, 2, "HC595_SRCLK", STPIO_OUT);
	rclk = stpio_request_pin(2, 3, "HC595_RCLK", STPIO_OUT);
	sda = stpio_request_pin(2, 4, "HC595_SDA", STPIO_OUT);
	if ((srclk == NULL) || (rclk == NULL) || (sda == NULL))
	{
		printk("stv090x: failed to allocate IO resources\n");
		if (srclk != NULL)
			stpio_free_pin(srclk);
		if (rclk != NULL)
			stpio_free_pin(rclk);
		if (sda != NULL)
			stpio_free_pin(sda);
		return;
	}
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

static int fe_get_demod_type(char *pcDemod)
{
	int iDemodType = STV090X;
	if ((pcDemod[0] == 0) || (strcmp("stv090x", pcDemod) == 0))
	{
		iDemodType = STV090X;
	}
	else if (strcmp("ce6353", pcDemod) == 0)
	{
		iDemodType = CE6353;
	}
	else if (strcmp("tda10023", pcDemod) == 0)
	{
		iDemodType = TDA10023;
	}
	else
	{
		iDemodType = STV090X;
	}
	return iDemodType;
}

static int fe_get_tuner_type(char *pcTuner)
{
	int iTunerType = SHARP7306;
	if ((pcTuner[0] == 0) || (strcmp("sharp7306", pcTuner) == 0))
	{
		iTunerType = SHARP7306;
	}
	else if (strcmp("stv6110x", pcTuner) == 0)
	{
		iTunerType = STV6110X;
	}
	else if (strcmp("sharp6465", pcTuner) == 0)
	{
		iTunerType = SHARP6465;
	}
	else if (strcmp("lg031", pcTuner) == 0)
	{
		iTunerType = LG031;
	}
	else
	{
		iTunerType = SHARP7306;
	}
	return iTunerType;
}

static void printk_demod_name_and_type(int iDemodType)
{
	switch (iDemodType)
	{
		case STV090X:
			printk("stv090x dvb-s2 ");
			break;
		case CE6353:
			printk("ce6353 dvb-t ");
			break;
		case TDA10023:
			printk("tda10023 dvb-c ");
			break;
	}
}

EXPORT_SYMBOL(fe_core_register_frontend);

int __init fe_core_init(void)
{
	/****** FRONT 0 ********/
	demodType1 = fe_get_demod_type(demod1);
	tunerType1 = fe_get_tuner_type(tuner1);
	printk("demodelator1: ");
	printk_demod_name_and_type(demodType1);
	printk("tuner1: %s\n", tuner_name[tunerType1]);
	/****** FRONT 1 ********/
	demodType2 = fe_get_demod_type(demod2);
	tunerType2 = fe_get_tuner_type(tuner2);
	printk("demodelator2: ");
	printk_demod_name_and_type(demodType2);
	printk("tuner2: %s\n", tuner_name[tunerType2]);
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
MODULE_AUTHOR("Spider-Team");
MODULE_LICENSE("GPL");
