#include "core.h"
#include "stv090x.h"
#include "stv6110x.h"

#include <linux/platform_device.h>
#include <asm/system.h>
#include <asm/io.h>
#include <linux/dvb/dmx.h>
#include <linux/proc_fs.h>
#include <pvr_config.h>
#include <linux/gpio.h>

#include "tda10048.h"
#include "tda18218.h"

//#define I2C_ADDR_STB6100_1 	(0x61)
//#define I2C_ADDR_STB6100_2 	(0x62)
#define I2C_ADDR_STV090X	(0x6A)

#define RESET stm_gpio(6, 7)

//#define DVBT

static struct core *core[MAX_DVB_ADAPTERS];

static struct stv090x_config stv090x_config = {
	.device			= STV0900,
	.demod_mode		= STV090x_DUAL,
	.clk_mode		= STV090x_CLK_EXT,
	

	.xtal			= 15000000,
	.address		= I2C_ADDR_STV090X,

	.ts1_mode		= STV090x_TSMODE_SERIAL_CONTINUOUS,
	.ts2_mode		= STV090x_TSMODE_SERIAL_CONTINUOUS,
	.ts1_clk		= 0,
	.ts2_clk		= 0,

	.lnb_enable 	= NULL,
	.lnb_vsel	 	= NULL,

	.repeater_level	= STV090x_RPTLEVEL_16,

	.tuner_init				= NULL,
	.tuner_set_mode			= NULL,
	.tuner_set_frequency	= NULL,
	.tuner_get_frequency	= NULL,
	.tuner_set_bandwidth	= NULL,
	.tuner_get_bandwidth	= NULL,
	.tuner_set_bbgain		= NULL,
	.tuner_get_bbgain		= NULL,
	.tuner_set_refclk		= NULL,
	.tuner_get_status		= NULL,
};

static struct stv6110x_config stv6110x_config1 = {
	.addr			= 0x60,
	.refclk			= 16000000,
};

static struct stv6110x_config stv6110x_config2 = {
	.addr			= 0x61,
	.refclk			= 16000000,
};

#ifdef DVBT

static struct tda10048_config tda10048_config_ = {
	.demod_address    = 0x10 >> 1,
	.output_mode      = TDA10048_SERIAL_OUTPUT,
	.fwbulkwritelen   = TDA10048_BULKWRITE_200,
	.inversion        = TDA10048_INVERSION_ON,
	.dtv6_if_freq_khz = TDA10048_IF_3300,
	.dtv7_if_freq_khz = TDA10048_IF_3500,
	.dtv8_if_freq_khz = TDA10048_IF_4000,
	.clk_freq_khz     = TDA10048_CLK_16000,
};
static struct tda18218_config tda18218_config_= {
.i2c_address = 0xc0 >> 1,	//0xc1
.i2c_wr_max = 21, /* max wr bytes AF9015 I2C adap can handle at once */
};


#endif




static struct dvb_frontend * frontend_init(struct core_config *cfg, int i)
{
	struct dvb_frontend *frontend = NULL;
	struct tuner_devctl *ctl = NULL;

	printk (KERN_INFO "%s >>>\n", __FUNCTION__);
/*
		if (i==0)
			frontend = dvb_attach(stv090x_attach, &stv090x_config_1,cfg->i2c_adap, STV090x_DEMODULATOR_0);
		else
			frontend = dvb_attach(stv090x_attach, &stv090x_config_2,cfg->i2c_adap, STV090x_DEMODULATOR_1);

		if (frontend) {
			printk("%s: stv090x attached\n", __FUNCTION__);

			if (i==0)
			{
				if (dvb_attach(stb6100_attach, frontend, &stb6100_config_1, cfg->i2c_adap) == 0) {
					printk (KERN_INFO "error attaching stb6100_1\n");
					goto error_out;
				}
				else
				{
					printk("fe_core : stb6100_1 attached\n");
					
					stv090x_config_1.tuner_get_frequency	= stb6100_get_frequency;
					stv090x_config_1.tuner_set_frequency	= stb6100_set_frequency;
					stv090x_config_1.tuner_set_bandwidth	= stb6100_set_bandwidth;
					stv090x_config_1.tuner_get_bandwidth	= stb6100_get_bandwidth;
					stv090x_config_1.tuner_get_status	= frontend->ops.tuner_ops.get_status;
					
				}
			}
			else
			{
				if (dvb_attach(stb6100_attach, frontend, &stb6100_config_2, cfg->i2c_adap) == 0) {
					printk (KERN_INFO "error attaching stb6100_2\n");
					goto error_out;
				}
				else
				{
					printk("fe_core : stb6100_2 attached\n");
					
					stv090x_config_2.tuner_get_frequency	= stb6100_get_frequency;
					stv090x_config_2.tuner_set_frequency	= stb6100_set_frequency;
					stv090x_config_2.tuner_set_bandwidth	= stb6100_set_bandwidth;
					stv090x_config_2.tuner_get_bandwidth	= stb6100_get_bandwidth;
					stv090x_config_2.tuner_get_status	= frontend->ops.tuner_ops.get_status;
					
				}
			}


		} else {
			printk (KERN_INFO "%s: error attaching stv090x\n", __FUNCTION__);
			goto error_out;
		}
*/
	if(i==0)
	{
	frontend = stv090x_attach(&stv090x_config, cfg->i2c_adap, STV090x_DEMODULATOR_0);
	if (frontend) {
		printk("%s: attached STV090x_DEMODULATOR_0\n", __FUNCTION__);
		
		ctl = dvb_attach(stv6110x_attach, frontend, &stv6110x_config1, cfg->i2c_adap);
		if(ctl)	{
			printk("%s: attached stv6110x\n", __FUNCTION__);
			stv090x_config.tuner_init	  	  = ctl->tuner_init;
			stv090x_config.tuner_set_mode	  = ctl->tuner_set_mode;
			stv090x_config.tuner_set_frequency = ctl->tuner_set_frequency;
			stv090x_config.tuner_get_frequency = ctl->tuner_get_frequency;
			stv090x_config.tuner_set_bandwidth = ctl->tuner_set_bandwidth;
			stv090x_config.tuner_get_bandwidth = ctl->tuner_get_bandwidth;
			stv090x_config.tuner_set_bbgain	  = ctl->tuner_set_bbgain;
			stv090x_config.tuner_get_bbgain	  = ctl->tuner_get_bbgain;
			stv090x_config.tuner_set_refclk	  = ctl->tuner_set_refclk;
			stv090x_config.tuner_get_status	  = ctl->tuner_get_status;
		} else {
			printk (KERN_INFO " error attaching stv6110x %s\n", __FUNCTION__);
			goto error_out;
		}
	} else {
		printk (KERN_INFO "%s: error attaching STV090x_DEMODULATOR_0\n", __FUNCTION__);
		goto error_out;
	}
	}
	if(i==1)
	{
	frontend = stv090x_attach(&stv090x_config, cfg->i2c_adap, STV090x_DEMODULATOR_1);
	if (frontend) {
		printk("%s: attached STV090x_DEMODULATOR_1\n", __FUNCTION__);
		
		ctl = dvb_attach(stv6110x_attach, frontend, &stv6110x_config2, cfg->i2c_adap);
		if(ctl)	{
			printk("%s: attached stv6110x\n", __FUNCTION__);
			stv090x_config.tuner_init	  	  = ctl->tuner_init;
			stv090x_config.tuner_set_mode	  = ctl->tuner_set_mode;
			stv090x_config.tuner_set_frequency = ctl->tuner_set_frequency;
			stv090x_config.tuner_get_frequency = ctl->tuner_get_frequency;
			stv090x_config.tuner_set_bandwidth = ctl->tuner_set_bandwidth;
			stv090x_config.tuner_get_bandwidth = ctl->tuner_get_bandwidth;
			stv090x_config.tuner_set_bbgain	  = ctl->tuner_set_bbgain;
			stv090x_config.tuner_get_bbgain	  = ctl->tuner_get_bbgain;
			stv090x_config.tuner_set_refclk	  = ctl->tuner_set_refclk;
			stv090x_config.tuner_get_status	  = ctl->tuner_get_status;
		} else {
			printk (KERN_INFO "error attaching stv6110x %s\n", __FUNCTION__);
			goto error_out;
		}
	} else {
		printk (KERN_INFO "%s: error attaching STV090x_DEMODULATOR_1\n", __FUNCTION__);
		goto error_out;
	}
	}

#ifdef DVBT
	if((i==2))
	{
			printk("DVBT TDA10048\n");
			frontend = dvb_attach(tda10048_attach, &tda10048_config_,	cfg->i2c_adap);
			if (frontend) {
				printk("%s: demod attached tda18218\n", __FUNCTION__);
				if(dvb_attach(tda18218_attach, frontend, cfg->i2c_adap, &tda18218_config_)== NULL)
				{
					printk ("%s: error attaching tda18218\n", __FUNCTION__);
					goto error_out;
				} else {
					printk("%s: tda18218 attached\n", __FUNCTION__);
				}
			}
			else {
				printk ("%s: error attaching TDA10048\n", __FUNCTION__);
				goto error_out;
				}
	}
#endif


	return frontend;

error_out:
	printk("core: Frontend registration failed!\n");
	if (frontend)
		dvb_frontend_detach(frontend);
	return NULL;
}

static struct dvb_frontend * 
init_fe_device (struct dvb_adapter *adapter, struct plat_tuner_config *tuner_cfg, int i)
{
  struct fe_core_state *state;
  struct dvb_frontend *frontend;
  struct core_config *cfg;

  printk ("%s> (bus = %d)\n", __FUNCTION__, tuner_cfg->i2c_bus);

  cfg = kmalloc (sizeof (struct core_config), GFP_KERNEL);
  if (cfg == NULL)
  {
    printk ("fe-core: kmalloc failed\n");
    return NULL;
  } else printk ("fe-core: kmalloc succeded\n");

  /* initialize the config data */
  cfg->i2c_adap = i2c_get_adapter (tuner_cfg->i2c_bus);

  printk("i2c adapter = 0x%0x\n", cfg->i2c_adap);

  cfg->i2c_addr = tuner_cfg->i2c_addr;


if (cfg->i2c_adap == NULL) {

    printk ("fe-core: failed to allocate resources (%s)\n",
    		(cfg->i2c_adap == NULL)?"i2c":"STPIO error");
    kfree (cfg);
    return NULL;
  } else printk ("fe-core: succeded to allocate resources\n");

  frontend = frontend_init(cfg, i);

  if (frontend == NULL)
  {
	printk("No frontend found !\n");
    return NULL;
  }

  printk (KERN_INFO "%s: Call dvb_register_frontend (adapter = 0x%x)\n",
           __FUNCTION__, (unsigned int) adapter);

  if (dvb_register_frontend (adapter, frontend))
  {
    printk ("%s: Frontend registration failed !\n", __FUNCTION__);
    if (frontend->ops.release)
      frontend->ops.release (frontend);
    return NULL;
  }

  state = frontend->demodulator_priv;

  return frontend;
}

struct plat_tuner_config tuner_resources[] = {

        [0] = {
                .adapter 	= 0,
                .i2c_bus 	= 0,
		.i2c_addr 	= I2C_ADDR_STV090X,
        },
        [1] = {
                .adapter 	= 0,
                .i2c_bus 	= 0,
		.i2c_addr 	= I2C_ADDR_STV090X,
        },
#ifdef DVBT
        [2] = {
                .adapter = 0,
                .i2c_bus = 0,
                .i2c_addr = 0x08,
        },
#endif
};

void stv090x_register_frontend(struct dvb_adapter *dvb_adap)
{
	int i = 0;
	int vLoop = 0;

	printk (KERN_INFO "%s: PACE7241 frontend core\n", __FUNCTION__);


	gpio_request(RESET, "RESET_STV0900");
	if (RESET==NULL)
	{
		printk("Request RESET_STV0900 failed !!!");
	}
	else
	{
		printk("Request RESET_STV0900 succeded :)");
		gpio_direction_output(RESET, STM_GPIO_DIRECTION_OUT);
		gpio_set_value(RESET, 0);
		msleep(100);
		gpio_set_value(RESET, 1);
		msleep(500);
	}


	core[i] = (struct core*) kmalloc(sizeof(struct core),GFP_KERNEL);
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
				   init_fe_device (core[i]->dvb_adapter, &tuner_resources[vLoop], vLoop);
	  }
	}

	printk (KERN_INFO "%s: <\n", __FUNCTION__);

	return;
}

EXPORT_SYMBOL(stv090x_register_frontend);

int __init fe_core_init(void)
{

    printk("frontend core loaded\n");
    return 0;
}

static void __exit fe_core_exit(void)
{
   printk("frontend core unloaded\n");
}

module_init             (fe_core_init);
module_exit             (fe_core_exit);

MODULE_DESCRIPTION      ("Tunerdriver Pace 7241");
MODULE_AUTHOR           ("Team Ducktales mod j00zek");
MODULE_LICENSE          ("GPL");
