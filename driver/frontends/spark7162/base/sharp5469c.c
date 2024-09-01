
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/types.h>

#include "dvb_frontend.h"
#include "sharp5469c.h"

extern int debug_fe7162;
#define _DEBUG if (debug_fe7162)

struct sharp5469c_state
{
	struct dvb_frontend *fe;
	struct i2c_adapter *i2c;
	const struct sharp5469c_config *config;

	u32 frequency;
	u32 bandwidth;
};

static long sharp5469c_calculate_mop_xtal(void);
static void sharp5469c_calculate_mop_ic(u32 freq, u32 baud, int *byte); //[kHz]
static void sharp5469c_calculate_mop_divider(u32 freq, int *byte);
static void sharp5469c_calculate_mop_uv_cp(u32 freq, int *cp, int *uv);
//static long sharp5469c_calculate_mop_if(void);
static long sharp5469c_calculate_mop_step(int *byte);
static void sharp5469c_calculate_mop_bw(u32 baud, int *byte);

static int sharp5469c_read(struct sharp5469c_state *state, u8 *buf)
{
	const struct sharp5469c_config *config = state->config;
	int err = 0;
	struct i2c_msg msg = { .addr = config->addr, .flags = I2C_M_RD, .buf = buf, .len = 2 };
	//printk(KERN_ERR "%s: state->i2c=<0x%x>, config->addr = 0x%02x\n",
	// __func__, state->i2c, config->addr);
	err = i2c_transfer(state->i2c, &msg, 1);
	if (err != 1)
		goto exit;
	return err;
exit:
	printk(KERN_ERR "%s: I/O Error err=<%d>\n", __func__, err);
	return err;
}

static int sharp5469c_write(struct sharp5469c_state *state, u8 *buf, u8 length)
{
	const struct sharp5469c_config *config = state->config;
	int err = 0;
	struct i2c_msg msg = { .addr = config->addr, .flags = 0, .buf = buf, .len = length };
	_DEBUG
	{
		int i;
		for (i = 0; i < length; i++)
		{
			printk("%02x ", buf[i]);
		}
		printk("\n");
	}
	err = i2c_transfer(state->i2c, &msg, 1);
	if (err != 1)
		goto exit;
	return err;
exit:
	printk(KERN_ERR "%s: I/O Error err=<%d>\n", __func__, err);
	return err;
}

static int sharp5469c_get_status(struct dvb_frontend *fe, u32 *status)
{
	struct sharp5469c_state *state = fe->tuner_priv;
	u8 result[2] = {0};
	int err = 0;
	*status = 0;
	err = sharp5469c_read(state, result);
	//printk(KERN_ERR "result[0] = %d\n", result[0]);
	if (err < 0)
		goto exit;
	if (result[0] & 0x40)
	{
		_DEBUG
		printk(KERN_DEBUG "%s: Tuner Phase Locked\n", __func__);
		*status = 1;
	}
	return 0;
exit:
	printk(KERN_ERR "%s: I/O Error\n", __func__);
	return err;
}

static long sharp5469c_calculate_mop_xtal()
{
	return 4000;//khz
}

//----------------------------------------------------------------

static void sharp5469c_calculate_mop_ic(u32 freq, u32 baud, int *byte) //[kHz]
{
	sharp5469c_calculate_mop_divider(freq, byte);
	{
		int cp = 0, uv = 0;
		sharp5469c_calculate_mop_uv_cp(freq, &cp, &uv);
		*(byte + 4) &= 0x38;
		*(byte + 4) |= uv;
		*(byte + 4) |= (cp << 6);
		sharp5469c_calculate_mop_bw(baud, byte);
	}
}

u64 __udivdi3(u64 n, u64 d);

static void sharp5469c_calculate_mop_divider(u32 freq, int *byte)
{
	long data;
	u64 i64Freq;
	i64Freq = (u64)freq * 100000;
	i64Freq += (u64)3612500000;
	i64Freq = __udivdi3(i64Freq, sharp5469c_calculate_mop_step(byte));
	i64Freq += 5;
	i64Freq /= 10;
	data = (long)i64Freq;
	_DEBUG
	printk(KERN_ERR "%s: data = %ld\n", __func__, data);
	//data = (long)((freq + sharp5469c_calculate_mop_if())/sharp5469c_calculate_mop_step(byte) +0.5);
	*(byte + 1) = (int)((data >> 8) & 0x7F); //byte2
	*(byte + 2) = (int)(data & 0xFF); //byte3
}

static void sharp5469c_calculate_mop_uv_cp(u32 freq, int *cp, int *uv)
{
	int i;
	int cp_value = 599, CP_DATA[601];
	/*charge pump lib*/
	for (i = 0; i <= 600; i++)
		CP_DATA[i] = 0;
	CP_DATA[350] = 2;
	CP_DATA[600] = 3;
	if (freq >= 51000 && freq <= 147000)//lwj add for 5469 low band
	{
		*uv = 1;
		cp_value = 600;
	}
	else if (freq > 147000 && freq < 430000)
	{
		*uv = 2;
		if (freq < 400000) cp_value = 350;
		else cp_value = 600;
	}
	else if (freq >= 430000)
	{
		*uv = 4;
		if (freq < 763000) cp_value = 350;
		else cp_value = 600;
	}
	*cp = CP_DATA[cp_value];
}

#if 0
static long sharp5469c_calculate_mop_if()
{
	long if_freq;
	//if_freq=(double)36166667/1000.;//ter
	if_freq = (double)36125000 / 1000.; //cab
	return if_freq;
}
#endif /* 0 */

static long sharp5469c_calculate_mop_step(int *byte)
{
	int byte4;
	long mop_step_ratio, mop_freq_step;
	int R210 = 0 ;
	byte4 = byte[3];
	R210 = (byte4 & 0x07) ;
	//if(R210==0)
	mop_step_ratio = 64; //lwj change 24 to 64 T:166.67K,divider ratio is 24; C:62.5K, divider ratio is 64
	//else if(R210==1) mop_step_ratio = 28.;
	//else if(R210==2) mop_step_ratio = 50.;
	//else if(R210==3) mop_step_ratio = 64.;
	//else if(R210==4) mop_step_ratio = 128.;
	//else if(R210==5) mop_step_ratio = 80. ;
	mop_freq_step = ((long)(sharp5469c_calculate_mop_xtal() * 10000) / mop_step_ratio + 5); //kHz
	return mop_freq_step;
}

static void sharp5469c_calculate_mop_bw(u32 baud, int *byte)
{
	if (baud > 7500) //BW=8M
	{
		byte[4] |= 0x10; //BW setting
	}
	else if ((6500 < baud) && (baud <= 7500)) //BW=7M
	{
		byte[4] &= 0xEF; //BW setting
		// byte[4] |= 0x00; //BW setting
	}
	else if (baud <= 6500) //BW=6M
	{
		byte[4] &= 0xEF; //BW setting
		// byte[4] |= 0x00; //BW setting
	}
}

static void tuner_SHARP5469C_CalWrBuffer(u32 Frequency,
										 unsigned char *pcIOBuffer)
{
	int buffer[10];
	u32 BandWidth = 8;
	memset(buffer, 0, sizeof(buffer));
	sharp5469c_calculate_mop_ic(Frequency, BandWidth * 1000, buffer);
	*pcIOBuffer = (unsigned char)buffer[1];
	*(pcIOBuffer + 1) = (unsigned char)buffer[2];
	*(pcIOBuffer + 2) = 0x83; // (unsigned char)buffer[3]; //lwj change 0x80 to 0x83 for cable
	*(pcIOBuffer + 3) = (unsigned char)buffer[4];
	*(pcIOBuffer + 4) = 0xC1; //set 5 byte
}

static int sharp5469c_set_params(struct dvb_frontend *fe,
								 struct dvb_frontend_parameters *params)
{
	struct sharp5469c_state *state = fe->tuner_priv;
	unsigned char ucIOBuffer[6];
	int err = 0;
	u32 status = 0;
	u32 f = params->frequency;
	//struct dvb_ofdm_parameters *op = &params->u.ofdm;
	//printk(KERN_ERR "%s: f = %d, bandwidth = %d\n", __func__, f, op->bandwidth);
	tuner_SHARP5469C_CalWrBuffer(f / 1000, ucIOBuffer);
	/*open i2c repeater gate*/
	if (fe->ops.i2c_gate_ctrl)
	{
		if (fe->ops.i2c_gate_ctrl(fe, 1) < 0)
			goto exit;
	}
	/* Set params */
	err = sharp5469c_write(state, ucIOBuffer, 4);
	if (err < 0)
		goto exit;
	if (fe->ops.i2c_gate_ctrl)
	{
		if (fe->ops.i2c_gate_ctrl(fe, 0) < 0)
			goto exit;
	}
	ucIOBuffer[2] = ucIOBuffer[4];
	if (fe->ops.i2c_gate_ctrl)
	{
		if (fe->ops.i2c_gate_ctrl(fe, 1) < 0)
			goto exit;
	}
	err = sharp5469c_write(state, ucIOBuffer, 4);
	if (err < 0)
		goto exit;
	if (fe->ops.i2c_gate_ctrl)
	{
		if (fe->ops.i2c_gate_ctrl(fe, 0) < 0)
			goto exit;
	}
	//msleep(1000);
	if (fe->ops.i2c_gate_ctrl)
	{
		if (fe->ops.i2c_gate_ctrl(fe, 1) < 0)
			goto exit;
	}
	sharp5469c_get_status(fe, &status);
	_DEBUG
	printk(KERN_ERR "%s: status = %d\n", __func__, status);
	return 0;
exit:
	printk(KERN_ERR "%s: I/O Error\n", __func__);
	return err;
}

static int sharp5469c_release(struct dvb_frontend *fe)
{
	struct sharp5469c_state *state = fe->tuner_priv;
	fe->tuner_priv = NULL;
	kfree(state);
	return 0;
}

static int sharp5469c_status(struct dvb_frontend *fe, u32 *status)
{
	int err = 0;
	if (fe->ops.i2c_gate_ctrl)
	{
		if (fe->ops.i2c_gate_ctrl(fe, 1) < 0)
			goto exit;
	}
	err = sharp5469c_get_status(fe, status);
	if (err < 0)
	{
		if (fe->ops.i2c_gate_ctrl)
		{
			if (fe->ops.i2c_gate_ctrl(fe, 0) < 0)
				goto exit;
		}
		goto exit;
	}
	if (fe->ops.i2c_gate_ctrl)
	{
		if (fe->ops.i2c_gate_ctrl(fe, 0) < 0)
			goto exit;
	}
	return 0;
exit:
	printk(KERN_ERR "%s: sharp5469c_status Error\n", __func__);
	return err;
}

static int sharp5469c_get_identify(struct dvb_frontend *fe)
{
	struct sharp5469c_state *state = fe->tuner_priv;
	u8 result[2] = {0, 0};
	int err = 0;
	err = sharp5469c_read(state, result);
	printk("result[0] = %x\n", result[0]);
	printk("result[1] = %x\n", result[1]);
	if (err < 0)
		goto exit;
	if ((result[0] & 0x70) != 0x70)
	{
		return -1;
	}
	return err;
exit:
	printk(KERN_ERR "%s: I/O Error\n", __func__);
	return err;
}

static struct dvb_tuner_ops sharp5469c_ops =
{
	.set_params = sharp5469c_set_params,
	.release = sharp5469c_release,
	.get_status = sharp5469c_status,
};

static int sharp5469c_check_identify(struct dvb_frontend *fe)
{
	int err = 0;
	u32 status = 0;
	unsigned char ucIOBuffer[10];
	struct sharp5469c_state *state = fe->tuner_priv;
	fe->ops.init(fe);
	if (fe->ops.i2c_gate_ctrl(fe, 1) < 0)
		goto exit;
	tuner_SHARP5469C_CalWrBuffer(474000, ucIOBuffer);
	if (fe->ops.i2c_gate_ctrl(fe, 1) < 0)
		goto exit;
	err = sharp5469c_write(state, ucIOBuffer, 4);
	if (fe->ops.i2c_gate_ctrl(fe, 0) < 0)
		goto exit;
	msleep(500);
	if (fe->ops.i2c_gate_ctrl(fe, 1) < 0)
		goto exit;
	ucIOBuffer[2] = ucIOBuffer[4];
	err = sharp5469c_write(state, ucIOBuffer, 4);
	if (fe->ops.i2c_gate_ctrl(fe, 0) < 0)
		goto exit;
	msleep(500);
	if (fe->ops.i2c_gate_ctrl(fe, 1) < 0)
		goto exit;
	err = sharp5469c_get_identify(fe);
	printk("status = %d\n", status);
	if (err < 0)
		goto exit;
	if (fe->ops.i2c_gate_ctrl(fe, 0) < 0)
		goto exit;
	return err;
exit:
	return -1;
}

struct dvb_frontend *sharp5469c_attach(struct dvb_frontend *fe,
									   const struct sharp5469c_config *config,
									   struct i2c_adapter *i2c)
{
	int err = 0;
	struct sharp5469c_state *state = NULL;
	struct dvb_tuner_info *pInfo;
	state = kzalloc(sizeof(struct sharp5469c_state), GFP_KERNEL);
	if (state == NULL)
		goto exit;
	state->config = config;
	state->i2c = i2c;
	state->fe = fe;
	fe->tuner_priv = state;
	fe->ops.tuner_ops = sharp5469c_ops;
	pInfo = &fe->ops.tuner_ops.info;
	memcpy(pInfo->name, config->name, 128);
	err = sharp5469c_check_identify(fe);
	if (err < 0)
		goto exit;
	printk("%s: Attaching sharp5469c (%s) tuner\n", __func__, pInfo->name);
	return fe;
exit:
	memset(&fe->ops.tuner_ops, 0, sizeof(struct dvb_tuner_ops));
	kfree(state);
	return NULL;
}
EXPORT_SYMBOL(sharp5469c_attach);
