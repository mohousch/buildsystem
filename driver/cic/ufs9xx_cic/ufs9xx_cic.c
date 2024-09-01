/*
 * ufs912/922 ci controller handling.
 *
 * Registers:
 *
 *
 * Dagobert
 * GPL
 * 2010
 *
 * konfetti:
 * - we should think about moving the cam routing settings to another module
 * - we should think about a platform configuration
 *  
 */



#include <linux/version.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/string.h>
#include <linux/slab.h>

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/string.h>

#include <linux/platform_device.h>

#include <linux/interrupt.h>
#include <linux/i2c.h> 
#include <linux/i2c-algo-bit.h>
#include <linux/firmware.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,23)
#include <linux/stpio.h>
#else
#include <linux/stm/pio.h>
#endif

#include <asm/system.h>
#include <asm/io.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,30)
#include <asm/semaphore.h>
#else
#include <linux/semaphore.h>
#endif
#include <linux/dvb/dmx.h>

#include "dvb_frontend.h"
#include "dvbdev.h"
#include "demux.h"
#include "dvb_demux.h"
#include "dmxdev.h"
#include "dvb_filter.h"
#include "dvb_net.h"
#include "dvb_ca_en50221.h"

#include "ufs9xx_cic.h"

static int paramDebug;

#define TAGDEBUG "[ufs9xx_cic] "

#define dprintk(level, x...) do { \
if ((paramDebug) && (paramDebug >= level)) printk(TAGDEBUG x); \
} while (0)


/* ***** emi for 7111&7105 */
#if defined(UFS912) || defined(UFS913)
#define EMIConfigBaseAddress 0xfe700000
#elif defined(UFS922) || defined(UFC960)
#define EMIConfigBaseAddress 0x1A100000
#endif


#define EMI_LCK 	0x0020
#define EMI_GEN_CFG 0x0028

#define EMI_FLASH_CLK_SEL 0x0050 /* WO: 00, 10, 01 */
#define EMI_CLK_EN 0x0068 /* WO: must only be set once !!*/

#define EMIBank0 0x100
#define EMIBank1 0x140
#define EMIBank2 0x180
#define EMIBank3 0x1C0
#define EMIBank4 0x200
#define EMIBank5 0x240 /* virtual */

#define EMI_CFG_DATA0	0x0000
#define EMI_CFG_DATA1	0x0008
#define EMI_CFG_DATA2	0x0010
#define EMI_CFG_DATA3	0x0018

#define EMIBank0_BASE_ADDR 0x800
#define EMIBank1_BASE_ADDR 0x810
#define EMIBank2_BASE_ADDR 0x820
#define EMIBank3_BASE_ADDR 0x830
#define EMIBank4_BASE_ADDR 0x840

/* ***** end 7111 emi */

/* *************************************************
 * For the reason I dont understand the bits in the
 * i2c registers I use this constants.
 */
#define	TUNER_1_VIEW			0
#define	TUNER_1_CAM_A_VIEW		1
#define	TUNER_1_CAM_B_VIEW		2
#define	TUNER_1_CAM_A_CAM_B_VIEW	3
#define	TUNER_2_CAM_A			4
#define	TUNER_2_CAM_B			5
#define	TUNER_2_CAM_A_B			6
#define	TUNER_2_VIEW			7
#define	TUNER_1_CAM_A_TUNER_2_CAM_B	8
#define	TUNER_1_VIEW_CAM_A		9

static struct ufs9xx_cic_core ci_core;
static struct ufs9xx_cic_state ci_state;

#if defined(UFS922) || defined(UFS913) || defined(UFC960)
void set_ts_path(int route);
void set_cam_path(int route);
#endif

#if defined(UFS912)
static int waitMS = 60;
#elif defined(UFS913) || defined(UFC960)
static int waitMS = 200;
#else
static int waitMS = 100;
#endif

static int ufs9xx_cic_read_attribute_mem(struct dvb_ca_en50221 *ca, int slot, int address);

/* *************************** */
/* map, write & read functions */
/* *************************** */

unsigned char ufs9xx_read_register_u8(unsigned long address)
{
	unsigned char result;

	volatile unsigned long mapped_register = address;

	//dprintk(200, "%s > address = 0x%.8lx, mapped = 0x%.8lx\n", __FUNCTION__, (unsigned long) address, mapped_register);

	result = readb(mapped_register);
     
	return result;
}

void ufs9xx_write_register_u8(unsigned long address, unsigned char value)
{
	volatile unsigned long mapped_register = address;

	writeb(value, mapped_register);
     
}

unsigned int ufs9xx_read_register_u32(unsigned long address)
{
	unsigned int result;

	volatile unsigned long mapped_register = address;

	//dprintk(200, "%s > address = 0x%.8lx, mapped = 0x%.8lx\n", __FUNCTION__, (unsigned long) address, mapped_register);

	result = readl(mapped_register);
     
	return result;
}

void ufs9xx_write_register_u32(unsigned long address, unsigned int value)
{
	volatile unsigned long mapped_register = address;

	writel(value, mapped_register);
     
}

unsigned int ufs9xx_read_register_u32_map(unsigned long address)
{
	unsigned int result;

	volatile unsigned long mapped_register = (unsigned long) ioremap_nocache(address, 4);

	result = readl(mapped_register);
     
	iounmap((void*) mapped_register);
    
	return result;
}

void ufs9xx_write_register_u32_map(unsigned long address, unsigned int value)
{
	volatile unsigned long mapped_register = (unsigned long) ioremap_nocache(address, 4);

	writel(value, mapped_register);
     
	iounmap((void*) mapped_register);
}

#if defined(UFS922) || defined(UFS913) || defined(UFC960)
static int ufs9xx_cic_readN (struct ufs9xx_cic_state *state, u8 * buf, u16 len)
{
	int ret = -EREMOTEIO;
	struct i2c_msg msg;

	msg.addr = state->i2c_addr;
	msg.flags = I2C_M_RD;
	msg.buf = buf;
	msg.len = len;

	if ((ret = i2c_transfer (state->i2c, &msg, 1)) != 1)
	{
		printk ("%s: writereg error(err == %i)\n",
			__FUNCTION__, ret);
		ret = -EREMOTEIO;
	}

	return ret;
}

static int ufs9xx_cic_writereg(struct ufs9xx_cic_state* state, int reg, int data)
{
	u8 buf[] = { reg, data };
	struct i2c_msg msg = { .addr = state->i2c_addr,
		.flags = 0, .buf = buf, .len = 2 };
	int err;

	if ((err = i2c_transfer(state->i2c, &msg, 1)) != 1) {
		printk("%s: writereg error(err == %i, reg == 0x%02x,"
			 " data == 0x%02x)\n", __FUNCTION__, err, reg, data);
		return -EREMOTEIO;
	}

	return 0;
}
#endif

static int ufs9xx_cic_poll_slot_status(struct dvb_ca_en50221 *ca, int slot, int open)
{
	struct ufs9xx_cic_state *state = ca->data;
	int                     slot_status = 0;
	unsigned int            result;
#if defined(UFS922)
	u8                      buf[2];
#endif

	dprintk(150, "%s (%d; open = %d) >\n", __FUNCTION__, slot, open);

#if defined(UFS922)
/* hacky workaround for stmfb problem (yes I really mean stmfb ;-) ):
 * switching the hdmi output of my lcd to the ufs922 while running or
 * switching the resolution leads to a modification of register
 * 0x00 which disable the stream input for ci controller. it seems so
 * that the ci controller has no bypass in this (and in any) case.
 * so we poll it here and set it back.
 */ 
	buf[0] = 0x00;
	ufs9xx_cic_readN (state, buf, 1);

	if (buf[0] != 0x3f)
	{
		printk("ALERT: CI Controller loosing input %02x\n", buf[0]);
		
		ufs9xx_cic_writereg(state, 0x00, 0x3f);
	}	
#endif

	result = stpio_get_pin(state->slot_status[slot]);

	dprintk(120, "Slot %d Status = 0x%x\n", slot, result);

	if (result == 0x01)
		slot_status = 1;

	if (slot_status)
	{
		if (state->module_status[slot] & SLOTSTATUS_RESET)
		{
	/* the sequence from dvbapi is to reset the cam after a detection change,
	* so we save the state and check here if the module is ready. on ufs913/ufs922
	* we have a special ready pin, on ufs912 we read from attribute memory.
	*/

#if defined(UFS912)

#ifdef use_additional_waiting_period
			/* timeout in progress */
			if(time_after(jiffies, state->detection_timeout[slot]))
			{
#endif
				result = ufs9xx_cic_read_attribute_mem(ca, slot, 0); 

				dprintk(200, "result = 0x%02x\n", result);

				if (result == 0x1d)
					state->module_status[slot] = SLOTSTATUS_READY;
#ifdef use_additional_waiting_period
			}
#endif
#else

			result = stpio_get_pin(state->module_ready_pin[slot]);

			dprintk(200, "readyPin = %d\n", result);
			if (result)
				state->module_status[slot] = SLOTSTATUS_READY;
#endif
		}
		else if (state->module_status[slot] & SLOTSTATUS_NONE)
		{
#if !defined(UFS913)

			stpio_set_pin(state->slot_enable[slot], 0);
			mdelay(waitMS);
#else
			stpio_set_pin(state->slot_enable[slot], 1);
			mdelay(waitMS);
#endif

			dprintk(1, "Modul now present\n");
			state->module_status[slot] = SLOTSTATUS_PRESENT;
		}
	}
	else
	{
		if (!(state->module_status[slot] & SLOTSTATUS_NONE))
		{
#ifdef UFS913
			stpio_set_pin(state->slot_enable[slot], 0);
#else
			stpio_set_pin(state->slot_enable[slot], 1);
#endif
			dprintk(1, "Modul now not present\n");
			state->module_status[slot] = SLOTSTATUS_NONE;
		}
	}

	if (state->module_status[slot] != SLOTSTATUS_NONE)
		slot_status = DVB_CA_EN50221_POLL_CAM_PRESENT;
	else
		slot_status = 0;
   
	if (state->module_status[slot] & SLOTSTATUS_READY)
		slot_status |= DVB_CA_EN50221_POLL_CAM_READY;

	dprintk(120, "Module %c (%d): result = %d, status = %d\n",
				slot ? 'B' : 'A', slot, slot_status,
				state->module_status[slot]);

	return slot_status;
}

static int ufs9xx_cic_slot_reset(struct dvb_ca_en50221 *ca, int slot)
{
	struct ufs9xx_cic_state *state = ca->data;
	int aPresent, bPresent;

	dprintk(1, "%s >\n", __FUNCTION__);


	state->module_status[slot] = SLOTSTATUS_RESET;

#if defined(UFS922) || defined(UFS913) || defined(UFC960)
	aPresent = ((ci_state.module_status[0] & SLOTSTATUS_READY) || (ci_state.module_status[0] & SLOTSTATUS_PRESENT));
	bPresent = ((ci_state.module_status[1] & SLOTSTATUS_READY) || (ci_state.module_status[1] & SLOTSTATUS_PRESENT));

	if ((aPresent) && (!bPresent))
	{
		set_cam_path(TUNER_1_CAM_A_VIEW);
		set_ts_path(TUNER_1_CAM_A_VIEW);
	} else
	if ((!aPresent) && (bPresent))
	{
		set_cam_path(TUNER_1_CAM_B_VIEW);
		set_ts_path(TUNER_1_CAM_B_VIEW);
	} else
	if ((aPresent) && (bPresent))
	{
		set_cam_path(TUNER_1_CAM_A_CAM_B_VIEW);
		set_ts_path(TUNER_1_CAM_A_CAM_B_VIEW);
	} else
	if ((!aPresent) && (!bPresent))
	{
		set_cam_path(TUNER_1_VIEW);
		set_ts_path(TUNER_1_VIEW);
	}
#endif

#if defined(UFS912)
	stpio_set_pin(state->slot_enable[slot], 1);
	mdelay(50);
	stpio_set_pin(state->slot_enable[slot], 0);
	mdelay(50);
#endif
	stpio_set_pin(state->slot_reset[slot], 1);
	mdelay(waitMS);
	stpio_set_pin(state->slot_reset[slot], 0);
	mdelay(waitMS);

	state->detection_timeout[slot] = jiffies + HZ / 2;

	dprintk(1, "%s <\n", __FUNCTION__);
	return 0;
}

static int ufs9xx_cic_read_attribute_mem(struct dvb_ca_en50221 *ca, int slot, int address)
{
	struct ufs9xx_cic_state *state = ca->data;
	unsigned char res = 0;

	dprintk(100, "%s > slot = %d, address = 0x%.8lx\n", __FUNCTION__, slot, (unsigned long) state->slot_attribute_read[slot] + address);

	res = ufs9xx_read_register_u8(state->slot_attribute_read[slot] + address);

	if (address <= 2)
		dprintk (100, "address = %d: res = 0x%.x\n", address, res);
	else
	{
		if ((res > 31) && (res < 127))
			dprintk(100, "%c", res);
		else
			dprintk(150, ".");
	}

	return (int) res;
}

static int ufs9xx_cic_write_attribute_mem(struct dvb_ca_en50221 *ca, int slot, int address, u8 value)
{
	struct ufs9xx_cic_state *state = ca->data;

	dprintk(100, "%s > slot = %d, address = 0x%.8lx, value = 0x%.x\n", __FUNCTION__, slot, (unsigned long) state->slot_attribute_write[slot] + address, value);
	ufs9xx_write_register_u8(state->slot_attribute_write[slot] + address, value);

	return 0;
}

static int ufs9xx_cic_read_cam_control(struct dvb_ca_en50221 *ca, int slot, u8 address)
{
	struct ufs9xx_cic_state *state = ca->data;
	unsigned char res = 0;
	
	res = ufs9xx_read_register_u8(state->slot_control_read[slot] + address);

	dprintk (100, "%s (%d): address = 0x%x: res = 0x%x (0x%x)\n", __func__, slot, address, res, (int) res);

	return (int) res;
}

static int ufs9xx_cic_write_cam_control(struct dvb_ca_en50221 *ca, int slot, u8 address, u8 value)
{
	struct ufs9xx_cic_state *state = ca->data;

	dprintk(100, "%s > slot = %d, address = 0x%.8lx, value = 0x%.x\n", __FUNCTION__, slot, (unsigned long) state->slot_control_write[slot] + address, value);

	ufs9xx_write_register_u8(state->slot_control_write[slot] + address, value);

	return 0;
}

static int ufs9xx_cic_slot_shutdown(struct dvb_ca_en50221 *ca, int slot)
{
	struct ufs9xx_cic_state *state = ca->data;
	int aPresent, bPresent;

	dprintk(1, "%s > slot = %d\n", __FUNCTION__, slot);
    
#if defined(UFS913)
	stpio_set_pin(state->slot_enable[slot], 0);
#else
	stpio_set_pin(state->slot_enable[slot], 1);
#endif

#if defined(UFS922) || defined(UFS913) || defined(UFC960)
	aPresent = ((ci_state.module_status[0] & SLOTSTATUS_READY) || (ci_state.module_status[0] & SLOTSTATUS_PRESENT));
	bPresent = ((ci_state.module_status[1] & SLOTSTATUS_READY) || (ci_state.module_status[1] & SLOTSTATUS_PRESENT));

	if ((aPresent) && (!bPresent))
	{
		set_cam_path(TUNER_1_CAM_A_VIEW);
		set_ts_path(TUNER_1_CAM_A_VIEW);
	} else
	if ((!aPresent) && (bPresent))
	{
		set_cam_path(TUNER_1_CAM_B_VIEW);
		set_ts_path(TUNER_1_CAM_B_VIEW);
	} else
	if ((aPresent) && (bPresent))
	{
		set_cam_path(TUNER_1_CAM_A_CAM_B_VIEW);
		set_ts_path(TUNER_1_CAM_A_CAM_B_VIEW);
	} else
	if ((!aPresent) && (!bPresent))
	{
		set_cam_path(TUNER_1_VIEW);
		set_ts_path(TUNER_1_VIEW);
	}
#endif
	return 0;
}

static int ufs9xx_cic_slot_ts_enable(struct dvb_ca_en50221 *ca, int slot)
{
//	struct ufs9xx_cic_state *state = ca->data;
	int aPresent, bPresent;

	dprintk(20, "%s > slot = %d\n", __FUNCTION__, slot);

#if defined(UFS922) || defined(UFS913) || defined(UFC960)
	aPresent = ((ci_state.module_status[0] & SLOTSTATUS_READY) || (ci_state.module_status[0] & SLOTSTATUS_PRESENT));
	bPresent = ((ci_state.module_status[1] & SLOTSTATUS_READY) || (ci_state.module_status[1] & SLOTSTATUS_PRESENT));

	if ((aPresent) && (!bPresent))
	{
		set_cam_path(TUNER_1_CAM_A_VIEW);
		set_ts_path(TUNER_1_CAM_A_VIEW);
	} else
	if ((!aPresent) && (bPresent))
	{
		set_cam_path(TUNER_1_CAM_B_VIEW);
		set_ts_path(TUNER_1_CAM_B_VIEW);
	} else
	if ((aPresent) && (bPresent))
	{
		set_cam_path(TUNER_1_CAM_A_CAM_B_VIEW);
		set_ts_path(TUNER_1_CAM_A_CAM_B_VIEW);
	} else
	if ((!aPresent) && (!bPresent))
	{
		set_cam_path(TUNER_1_VIEW);
		set_ts_path(TUNER_1_VIEW);
	}
#endif

	return 0;
}

int setCiSource(int slot, int source)
{
#if defined(UFS922) || defined(UFS913) || defined(UFC960)
	int aPresent, bPresent;

	dprintk(1, "%s slot %d source %d\n", __FUNCTION__, slot, source);
  
	ci_state.module_source[slot] = source;

	aPresent = ((ci_state.module_status[0] & SLOTSTATUS_READY) || (ci_state.module_status[0] & SLOTSTATUS_PRESENT));
	bPresent = ((ci_state.module_status[1] & SLOTSTATUS_READY) || (ci_state.module_status[1] & SLOTSTATUS_PRESENT));

	if (source == 0)
	{
		if ((aPresent) && (!(bPresent)))
		{
			set_cam_path(TUNER_1_CAM_A_VIEW);				//	0x01	00000001	
			set_ts_path(TUNER_1_CAM_A_VIEW);				//	0x23	00100011
		} else
		if ((!(aPresent)) && (bPresent))
		{
			set_cam_path(TUNER_1_CAM_B_VIEW);				//	0x10	00010000
			set_ts_path(TUNER_1_CAM_B_VIEW);				//	0x24	00100100
		} else
		if ((aPresent) && (bPresent))
		{

			if (ci_state.module_source[1] == 1) {				//	Cam B and Tuner 2 in use
				set_cam_path(TUNER_1_CAM_A_TUNER_2_CAM_B);		//	0x21	00100001 T1_CAM_A_T2_CAM_B
				set_ts_path(TUNER_1_CAM_A_TUNER_2_CAM_B);		//	0x43	01000011 T1_CAM_A_T2_CAM_B
			} else if (ci_state.module_source[0] == 1) {			//	Cam A and Tuner 2 in use
				set_cam_path(TUNER_1_VIEW_CAM_A);			//	0x12	00010010 T1_CAM_B_T2_CAM_A
				set_ts_path(TUNER_1_VIEW_CAM_A);			//	0x34	00110100 T1_CAM_B_T2_CAM_A
			} else {
				set_cam_path(TUNER_1_CAM_A_CAM_B_VIEW);			//	0x14	00010100
				set_ts_path(TUNER_1_CAM_A_CAM_B_VIEW);			//	0x23	00100011
			}
		} else
		if ((!(aPresent)) && (!(bPresent)))
		{
			set_cam_path(TUNER_1_VIEW);					//	0x00	00000000
			set_ts_path(TUNER_1_VIEW);					//	0x21	00100001
		}
	} else
	{
		if ((aPresent) && (!bPresent))
		{
			set_cam_path(TUNER_2_CAM_A);					//	0x12	00010010
			set_ts_path(TUNER_2_CAM_A);					//	0x31	00110001
		} else
		if ((!aPresent) && (bPresent))
		{
			set_cam_path(TUNER_2_CAM_B);					//	0x20	00100000
			set_ts_path(TUNER_2_CAM_B);					//	0x41	01000001
		} else
		if ((aPresent) && (bPresent))
		{
			if (slot == 0) {
				if (ci_state.module_source[1] == 0) {			//	Cam B and Tuner 1 in use
					set_cam_path(TUNER_1_VIEW_CAM_A);		//	0x12	00010010 T1_CAM_B_T2_CAM_A
					set_ts_path(TUNER_1_VIEW_CAM_A);		//	0x34	00110100 T1_CAM_B_T2_CAM_A
				} else {
					set_cam_path(TUNER_2_CAM_A_B);			//	0x24	00100100
					set_ts_path(TUNER_2_CAM_A);			//	0x31	00110001
				}
			} else {
				set_cam_path(TUNER_1_CAM_A_TUNER_2_CAM_B);		//	0x21	00100001 T1_CAM_A_T2_CAM_B
				set_ts_path(TUNER_1_CAM_A_TUNER_2_CAM_B);		//	0x43	01000011 T1_CAM_A_T2_CAM_B

			}
		} else
		if ((!aPresent) && (!bPresent))
		{
/* ??? */
			set_ts_path(TUNER_2_VIEW);					//	0x12	00010010
		}
	}
#endif
	return 0;
}

void getCiSource(int slot, int* source)
{
#if defined(UFS922) || defined(UFS913) || defined(UFC960)
	*source = ci_state.module_source[slot];
#else
	*source = 0;
#endif
}

#if defined(UFS922) || defined(UFS913) || defined(UFC960)
void set_cam_path(int route)
{
	struct ufs9xx_cic_state *state = &ci_state;

	switch (route)
	{
		case TUNER_1_VIEW:
			dprintk(1,"%s: TUNER_1_VIEW\n", __func__);
			ufs9xx_cic_writereg(state, 0x02, 0x00);
		break;
		case TUNER_1_CAM_A_VIEW:
			dprintk(1,"%s: TUNER_1_CAM_A_VIEW\n", __func__);
			ufs9xx_cic_writereg(state, 0x02, 0x01);
		break;
		case TUNER_1_CAM_B_VIEW:
			dprintk(1,"%s: TUNER_1_CAM_B_VIEW\n", __func__);
			ufs9xx_cic_writereg(state, 0x02, 0x10);
		break;
		case TUNER_1_CAM_A_CAM_B_VIEW:
			dprintk(1,"%s: TUNER_1_CAM_A_CAM_B_VIEW\n", __func__);
//FIXME: maruapp sets first 0x11 and then 0x14 ???
			ufs9xx_cic_writereg(state, 0x02, 0x14);
		break;
		case TUNER_2_CAM_A:
			dprintk(1, "%s: TUNER_2_CAM_A\n", __func__);
			ufs9xx_cic_writereg(state, 0x02, 0x12);
		break;
		case TUNER_2_CAM_B:
			dprintk(1,"%s: TUNER_2_CAM_B\n", __func__);
			ufs9xx_cic_writereg(state, 0x02, 0x20);
		break;
		case TUNER_2_CAM_A_B:
/* fixme maurapp sets first 0x22 */
			dprintk(1,"%s: TUNER_2_CAM_A_B\n", __func__);
			ufs9xx_cic_writereg(state, 0x02, 0x24);
		break;
		case TUNER_1_CAM_A_TUNER_2_CAM_B:
			dprintk(1,"%s: TUNER_1_CAM_A_TUNER_2_CAM_B\n", __func__);
			ufs9xx_cic_writereg(state, 0x02, 0x21);
		break;
		case TUNER_1_VIEW_CAM_A:
			dprintk(1,"%s: TUNER_1_VIEW_CAM_A\n", __func__);
			ufs9xx_cic_writereg(state, 0x02, 0x12);
		break;
		default:
			dprintk(1,"%s: TUNER_1_VIEW\n", __func__);
			ufs9xx_cic_writereg(state, 0x02, 0x00);
		break;
	}
}


void set_ts_path(int route)
{
	struct ufs9xx_cic_state *state = &ci_state;

	switch (route)
	{
		case TUNER_1_VIEW:
			dprintk(1,"%s: TUNER_1_VIEW\n", __func__);
			ufs9xx_cic_writereg(state, 0x01, 0x21);
		break;
		case TUNER_1_CAM_A_VIEW:
			dprintk(1,"%s: TUNER_1_CAM_A_VIEW\n", __func__);
			ufs9xx_cic_writereg(state, 0x01, 0x23);
		break;
		case TUNER_1_CAM_B_VIEW:
			dprintk(1,"%s: TUNER_1_CAM_B_VIEW\n", __func__);
			ufs9xx_cic_writereg(state, 0x01, 0x24);
		break;
		case TUNER_1_CAM_A_CAM_B_VIEW:
			ufs9xx_cic_writereg(state, 0x01, 0x23);
			dprintk(1,"%s: TUNER_1_CAM_A_CAM_B_VIEW\n", __func__);
		break;
		case TUNER_2_VIEW:
/* fixme: maruapp sets sometimes 0x11 before */
			ufs9xx_cic_writereg(state, 0x01, 0x12);
			dprintk(1,"%s: TUNER_2_VIEW\n", __func__);
		break;
		case TUNER_2_CAM_A:
			ufs9xx_cic_writereg(state, 0x01, 0x31);
			dprintk(1,"%s: TUNER_2_CAM_A\n", __func__);
		break;
		case TUNER_2_CAM_B:
			ufs9xx_cic_writereg(state, 0x01, 0x41);
			dprintk(1, "%s: TUNER_2_CAM_B\n", __func__);
		break;
		case TUNER_1_CAM_A_TUNER_2_CAM_B:
			ufs9xx_cic_writereg(state, 0x01, 0x43);
			dprintk(1,"%s: TUNER_1_CAM_A_TUNER_2_CAM_B\n", __func__);
		break;
		case TUNER_1_VIEW_CAM_A:
			ufs9xx_cic_writereg(state, 0x01, 0x34);
			dprintk(1,"%s: TUNER_1_VIEW_CAM_A\n", __func__);
		break;
		default:
			dprintk(1,"%s: TUNER_1_VIEW\n", __func__);
			ufs9xx_cic_writereg(state, 0x01, 0x21);
		break;
	}
}
#endif

#if defined(UFS912)
int cic_init_hw(void)
{
	struct ufs9xx_cic_state *state = &ci_state;
	
	ufs9xx_write_register_u32_map(EMIConfigBaseAddress + EMI_LCK, 0x00);
	ufs9xx_write_register_u32_map(EMIConfigBaseAddress + EMI_GEN_CFG, 0x18);

	ufs9xx_write_register_u32_map(EMIConfigBaseAddress + EMIBank2 + EMI_CFG_DATA0, 0x04f446d9);
	ufs9xx_write_register_u32_map(EMIConfigBaseAddress + EMIBank2 + EMI_CFG_DATA1, 0xfd44ffff);
	ufs9xx_write_register_u32_map(EMIConfigBaseAddress + EMIBank2 + EMI_CFG_DATA2, 0xfd88ffff);
	ufs9xx_write_register_u32_map(EMIConfigBaseAddress + EMIBank2 + EMI_CFG_DATA3, 0x00000000);
	ufs9xx_write_register_u32_map(EMIConfigBaseAddress + EMIBank3 + EMI_CFG_DATA0, 0x04f446d9);
	ufs9xx_write_register_u32_map(EMIConfigBaseAddress + EMIBank3 + EMI_CFG_DATA1, 0xfd44ffff);
	ufs9xx_write_register_u32_map(EMIConfigBaseAddress + EMIBank3 + EMI_CFG_DATA2, 0xfd88ffff);
	ufs9xx_write_register_u32_map(EMIConfigBaseAddress + EMIBank3 + EMI_CFG_DATA3, 0x00000000);

	ufs9xx_write_register_u32_map(EMIConfigBaseAddress + EMI_LCK, 0x1f);

	state->module_status[0] = SLOTSTATUS_NONE;
	state->module_status[1] = SLOTSTATUS_NONE;

	/* will be set to one if a module is present in slot a/b.
	 */
	state->module_ready_pin[0] = state->slot_status[0] = stpio_request_pin (6, 0, "SLOT_A_STA", STPIO_IN);
	state->module_ready_pin[1] = state->slot_status[1] = stpio_request_pin (6, 1, "SLOT_B_STA", STPIO_IN);

	/* these one will set and then cleared if a module is presented
	 * or for reset purpose. in our case its ok todo this only 
	 * in reset function because it will be called after a module
	 * is inserted (in e2 case, if other applications does not do
	 * this we must set and clear it also in the poll function).
	 */
/* fixme: not sure here */
	state->slot_reset[0] = stpio_request_pin (6, 2, "SLOT_A", STPIO_OUT);
	state->slot_reset[1] = stpio_request_pin (6, 3, "SLOT_B", STPIO_OUT);

	/* must be cleared when a module is present and vice versa
	 */
	state->slot_enable[0] = stpio_request_pin (6, 4, "SLOT_A_EN", STPIO_OUT);
	state->slot_enable[1] = stpio_request_pin (6, 5, "SLOT_B_EN", STPIO_OUT);
	
	stpio_set_pin(state->slot_enable[0], 0);
	mdelay(50);
	stpio_set_pin(state->slot_enable[0], 1);
	
	stpio_set_pin(state->slot_enable[1], 0);
	mdelay(50);
	stpio_set_pin(state->slot_enable[1], 1);

	return 0;
}
#elif defined(UFS913)
int cic_init_hw(void)
{
	struct ufs9xx_cic_state *state = &ci_state;

	ufs9xx_write_register_u32_map(EMIConfigBaseAddress + EMI_LCK, 0x00);

	ufs9xx_write_register_u32_map(EMIConfigBaseAddress + EMIBank2 + EMI_CFG_DATA0, 0x048637f9);
	ufs9xx_write_register_u32_map(EMIConfigBaseAddress + EMIBank2 + EMI_CFG_DATA1, 0xbc66f9f9);
	ufs9xx_write_register_u32_map(EMIConfigBaseAddress + EMIBank2 + EMI_CFG_DATA2, 0xbc66f9f9);
	ufs9xx_write_register_u32_map(EMIConfigBaseAddress + EMIBank2 + EMI_CFG_DATA3, 0x00000000);

	ufs9xx_write_register_u32_map(EMIConfigBaseAddress + EMIBank3 + EMI_CFG_DATA0, 0x048637f9);
	ufs9xx_write_register_u32_map(EMIConfigBaseAddress + EMIBank3 + EMI_CFG_DATA1, 0xbc66f9f9);
	ufs9xx_write_register_u32_map(EMIConfigBaseAddress + EMIBank3 + EMI_CFG_DATA2, 0xbc66f9f9);
	ufs9xx_write_register_u32_map(EMIConfigBaseAddress + EMIBank3 + EMI_CFG_DATA3, 0x00000000);

	ufs9xx_write_register_u32_map(EMIConfigBaseAddress + EMI_LCK, 0x1f);
	ufs9xx_write_register_u32_map(EMIConfigBaseAddress + EMI_GEN_CFG, 0x00000019);

	state->module_status[0] = SLOTSTATUS_NONE;
	state->module_status[1] = SLOTSTATUS_NONE;

	state->ci_reset = stpio_request_pin (1, 6, "CI_RESET", STPIO_OUT);

	/* necessary for accessing i2c-1 */
	ufs9xx_write_register_u32_map(0xfe000010, 0x0000c0de);
	ufs9xx_write_register_u32_map(0xfe0000b4, 0x00000008);
	ufs9xx_write_register_u32_map(0xfe000010, 0x0000c1a0);

	ufs9xx_write_register_u32_map(0xfe001160, 0x0000001c);

	stpio_set_pin(state->ci_reset, 0);
	msleep(250);
	stpio_set_pin(state->ci_reset, 1);
	
	ufs9xx_cic_writereg(state, 0x00, 0x23);

	/* will be set to one if a module is present in slot a/b.
	 */
	state->slot_status[0] = stpio_request_pin (11, 4, "SLOT_A_STA", STPIO_IN);
	state->slot_status[1] = stpio_request_pin (11, 5, "SLOT_B_STA", STPIO_IN);

	dprintk(1,"status %p, %p\n", state->slot_status[0], state->slot_status[1]);

	state->module_ready_pin[0] = stpio_request_pin (5, 5, "MOD_A_RDY", STPIO_IN);
	state->module_ready_pin[1] = stpio_request_pin (5, 4, "MOD_B_RDY", STPIO_IN);

	state->slot_reset[0] = stpio_request_pin (11, 6, "SLOT_A", STPIO_OUT);
	state->slot_reset[1] = stpio_request_pin (11, 7, "SLOT_B", STPIO_OUT);

	dprintk(1,"reset %p, %p\n", state->slot_reset[0], state->slot_reset[1]);

	/* must be cleared when a module is present and vice versa
	 */
	state->slot_enable[0] = stpio_request_pin (11, 0, "SLOT_A_EN", STPIO_OUT);
	state->slot_enable[1] = stpio_request_pin (11, 2, "SLOT_B_EN", STPIO_OUT);
	
	dprintk(1, "enable %p, %p\n", state->slot_enable[0], state->slot_enable[1]);

	stpio_set_pin(state->slot_enable[0], 0);
	stpio_set_pin(state->slot_enable[1], 0);

	return 0;
}
#elif defined(UFS922) || defined(UFC960)
int cic_init_hw(void)
{
	struct stpio_pin* enable_pin;
	struct ufs9xx_cic_state *state = &ci_state;

	ufs9xx_write_register_u32_map(0x19213000, 0x0000c0de);
	ufs9xx_write_register_u32_map(0x19213038, 0x0000000b);
	ufs9xx_write_register_u32_map(0x19213088, 0x00000001);

	ufs9xx_write_register_u32_map(EMIConfigBaseAddress + EMI_LCK, 0x0);

	ufs9xx_write_register_u32_map(EMIConfigBaseAddress + EMI_GEN_CFG, 0x00000018);
	ufs9xx_write_register_u32_map(EMIConfigBaseAddress + EMI_FLASH_CLK_SEL, 0x2);
	ufs9xx_write_register_u32_map(EMIConfigBaseAddress + EMI_CLK_EN, 0x1);

	ufs9xx_write_register_u32_map(EMIConfigBaseAddress + EMIBank2 + EMI_CFG_DATA0, 0x04f446d9);
	ufs9xx_write_register_u32_map(EMIConfigBaseAddress + EMIBank2 + EMI_CFG_DATA1, 0xfd44ffff);
	ufs9xx_write_register_u32_map(EMIConfigBaseAddress + EMIBank2 + EMI_CFG_DATA2, 0xfd88ffff);
	ufs9xx_write_register_u32_map(EMIConfigBaseAddress + EMIBank2 + EMI_CFG_DATA3, 0x00000000);

	ufs9xx_write_register_u32_map(EMIConfigBaseAddress + EMIBank3 + EMI_CFG_DATA0, 0x04f446d9);
	ufs9xx_write_register_u32_map(EMIConfigBaseAddress + EMIBank3 + EMI_CFG_DATA1, 0xfd44ffff);
	ufs9xx_write_register_u32_map(EMIConfigBaseAddress + EMIBank3 + EMI_CFG_DATA2, 0xfd88ffff);
	ufs9xx_write_register_u32_map(EMIConfigBaseAddress + EMIBank3 + EMI_CFG_DATA3, 0x00000000);

	ufs9xx_write_register_u32_map(EMIConfigBaseAddress + EMIBank4 + EMI_CFG_DATA0, 0x04f47ed1);
	ufs9xx_write_register_u32_map(EMIConfigBaseAddress + EMIBank4 + EMI_CFG_DATA1, 0x9e113f3f);
	ufs9xx_write_register_u32_map(EMIConfigBaseAddress + EMIBank4 + EMI_CFG_DATA2, 0x98339999);
	ufs9xx_write_register_u32_map(EMIConfigBaseAddress + EMIBank4 + EMI_CFG_DATA3, 0x00000000);

	ufs9xx_write_register_u32_map(EMIConfigBaseAddress + EMI_LCK, 0x1F);

	state->module_status[0] = SLOTSTATUS_NONE;
	state->module_status[1] = SLOTSTATUS_NONE;

	/* enables the ci controller itself */	
	state->ci_reset = enable_pin = stpio_request_pin (3, 4, "CI_enable", STPIO_OUT);

	state->module_ready_pin[0] = stpio_request_pin (0, 1, "MOD_A_RDY", STPIO_IN);
	state->module_ready_pin[1] = stpio_request_pin (0, 2, "MOD_B_RDY", STPIO_IN);

	/* will be set to one if a module is present in slot a/b.
	 */
	state->slot_status[0] = stpio_request_pin (0, 0, "SLOT_A_STA", STPIO_IN);
	state->slot_status[1] = stpio_request_pin (2, 6, "SLOT_B_STA", STPIO_IN);

	/* these one will set and then cleared if a module is presented
	 * or for reset purpose. in our case its ok todo this only 
	 * in reset function because it will be called after a module
	 * is inserted (in e2 case, if other applications does not do
	 * this we must set and clear it also in the poll function).
	 */
	state->slot_reset[0] = stpio_request_pin (5, 4, "SLOT_A", STPIO_OUT);
	state->slot_reset[1] = stpio_request_pin (5, 5, "SLOT_B", STPIO_OUT);

	/* must be cleared when a module is present and vice versa
	 * ->setting this bit during runtime gives output from maruapp
	 * isBusy ...
	 */
	state->slot_enable[0] = stpio_request_pin (5, 2, "SLOT_A_EN", STPIO_OUT);
	state->slot_enable[1] = stpio_request_pin (5, 3, "SLOT_B_EN", STPIO_OUT);
	
	//reset fpga charon
	dprintk(1, "reset fpga charon\n");
	
	stpio_set_pin(enable_pin, 0);
	mdelay(50); //necessary?
	stpio_set_pin(enable_pin, 1);

	set_ts_path(TUNER_1_VIEW);
	set_cam_path(TUNER_1_VIEW);

	return 0;
}
#endif

int init_ci_controller(struct dvb_adapter* dvb_adap)
{
	struct ufs9xx_cic_state *state = &ci_state;
	struct ufs9xx_cic_core *core = &ci_core;
	int result;

	dprintk(1,"init_ufs9xx_cic >\n");

	core->dvb_adap = dvb_adap;

#if defined(UFS922) || defined(UFC960)
	state->i2c = i2c_get_adapter(2);
	state->i2c_addr = 0x23;

	state->slot_attribute_read[0]   = (volatile unsigned long) ioremap_nocache(0x02828000, 0x200);
	state->slot_attribute_write[0]  = (volatile unsigned long) ioremap_nocache(0x02808000, 0x200);
	state->slot_control_read[0]     = (volatile unsigned long) ioremap_nocache(0x02820000, 0x200);
	state->slot_control_write[0]    = (volatile unsigned long) ioremap_nocache(0x02800000, 0x200);

	state->slot_attribute_read[1]   = (volatile unsigned long) ioremap_nocache(0x02028000, 0x200);
	state->slot_attribute_write[1]  = (volatile unsigned long) ioremap_nocache(0x02008000, 0x200);
	state->slot_control_read[1]     = (volatile unsigned long) ioremap_nocache(0x02020000, 0x200);
	state->slot_control_write[1]    = (volatile unsigned long) ioremap_nocache(0x02000000, 0x200);

#elif defined(UFS913)
	state->slot_attribute_read[0]   = (volatile unsigned long) ioremap_nocache(0x04828000, 0x200);
	state->slot_attribute_write[0]  = (volatile unsigned long) ioremap_nocache(0x04808000, 0x200);
	state->slot_control_read[0]     = (volatile unsigned long) ioremap_nocache(0x04820000, 0x200);
	state->slot_control_write[0]    = (volatile unsigned long) ioremap_nocache(0x04800000, 0x200);

	state->slot_attribute_read[1]   = (volatile unsigned long) ioremap_nocache(0x05028000, 0x200);
	state->slot_attribute_write[1]  = (volatile unsigned long) ioremap_nocache(0x05008000, 0x200);
	state->slot_control_read[1]     = (volatile unsigned long) ioremap_nocache(0x05020000, 0x200);
	state->slot_control_write[1]    = (volatile unsigned long) ioremap_nocache(0x05000000, 0x200);

	ufs9xx_write_register_u32_map(0xfe000010, 0x0000c0de);
	ufs9xx_write_register_u32_map(0xfe000088, 0x00000000);
	ufs9xx_write_register_u32_map(0xfe000080, 0x00000019);
	ufs9xx_write_register_u32_map(0xfe000084, 0x00003334);
	ufs9xx_write_register_u32_map(0xfe00008c, 0x00000000);
	ufs9xx_write_register_u32_map(0xfe000088, 0x00000001);

	ufs9xx_write_register_u8(state->slot_attribute_write[0] + 0x10c, 0x01);
	ufs9xx_write_register_u8(state->slot_attribute_write[0] + 0x1b0, 0x53);
	ufs9xx_write_register_u8(state->slot_attribute_write[0] + 0x1b2, 0x54);
	ufs9xx_write_register_u8(state->slot_attribute_write[0] + 0x1b4, 0x41);
	ufs9xx_write_register_u8(state->slot_attribute_write[0] + 0x1b6, 0x50);
	ufs9xx_write_register_u8(state->slot_attribute_write[0] + 0x1b8, 0x49);
	ufs9xx_write_register_u8(state->slot_attribute_write[0] + 0x170, 0x53);
	ufs9xx_write_register_u8(state->slot_attribute_write[0] + 0x172, 0x44);
	ufs9xx_write_register_u8(state->slot_attribute_write[0] + 0x174, 0x4b);

	state->i2c = i2c_get_adapter(1);
	state->i2c_addr = 0x23;
#elif defined(UFS912)
	state->slot_attribute_read[0]   = (volatile unsigned long) ioremap_nocache(0x03028000, 0x200);
	state->slot_attribute_write[0]  = (volatile unsigned long) ioremap_nocache(0x03008000, 0x200);
	state->slot_control_read[0]     = (volatile unsigned long) ioremap_nocache(0x03020000, 0x200);
	state->slot_control_write[0]     = (volatile unsigned long) ioremap_nocache(0x03000000, 0x200);

	state->slot_attribute_read[1]   = (volatile unsigned long) ioremap_nocache(0x04028000, 0x200);
	state->slot_attribute_write[1]  = (volatile unsigned long) ioremap_nocache(0x04008000, 0x200);
	state->slot_control_read[1]     = (volatile unsigned long) ioremap_nocache(0x04020000, 0x200);
	state->slot_control_write[1]     = (volatile unsigned long) ioremap_nocache(0x04000000, 0x200);
#endif

	memset(&core->ca, 0, sizeof(struct dvb_ca_en50221));

	/* register CI interface */
	core->ca.owner = THIS_MODULE;

	core->ca.read_attribute_mem  = ufs9xx_cic_read_attribute_mem;
	core->ca.write_attribute_mem = ufs9xx_cic_write_attribute_mem;
	core->ca.read_cam_control 	 = ufs9xx_cic_read_cam_control;
	core->ca.write_cam_control 	 = ufs9xx_cic_write_cam_control;
	core->ca.slot_shutdown 		 = ufs9xx_cic_slot_shutdown;
	core->ca.slot_ts_enable 	 = ufs9xx_cic_slot_ts_enable;

	core->ca.slot_reset 		 = ufs9xx_cic_slot_reset;
	core->ca.poll_slot_status 	 = ufs9xx_cic_poll_slot_status;

	state->core 			     = core;
	core->ca.data 			     = state;

	cic_init_hw();
	
	dprintk(1, "init_ufs9xx_cic: call dvb_ca_en50221_init\n");

	if ((result = dvb_ca_en50221_init(core->dvb_adap,
					  &core->ca, 0, 2)) != 0) {
		printk(KERN_ERR "ca0 initialisation failed.\n");
		goto error;
	}

	dprintk(1, "ufs9xx_cic: ca0 interface initialised.\n");

	dprintk(10, "init_ufs9xx_cic <\n");

	return 0;

error:

	printk("init_ufs9xx_cic < error\n");

	return result;
}

EXPORT_SYMBOL(init_ci_controller);
EXPORT_SYMBOL(setCiSource);
EXPORT_SYMBOL(getCiSource);

int __init ufs9xx_cic_init(void)
{
	dprintk(1, "ufs9xx_cic loaded\n");
	return 0;
}

static void __exit ufs9xx_cic_exit(void)
{  
	dprintk(1,"ufs9xx_cic unloaded\n");
}

module_init             (ufs9xx_cic_init);
module_exit             (ufs9xx_cic_exit);

MODULE_DESCRIPTION      ("CI Controller");
MODULE_AUTHOR           ("Dagobert");
MODULE_LICENSE          ("GPL");

module_param(paramDebug, int, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
MODULE_PARM_DESC(paramDebug, "Debug Output 0=disabled >0=enabled(debuglevel)");

module_param(waitMS, int, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
MODULE_PARM_DESC(waitMS, "waiting time between pio settings for reset/enable purpos in milliseconds (default=200)");
