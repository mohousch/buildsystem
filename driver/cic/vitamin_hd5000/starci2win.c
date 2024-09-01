#include <linux/module.h>
#include <linux/dvb/audio.h>
#include <linux/dvb/ca.h>
#include <linux/io.h>
#include <linux/delay.h>		// for msleep
#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/i2c-algo-bit.h>
#include <linux/stm/pio.h>
#include "dvb_ca_en50221.h"

#define STARCI2_ADDR		0x40
#define STARCI2_I2C_BUS		2
#define STARCI2_MAX_SLOTS	2
#define SLOT_RESETS_MAX		5

#define CI_ADDR_MAP_SIZE	0x00100000
#define CI_ADDR_MOD_A		0x00010000
#define CI_ADDR_MOD_B		0x00020000
#define CI_ADDR_MOD_C		0x00030000
#define CI_ADDR_EXT			0x00030000

#define REG12				(((CI_ADDR_MAP_SIZE-1) >> 23) & 0x07)
#define REG13 				(((CI_ADDR_MAP_SIZE-1) >> 15) & 0xff)
#define REG14				((CI_ADDR_EXT >> 23) & 0x07)
#define REG15				((CI_ADDR_EXT >> 15) & 0xff)

#define SLOT_STATUS_NONE	0x01
#define SLOT_STATUS_PRESENT	0x02
#define SLOT_STATUS_RESET	0x04
#define SLOT_STATUS_READY	0x08
#define SLOT_STATUS_OCCUPIED	(SLOT_STATUS_PRESENT | SLOT_STATUS_RESET | SLOT_STATUS_READY)
#define BANKMEM_OFFSET		0x10000



static int debug = 1;

static struct starci2_cfg 
{
    int inited;
	struct mutex lock;
	int config_addr;
	struct dvb_ca_en50221 ca;
	struct 
	{
		int mode;
		unsigned char input[STARCI2_MAX_SLOTS];
		unsigned char output[STARCI2_MAX_SLOTS];
	} slot;
	struct 
	{
		unsigned long config;
		unsigned long buffer;
		unsigned long bank2;
	} reg;
	unsigned char slot_state[STARCI2_MAX_SLOTS];
	unsigned char slot_resets[STARCI2_MAX_SLOTS]; // unused, yet (should hold max resets per insertion)
	unsigned long detection_timeout[STARCI2_MAX_SLOTS];
	struct i2c_adapter *i2c;
} cicam;

/* StarCI2Win register definitions */
#define MODA_CTRL_REG 0x00
#define MODA_MASK_HIGH_REG 0x01
#define MODA_MASK_LOW_REG 0x02
#define MODA_PATTERN_HIGH_REG 0x03
#define MODA_PATTERN_LOW_REG 0x04
#define MODA_TIMING_REG 0x05
#define MODB_CTRL_REG 0x09
#define MODB_MASK_HIGH_REG 0x0a
#define MODB_MASK_LOW_REG 0x0b
#define MODB_PATTERN_HIGH_REG 0x0c
#define MODB_PATTERN_LOW_REG 0x0d
#define MODB_TIMING_REG 0x0e
#define SINGLE_MODE_CTRL_REG 0x10
#define TWIN_MODE_CTRL_REG 0x11
#define DEST_SEL_REG 0x17
#define INT_STATUS_REG 0x1a
#define INT_MASK_REG 0x1b
#define INT_CONFIG_REG 0x1c
#define STARCI_CTRL_REG 0x1f

static int starci2_writereg(int reg, int data)
{
        u8 buf[] = { reg, data };
        struct i2c_msg msg = { .addr = cicam.config_addr,
                .flags = 0, .buf = buf, .len = 2 };
        int err;

	if(debug > 1)
                printk("%s:  write reg 0x%02x, value 0x%02x\n",
                                                __FUNCTION__,reg, data);

        if ((err = i2c_transfer(cicam.i2c, &msg, 1)) != 1) {
                printk("%s: writereg error(err == %i, reg == 0x%02x,"
                         " data == 0x%02x)\n", __FUNCTION__, err, reg, data);
                return -EREMOTEIO;
        }

        return 0;
}

static int starci2_readreg(u8 reg)
{
	int ret;
	u8 b0[] = { reg };
	u8 b1[] = { 0 };

	struct i2c_msg msg[] = {
        { .addr = cicam.config_addr, .flags = 0, .buf = b0, .len = 1 },
    	{ .addr = cicam.config_addr, .flags = I2C_M_RD, .buf = b1, .len = 1 }
	}; 

	ret = i2c_transfer(cicam.i2c, msg, 2);

	if (ret != 2) 
	{
        printk("%s: reg=0x%x (error=%d)\n", __FUNCTION__, reg, ret);
    	return -EREMOTEIO;
    }

	if(debug > 1)
		printk("%s read reg 0x%02x, value 0x%02x\n",__FUNCTION__, reg, b1[0]);

    return b1[0];
}

//
// external module API follow ...
//

int ciintf_detect(void)
{
	struct i2c_adapter *i2c;
	struct stpio_pin *cic_enable_pin = NULL;
	int val;
    u8 b0[] = { 0x00 }; // MOD-A CTRL
    u8 b1[] = { 0 };
    struct i2c_msg msg[] = {
    	{ .addr = STARCI2_ADDR, .flags = 0, .buf = b0, .len = 1 },
    	{ .addr = STARCI2_ADDR, .flags = I2C_M_RD, .buf = b1, .len = 1 }
    };

	if((i2c = i2c_get_adapter(STARCI2_I2C_BUS)) == NULL) 
	{
		printk ("%s: Error: Can't find i2c adapter #0\n", __FUNCTION__);
		return -ENODEV;
	}
	
	cic_enable_pin = stpio_request_pin (1, 6, "StarCI_RST", STPIO_OUT);
  	stpio_set_pin (cic_enable_pin, 1);
  	msleep(250);
  	stpio_set_pin (cic_enable_pin, 0);
  	msleep(250);

    val = i2c_transfer(i2c, msg, 2);
	if(val < 0)
		return val;
	if(val != 2)
		return -ENODEV;

	printk("%s: found supported device on %d@%x\n", __func__, STARCI2_I2C_BUS, STARCI2_ADDR);

	return 0;
}

int ciintf_set_twin_mode(int on, int locked)
{
	int rv = 0;

	if((on && cicam.slot.mode) || (!on && !cicam.slot.mode))
		return 0;

	if(!locked)
		mutex_lock(&cicam.lock);

	if(on) 
	{
		// TWIN MODE
		cicam.slot.mode = 1;
		cicam.slot.input[0] = 0; // 0 = tuner-A, 1 = tuner-B, 2 = ci-A, 3 = ci-B
		cicam.slot.input[1] = 1;
		cicam.slot.output[0] = 0;
		cicam.slot.output[1] = 1;
		starci2_writereg(0x10, 0x00); // TS PATH
		starci2_writereg(0x11, 0x98); // TS PATH: TWIN ON, TSI1->1st TSI, TSI2->2nd TSI
		printk("CICAM: TWIN mode set\n");
	} 
	else 
	{
		// SINGLE MODE
		cicam.slot.mode = 0;
		cicam.slot.input[0] = 0;
		cicam.slot.input[1] = 2;
		cicam.slot.output[0] = 2;
		cicam.slot.output[1] = 1;
		starci2_writereg(0x10, 0x00); // TS PATH
		starci2_writereg(0x11, 0x00); // TS PATH: TWIN OFF
		printk("CICAM: SINGLE mode set\n");
	}

	if(!locked)
		mutex_unlock(&cicam.lock);

	return rv;
}

int ciintf_init(struct dvb_ca_en50221 *ca)
{
	int i;
	unsigned long res;

	if(cicam.inited) {
		// expect this is second, of init => second device
		return 0;
	}
    cicam.inited = 1;

	mutex_init(&cicam.lock);
	mutex_lock(&cicam.lock);

	if((cicam.i2c = i2c_get_adapter(STARCI2_I2C_BUS)) == NULL) 
	{
		printk ("%s: Error: Can't find i2c adapter #0\n", __FUNCTION__);
		return -1;
	}
	cicam.config_addr = STARCI2_ADDR;
	for(i = 0; i < STARCI2_MAX_SLOTS; i++) 
	{
		cicam.slot_state[i] = SLOT_STATUS_NONE;
		cicam.slot_resets[i] = SLOT_RESETS_MAX;
		cicam.detection_timeout[i] = 0;
	}
	
	starci2_writereg(0x1F, 0x80); // RESET ON
	msleep(10); //FIXME: better value?
	starci2_writereg(0x1F, 0x00); // RESET OFF
	

#if 0
/*0x00  0x01  0x02  0x03  0x04  0x05  0x06  0x07  0x08  0x09  0x0a  0x0b  0x0c  0x0d  0x0e  0x0f */
  0x02, 0x00, 0x01, 0x00, 0x00, 0x33, 0x00, 0x00, 0x00, 0x02, 0x00, 0x01, 0x00, 0x00, 0x33, 0x00,
  
/*0x10  0x11  0x12  0x13  0x14  0x15  0x16  0x17  0x18  0x19  0x1a  0x1b  0x1c  0x1d  0x1e  0x1f */  
  0x00, 0x98, REG12,REG13,REG14,REG15,0x00, 0x01, 0xC0, 0x00, 0x00, 0x03, 0x01, 0x00, 0x03, 0x00
#endif  	
	starci2_writereg(0x00, 0x62); // MOD-A CONTROL AUTO-ON,TSIEN,TSOEN
	starci2_writereg(0x01, 0x00); // MOD-A AUTOSELECT MSK HI
	starci2_writereg(0x02, 0x01); // MOD-A AUTOSELECT MSK LO
	starci2_writereg(0x03, 0x00); // MOD-A AUTOSELECT PATT HI
	starci2_writereg(0x04, 0x00); // MOD-A AUTOSELECT PATT LO
	starci2_writereg(0x05, 0x33); // MOD-A AM,CM TIMING
	starci2_writereg(0x09, 0x62); // MOD-B CONTROL AUTO-ON,TSIEN,TSOEN
	starci2_writereg(0x0A, 0x00); // MOD-B AUTOSELECT MSK HI
	starci2_writereg(0x0B, 0x01); // MOD-B AUTOSELECT MSK LO
	starci2_writereg(0x0C, 0x00); // MOD-B AUTOSELECT PATT HI
	starci2_writereg(0x0D, 0x00); // MOD-B AUTOSELECT PATT LO
	starci2_writereg(0x0E, 0x33); // MOD-B AM,CM TIMING
	starci2_writereg(0x12, REG12);
	starci2_writereg(0x13, REG13);
	starci2_writereg(0x14, REG14);
	starci2_writereg(0x15, REG15);
	starci2_writereg(0x17, 0x01); // SET AUTO MODULE SELECT
	starci2_writereg(0x18, 0xC0); // SET PINS
	starci2_writereg(0x1B, 0x03); // MASK ALL INTERRUPTS
	starci2_writereg(0x1C, 0x01);
	starci2_writereg(0x1D, 0x00); // RD,DIR
	starci2_writereg(0x1E, 0x03); // WAIT BUFF

    cicam.slot.mode = 0; // faked to force processing on next line
	ciintf_set_twin_mode(1, 1); /* STARCI TWIN MODE */

	starci2_writereg(0x1F, 0x01); // LOCK CONFIG
	starci2_writereg(0x18, 0xC1); // POWER ON

	//
	// --- remapping stuffs ---
	//
#define EMIConfigBaseAddress	(0xFE700000L)
#define EMIBank2 				0x180
#define EMIBank2BaseAddress 	(EMIConfigBaseAddress + EMIBank2)
#define EMI_CFG_DATA0			0x0000
#define EMI_CFG_DATA1			0x0008
#define EMI_CFG_DATA2			0x0010
#define EMI_CFG_DATA3			0x0018


	cicam.reg.config = (unsigned long)ioremap(EMIConfigBaseAddress, 0x800); // 0x7ff
	printk("EMICfg %lX -> %lX\n", EMIConfigBaseAddress, cicam.reg.config);

	writel( 0x00, cicam.reg.config + 0x18);            //      EMI_STA_LCK : unlock
	writel( 0x00, cicam.reg.config + 0x20);            //      EMI_LCK : unlock

	// bank 2 config
	writel(0x002046f9, cicam.reg.config + EMIBank2 + EMI_CFG_DATA0);
	writel(0xa5a00000, cicam.reg.config + EMIBank2 + EMI_CFG_DATA1);
	writel(0xa5a20000, cicam.reg.config + EMIBank2 + EMI_CFG_DATA2);
  	writel(0x00000000, cicam.reg.config + EMIBank2 + EMI_CFG_DATA3);

	writel( 0x04, cicam.reg.config + 0x10);            //      EMI_STA_CFG : update
	writel( 0x1f, cicam.reg.config + 0x18);            //      EMI_STA_LCK : lock
	writel( 0x1f, cicam.reg.config + 0x20);            //      EMI_LCK : lock


	// bank2
	res = ctrl_inl(cicam.reg.config + 0x0800 + 0x20); //bank2
	res <<= 22;
	cicam.reg.bank2 = (unsigned long)ioremap(res, 0x02000000);
	printk("CI bank2 mapped %lX -> %lX\n", res, cicam.reg.bank2);

	// CI power
	{
		struct stpio_pin *pwr = NULL;
		if((pwr = stpio_request_pin( 0, 6, "CI_PWR", STPIO_OUT)) == NULL)
			printk("ERR: Can't acquire CI power pin0.6!\n");
		else 
		{
			printk("CI power pin0.6 acquired OK\n");
			stpio_set_pin(pwr,1);
		}
	}

	mutex_unlock(&cicam.lock);

	return 0;
}

/* NOTE: the read_*, write_* and poll_slot_status functions will be
 * called for different slots concurrently and need to use locks where
 * and if appropriate. There will be no concurrent access to one slot.
 */

static int ciintf_read_attribute_mem_locked(struct dvb_ca_en50221 *ca, int slot, int address)
{
	int val, res = 0;
	int real_slot = slot;

	val = starci2_readreg(real_slot == 0 ? 0x00 : 0x09); // MOD-A CTRL
	if(val < 0) 
		return 0;

	if((val & 0x01) == 0) // 0 = no module detected
		return 0;

	//if(val & 0x20) // mpeg control active
	//	return 0;

	starci2_writereg(real_slot == 0 ? 0x00 : 0x09, val & 0xF3); // MOD-A CTRL: access to attribute mem
	res = ctrl_inb(cicam.reg.bank2 + (BANKMEM_OFFSET*real_slot) + (address << 0));
	//res &= 0xFF;

	if(address <= 2) 
		printk("ATTRMEM#%d: address = %lx(%u): res = %x\n", real_slot, cicam.reg.bank2 + (BANKMEM_OFFSET*real_slot) + (address << 1), address, res);
	else 
	{
		if(res > 31 && res < 127)
			printk("%c", res);
		else
			printk(".");
	}
	return res;
}

int ciintf_read_attribute_mem(struct dvb_ca_en50221 *ca, int slot, int address)
{
	int res;

	mutex_lock(&cicam.lock);
	res = ciintf_read_attribute_mem_locked(ca, slot, address);
	mutex_unlock(&cicam.lock);

	return res;
}

int ciintf_write_attribute_mem(struct dvb_ca_en50221 *ca, int slot, int address, u8 value)
{
	int val;
	int real_slot = slot;

	mutex_lock(&cicam.lock);

	val = starci2_readreg(real_slot == 0 ? 0x00 : 0x09); // MOD-A CTRL
	if(val < 0) 
	{
		mutex_unlock(&cicam.lock);
		return 0;
	}
	
	if((val & 0x01) == 0) // 0 = no module detected
	{ 
		mutex_unlock(&cicam.lock);
		return 0;
	}
	//if(val & 0x20) // mpeg control active
	//	return 0;

	starci2_writereg(real_slot == 0 ? 0x00 : 0x09, 0x63); // MOD-A CTRL: access to attribute mem
	ctrl_inb(cicam.reg.bank2  + (BANKMEM_OFFSET*real_slot) + (address << 0));

	ctrl_outb(value, cicam.reg.bank2  + (BANKMEM_OFFSET*real_slot) + (address << 0));

	mutex_unlock(&cicam.lock);

	return 0;
}

int ciintf_read_cam_control(struct dvb_ca_en50221 *ca, int slot, u8 address)
{
	int val, res = 0;
	int real_slot = slot;

	mutex_lock(&cicam.lock);

	val = starci2_readreg(real_slot == 0 ? 0x00 : 0x09); // MOD-A CTRL
	if(val < 0) 
	{
		mutex_unlock(&cicam.lock);
		return 0;
	}
	
	if((val & 0x01) == 0) // 0 = no module detected
	{ 
		mutex_unlock(&cicam.lock);
		return 0;
	}
	//if(val & 0x20) // mpeg control active
	//	return 0;

	starci2_writereg(real_slot == 0 ? 0x00 : 0x09, (val & 0xF3) | 0x04); // MOD-A CTRL: access to i/o mem
	res = ctrl_inb(cicam.reg.bank2  + (BANKMEM_OFFSET*real_slot) + (address << 0));
	res &= 0xFF;

	mutex_unlock(&cicam.lock);

	return res;
}

int ciintf_write_cam_control(struct dvb_ca_en50221 *ca, int slot, u8 address, u8 value)
{
	int val;
	int real_slot = slot;

	//printk("[%s]\n", __FUNCTION__);

	mutex_lock(&cicam.lock);

	val = starci2_readreg(real_slot == 0 ? 0x00 : 0x09); // MOD-A CTRL
	if(val < 0) 
	{
		mutex_unlock(&cicam.lock);
		return 0;
	}
	
	if((val & 0x01) == 0) // 0 = no module detected 
	{ 
		mutex_unlock(&cicam.lock);
		return 0;
	}
	//if(val & 0x20) // mpeg control active
	//	return 0;

	starci2_writereg(real_slot == 0 ? 0x00 : 0x09, (val & 0xF3) | 0x04); // MOD-A CTRL: access to i/o mem
	ctrl_outb(value, cicam.reg.bank2  + (BANKMEM_OFFSET*real_slot) + (address << 0));

	mutex_unlock(&cicam.lock);

	return 0;
}

int ciintf_slot_reset(struct dvb_ca_en50221 *ca, int slot)
{
	int res;

	printk("[%s %d]\n", __FUNCTION__, slot);

	mutex_lock(&cicam.lock);

	res = starci2_readreg(slot == 0 ? 0x00 : 0x09);
	if(res & 0x01) 
	{
		starci2_writereg(slot == 0 ? 0x00 : 0x09, res | 0x80); // MOD-A CTRL
		msleep(60);
		printk("RESET module %d\n", slot);
		starci2_writereg(slot == 0 ? 0x00 : 0x09, 0x02); // MOD-A CTRL
		cicam.slot_state[slot] = SLOT_STATUS_NONE;
		cicam.detection_timeout[slot] = 0;
	}

	mutex_unlock(&cicam.lock);

	return 0;
}

int ciintf_slot_shutdown(struct dvb_ca_en50221 *ca, int slot)
{
	printk("[%s] slot %d\n", __FUNCTION__, slot);
	return 0;
}

int ciintf_slot_ts_enable(struct dvb_ca_en50221 *ca, int slot)
{
	int real_slot = slot;

	mutex_lock(&cicam.lock);

	starci2_writereg(real_slot == 0 ? 0x00 : 0x09, 0x62); // MPEG on, no bypass
	printk("[%s] slot %d\n", __FUNCTION__, real_slot);

	mutex_unlock(&cicam.lock);

	return 0;
}

int ciintf_poll_slot_status(struct dvb_ca_en50221 *ca, int slot, int open)
{
	int debug_ori;
	int slot_status = 0;

	//printk("[%s]\n", __FUNCTION__);

	if ((slot < 0) || (slot > 1))
		return 0;

	mutex_lock(&cicam.lock);

	debug_ori = debug;
	debug = 0; // DISABLE DEBUG
	slot_status = starci2_readreg(slot == 0 ? 0x00 : 0x09) & 0x01; // MOD-A CTRL
	debug = debug_ori;

	if(slot_status < 0) 
	{
		mutex_unlock(&cicam.lock);
		return 0;
	}

	if(slot_status == 1) // 1 = module detected
	{
	  	/* Phantomias: an insertion should not be reported immediately
		 * because the module needs time to power up. Therefore the
		 * detection is reported after the module has been present for
		 * the specified period of time (to be confirmed in tests). 
		 */
		if(cicam.slot_state[slot] & SLOT_STATUS_NONE)
		{
			if(cicam.detection_timeout[slot] == 0)
			{
				/* detected module insertion, set the detection
			   	 * timeout (500 ms) */
				cicam.detection_timeout[slot] = jiffies + HZ/2;
			}
			else
			{
				/* timeout in progress */
				if(time_after(jiffies, cicam.detection_timeout[slot]))
				{
					/* timeout expired, report module present */
					cicam.slot_state[slot] = SLOT_STATUS_RESET;
				}
			}
		}

		// during a RESET, we check if we can read from IO memory to see when CAM is ready
		if(cicam.slot_state[slot] & SLOT_STATUS_RESET)
		{
			if(ciintf_read_attribute_mem_locked(ca, slot, 0) == 0x1d)
			{
				printk("DEB: >>> slot %d: DVB_CA_EN50221_POLL_CAM_READY\n", slot);
				cicam.slot_state[slot] = SLOT_STATUS_READY;
			}
			else
			{
				cicam.slot_state[slot] = SLOT_STATUS_NONE;
				cicam.detection_timeout[slot] = 0;
			}
		}
	}
	else
	{
		cicam.slot_state[slot] = SLOT_STATUS_NONE;
		cicam.detection_timeout[slot] = 0;
	}
	
	slot_status = slot_status ? DVB_CA_EN50221_POLL_CAM_PRESENT : 0;
	if(cicam.slot_state[slot] & SLOT_STATUS_READY)
		slot_status |= DVB_CA_EN50221_POLL_CAM_READY;

	/*printk("Module %d: present = %d, ready = %d\n",
			  slot, slot_status & DVB_CA_EN50221_POLL_CAM_PRESENT,
			  slot_status & DVB_CA_EN50221_POLL_CAM_READY);*/

	mutex_unlock(&cicam.lock);

	return slot_status;
}

/* This function sets the CI source
   Params:
     slot - CI slot number (0|1)
     source - tuner number (0|1) or other CI slot (2|3)
*/

int setInputSource(int pti, int source)
{
	printk("setInputSource (%d , %d)\n", pti, source);
	
	if((pti < 0) || (pti > 1) || (source < 0) || (source > 3))
	        return -1;
	        
	if (pti == 0)
	{
		switch (source)
    	{
    		case 0: printk("[%s] TUNER_A->TSIN0\n",__func__); break;
   			case 1: printk("[%s] TUNER_B->TSIN0\n",__func__); break;
   			case 2: printk("[%s] CI0->TSIN0\n",__func__); break;
   			case 3: printk("[%s] CI1->TSIN0\n",__func__); break;
   		}
	} 
	else 
	{
		switch (source)
    	{
    		case 0: printk("[%s] TUNER_A->TSIN1\n",__func__); break;  			
    		case 1: printk("[%s] TUNER_B->TSIN1\n",__func__); break;   			
    		case 2: printk("[%s] CI0->TSIN1\n",__func__); break;			
    		case 3: printk("[%s] CI1->TSIN1\n",__func__); break;   			
    	}
	}
	return 0;
}
EXPORT_SYMBOL ( setInputSource );

/* This function gets the CI source
   Params:
     slot - CI slot number (0|1)
     source - tuner number (0|1)
*/
void getCiSource(int slot, int* source)
{
  int val;

  val = starci2_readreg(TWIN_MODE_CTRL_REG);
  val &= 0x20;

  if(slot == 0)
  {
    if(val != 0) 
      *source = 0;
    else
      *source = 1;
  }

  if(slot == 1)
  {
    if(val != 0)
      *source = 1;
    else
      *source = 0;
  }
}
EXPORT_SYMBOL(getCiSource);

/* This function sets the CI source
   Params:
     slot - CI slot number (0|1)
     source - tuner number (0|1)
*/
int setCiSource(int slot, int source)
{
	int val;

	printk("setCiSource (ci=%d , fe=%d)\n", slot, source);
	if((slot < 0) || (slot > 1) /* invalid slot */ 
		      || (source > 3) /* invalid source */ 
		      || (slot == 0 && source == 2) /* nonsence */ 
		      || (slot == 1 && source == 3) /* nonsence */)
		return -1;

	val = starci2_readreg(0x11); // TWIN MODE TS CONTROL REG
	if(val < 0)
		return 0;

   	switch (source)
   	{
		case -1:
		{
			printk("[%s] CI%d->bypass\n",__func__, slot);
			val |= slot == 0 ? 0x10 : 0x08;
			starci2_writereg(0x11, val);
			break;
		}
    	case 0: 
    	case 1: 
    	{
			printk("[%s] TUNER_%c->CI%d\n",__func__, source ? 'B' : 'A', slot);
	    	if((slot == source && (val & 0x20)) || (slot != source && (val & 0x20) == 0)) 
				val ^= 0x20;
			val &= ~(slot == 0 ? 0x10 : 0x08); // disable bypass again
			printk("setCiSource: new ctrlx11=0x%x\n", val);
			starci2_writereg(0x11, val);
			break;
		}	
   		case 2: printk("[%s] CI0->CI%d\n",__func__, slot); break;
    	case 3: printk("[%s] CI1->CI%d\n",__func__, slot); break;
    }
	return 0;
}
EXPORT_SYMBOL ( setCiSource );

int init_ci_controller (struct dvb_adapter* dvb_adap)
{	
	// --- CI switch support ---
	int result;
	// do autodetect stuff
	result = ciintf_detect();
	if (result)
		printk("CICAM chip not detected.\n");
	else 
	{
		// register CI interface
		memset(&cicam.ca, 0, sizeof(struct dvb_ca_en50221));
		cicam.ca.owner = THIS_MODULE;
		cicam.ca.read_attribute_mem = ciintf_read_attribute_mem;
		cicam.ca.write_attribute_mem = ciintf_write_attribute_mem;
		cicam.ca.read_cam_control = ciintf_read_cam_control;
		cicam.ca.write_cam_control = ciintf_write_cam_control;
		cicam.ca.slot_reset = ciintf_slot_reset;
		cicam.ca.slot_shutdown = ciintf_slot_shutdown;
		cicam.ca.slot_ts_enable = ciintf_slot_ts_enable;
		cicam.ca.poll_slot_status = ciintf_poll_slot_status;
		cicam.ca.data = NULL;

		result = dvb_ca_en50221_init(dvb_adap, &cicam.ca, 0, 2);
		if (result != 0) 
		{
			printk("CICAM detected, but registration failed (errno = %d).\n", result);
			//goto error;
			return result;
		} 
		else 
		{
			// init cicam
			result = ciintf_init(&cicam.ca);
			if (result != 0) 
			{
				printk("CICAM initialisation failed (errno = %d).\n", result);
				return result;
			} 
			else
				printk("CICAM initialisation OK.\n");
		}
	} // autodetect
	return 0;
}
EXPORT_SYMBOL(init_ci_controller);

int starci2win_init(void)
{
    printk("starci2win loaded\n");
    return 0;
}

void starci2win_exit(void)
{  
   printk("starci2win unloaded\n");
}

module_init             (starci2win_init);
module_exit             (starci2win_exit);

MODULE_DESCRIPTION      ("CI Controller");
MODULE_AUTHOR           ("");
MODULE_LICENSE          ("GPL");

