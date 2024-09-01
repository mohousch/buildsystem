
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/types.h>

#include "dvb_frontend.h"
#include "sharp6465.h"

extern int debug_fe7162;
#define _DEBUG if (debug_fe7162)

struct sharp6465_state
{
	struct dvb_frontend *fe;
	struct i2c_adapter *i2c;
	const struct sharp6465_config *config;

	u32 frequency;
	u32 bandwidth;
};

static long calculate_mop_xtal(void);
static void calculate_mop_ic(u32 freq, u32 baud, int *byte); //[kHz]
static void calculate_mop_divider(u32 freq, int *byte);
static void calculate_mop_uv_cp(u32 freq, int *cp, int *uv);
//static long calculate_mop_if(void);
static long calculate_mop_step(int *byte);
static void calculate_mop_bw(u32 baud, int *byte);

static int sharp6465_read(struct sharp6465_state *state, u8 *buf)
{
	const struct sharp6465_config *config = state->config;
	int err = 0;
	struct i2c_msg msg = { .addr = config->addr, .flags = I2C_M_RD, .buf = buf, .len = 2 };
	err = i2c_transfer(state->i2c, &msg, 1);
	if (err != 1)
		goto exit;
	return err;
exit:
	printk(KERN_ERR "%s: I/O Error err=<%d>\n", __func__, err);
	return err;
}

static int sharp6465_write(struct sharp6465_state *state, u8 *buf, u8 length)
{
	const struct sharp6465_config *config = state->config;
	int err = 0;
	struct i2c_msg msg = { .addr = config->addr, .flags = 0, .buf = buf, .len = length };
	_DEBUG
	printk(KERN_ERR "%s: state->i2c=<%d>, config->addr = %d\n",
		   __func__, (int)state->i2c, config->addr);
	err = i2c_transfer(state->i2c, &msg, 1);
	if (err != 1)
		goto exit;
	return err;
exit:
	printk(KERN_ERR "%s: I/O Error err=<%d>\n", __func__, err);
	return err;
}

static int sharp6465_get_status(struct dvb_frontend *fe, u32 *status)
{
	struct sharp6465_state *state = fe->tuner_priv;
	u8 result[2] = {0};
	int err = 0;
	*status = 0;
	err = sharp6465_read(state, result);
	if (err < 0)
		goto exit;
	if (result[0] & 0x40)
	{
		_DEBUG
		printk(KERN_DEBUG "%s: Tuner Phase Locked\n", __func__);
		*status = 1;
	}
	return err;
exit:
	printk(KERN_ERR "%s: I/O Error\n", __func__);
	return err;
}

static int sharp6465_get_identify(struct dvb_frontend *fe)
{
	struct sharp6465_state *state = fe->tuner_priv;
	u8 result[2] = {0, 0};
	int err = 0;
	err = sharp6465_read(state, result);
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

#if 1
static long calculate_mop_xtal()
{
	return 4000;//khz
}

//----------------------------------------------------------------

static void calculate_mop_ic(u32 freq, u32 baud, int *byte) //[kHz]
{
	calculate_mop_divider(freq, byte);
	{
		int cp = 0, uv = 0;
		calculate_mop_uv_cp(freq, &cp, &uv);
		*(byte + 4) &= 0x38;
		*(byte + 4) |= uv;
		*(byte + 4) |= (cp << 6);
		calculate_mop_bw(baud, byte);
	}
}

u64 __udivdi3(u64 n, u64 d);

static void calculate_mop_divider(u32 freq, int *byte)
{
	long data;
	u64 i64Freq;
	i64Freq = (u64)freq * 100000;
	i64Freq += (u64)3616666700;
	i64Freq = __udivdi3(i64Freq, calculate_mop_step(byte));
	i64Freq += 5;
	i64Freq /= 10;
	data = (long)i64Freq;
	_DEBUG
	printk(KERN_ERR "%s: data = %ld\n", __func__, data);
	//data = (long)((freq + calculate_mop_if())/calculate_mop_step(byte) + 0.5);
	*(byte + 1) = (int)((data >> 8) & 0x7F); //byte2
	*(byte + 2) = (int)(data & 0xFF); //byte3
}

static void calculate_mop_uv_cp(u32 freq, int *cp, int *uv)
{
	int i;
	int cp_value = 599;
	int *CP_DATA = kzalloc(sizeof(int) * 601, GFP_KERNEL);
	/*charge pump lib*/
	for (i = 0; i <= 600; i++)
		CP_DATA[i] = 0;
	CP_DATA[350] = 2;
	CP_DATA[600] = 3;
	if (freq >= 147000 && freq < 430000)
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
	kfree(CP_DATA);
}

#if 0
static long calculate_mop_if()
{
	long if_freq;
	if_freq = (long)36166667 / 1000;
	return if_freq;
}
#endif /* 0 */

static long calculate_mop_step(int *byte)
{
	int byte4;
	long mop_step_ratio, mop_freq_step;
	int R210 = 0;
	byte4 = byte[3];
	R210 = (byte4 & 0x07);
	//if(R210==0)
	mop_step_ratio = 24;
	//else if(R210==1) mop_step_ratio = 28.;
	//else if(R210==2) mop_step_ratio = 50.;
	//else if(R210==3) mop_step_ratio = 64.;
	//else if(R210==4) mop_step_ratio = 128.;
	//else if(R210==5) mop_step_ratio = 80. ;
	mop_freq_step = ((long)(calculate_mop_xtal() * 10000) / mop_step_ratio + 5); //kHz
	return mop_freq_step ;
}

static void calculate_mop_bw(u32 baud, int *byte)
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

static void tuner_SHARP6465_CalWrBuffer(u32 Frequency,
										u32 BandWidth,
										unsigned char *pcIOBuffer)
{
	int buffer[10];
	memset(buffer, 0, sizeof(buffer));
	calculate_mop_ic(Frequency, BandWidth * 1000, buffer);
	*pcIOBuffer = (unsigned char)buffer[1];
	*(pcIOBuffer + 1) = (unsigned char)buffer[2];
	*(pcIOBuffer + 2) = 0x80; // (unsigned char)buffer[3];
	*(pcIOBuffer + 3) = (unsigned char)buffer[4];
	*(pcIOBuffer + 4) = 0xE1; //set 5 byte
}
#endif

static int sharp6465_set_params(struct dvb_frontend *fe,
								struct dvb_frontend_parameters *params)
{
	struct sharp6465_state *state = fe->tuner_priv;
	unsigned char ucIOBuffer[6];
	int err = 0;
	u32 status = 0;
	u32 f = params->frequency;
	struct dvb_ofdm_parameters *op = &params->u.ofdm;
	_DEBUG
	printk(KERN_ERR "%s: f = %d, bandwidth = %d\n", __func__, f, op->bandwidth);
	tuner_SHARP6465_CalWrBuffer(f / 1000,
								8 - op->bandwidth - BANDWIDTH_8_MHZ,
								ucIOBuffer);
	/*open i2c repeater gate*/
	if (fe->ops.i2c_gate_ctrl(fe, 1) < 0)
		goto exit;
	/* Set params */
	err = sharp6465_write(state, ucIOBuffer, 4);
	if (err < 0)
		goto exit;
	if (fe->ops.i2c_gate_ctrl(fe, 0) < 0)
		goto exit;
	ucIOBuffer[2] = ucIOBuffer[4];
	if (fe->ops.i2c_gate_ctrl(fe, 1) < 0)
		goto exit;
	err = sharp6465_write(state, ucIOBuffer, 4);
	if (err < 0)
		goto exit;
	if (fe->ops.i2c_gate_ctrl(fe, 0) < 0)
		goto exit;
	//msleep(1000);
	if (fe->ops.i2c_gate_ctrl(fe, 1) < 0)
		goto exit;
	sharp6465_get_status(fe, &status);
	_DEBUG
	printk(KERN_ERR "%s: status = %d\n", __func__, status);
	return 0;
exit:
	printk(KERN_ERR "%s: I/O Error\n", __func__);
	return err;
}

#if 0
static int sharp6465_set_state(struct dvb_frontend *fe,
							   enum tuner_param param,
							   struct tuner_state *tstate)
{
	struct sharp6465_state *state = fe->tuner_priv;
	const struct sharp6465_config *config = state->config;
	u32 frequency, status = 0;
	u8 buf[4];
	int err = 0;
	if (param & DVBFE_TUNER_FREQUENCY)
	{
		unsigned char ucIOBuffer[6];
		tuner_SHARP6465_CalWrBuffer(tstate->frequency,
									tstate->bandwidth,
									ucIOBuffer);
		/* Set params */
		err = sharp6465_write(state, buf, 4);
		if (err < 0)
			goto exit;
		ucIOBuffer[2] = ucIOBuffer[4];
		err = sharp6465_write(state, buf, 4);
		if (err < 0)
			goto exit;
		/* sleep for some time */
		printk(KERN_DEBUG "%s: Waiting to Phase LOCK\n", __func__);
		msleep(20);
		/* check status */
		err = sharp6465_get_status(fe, &status);
		if (err < 0)
			goto exit;
		if (status == 1)
		{
			printk(KERN_DEBUG "%s: Tuner Phase locked: status=%d\n", __func__, status);
			state->frequency = frequency; /* cache successful state */
		}
		else
		{
			printk(KERN_ERR "%s: No Phase lock: status=%d\n", __func__, status);
		}
	}
	else
	{
		printk(KERN_ERR "%s: Unknown parameter (param=%d)\n", __func__, param);
		return -EINVAL;
	}
	return 0;
exit:
	printk(KERN_ERR "%s: I/O Error\n", __func__);
	return err;
}
#endif

static int sharp6465_release(struct dvb_frontend *fe)
{
	struct sharp6465_state *state = fe->tuner_priv;
	fe->tuner_priv = NULL;
	kfree(state);
	return 0;
}

static struct dvb_tuner_ops sharp6465_ops =
{
	.set_params = sharp6465_set_params,
	.release = sharp6465_release,
};

static int sharp6465_check_identify(struct dvb_frontend *fe)
{
	int err = 0;
	u32 status = 0;
	unsigned char ucIOBuffer[10];
	struct sharp6465_state *state = fe->tuner_priv;
	fe->ops.init(fe);
	if (fe->ops.i2c_gate_ctrl(fe, 1) < 0)
		goto exit;
	tuner_SHARP6465_CalWrBuffer(474000, 8, ucIOBuffer);
	if (fe->ops.i2c_gate_ctrl(fe, 1) < 0)
		goto exit;
	err = sharp6465_write(state, ucIOBuffer, 4);
	if (fe->ops.i2c_gate_ctrl(fe, 0) < 0)
		goto exit;
	msleep(500);
	if (fe->ops.i2c_gate_ctrl(fe, 1) < 0)
		goto exit;
	ucIOBuffer[2] = ucIOBuffer[4];
	err = sharp6465_write(state, ucIOBuffer, 4);
	if (fe->ops.i2c_gate_ctrl(fe, 0) < 0)
		goto exit;
	msleep(500);
	if (fe->ops.i2c_gate_ctrl(fe, 1) < 0)
		goto exit;
	err = sharp6465_get_identify(fe);
	printk("status = %d\n", status);
	if (err < 0)
		goto exit;
	if (fe->ops.i2c_gate_ctrl(fe, 0) < 0)
		goto exit;
	return err;
exit:
	return -1;
}

struct dvb_frontend *sharp6465_attach(struct dvb_frontend *fe,
									  const struct sharp6465_config *config,
									  struct i2c_adapter *i2c)
{
	int err = 0;
	struct sharp6465_state *state = NULL;
	struct dvb_tuner_info *info;
	state = kzalloc(sizeof(struct sharp6465_state), GFP_KERNEL);
	if (state == NULL)
		goto exit;
	state->config = config;
	state->i2c = i2c;
	state->fe = fe;
	fe->tuner_priv = state;
	fe->ops.tuner_ops = sharp6465_ops;
	info = &fe->ops.tuner_ops.info;
	memcpy(info->name, config->name, sizeof(config->name));
	err = sharp6465_check_identify(fe);
	if (err < 0)
		goto exit;
	printk("%s: Attaching sharp6465 (%s) tuner\n", __func__, info->name);
	return fe;
exit:
	memset(&fe->ops.tuner_ops, 0, sizeof(struct dvb_tuner_ops));
	kfree(state);
	return NULL;
}
EXPORT_SYMBOL(sharp6465_attach);
