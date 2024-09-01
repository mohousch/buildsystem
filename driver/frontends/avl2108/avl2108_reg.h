/*
 Availink avl2108 - DVBS/S2 Satellite demod driver with Sharp BS2S7HZ6360 tuner

 Copyright (C) 2009 Duolabs Spa

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifndef _AVL2108_REG_H
#define _AVL2108_REG_H

/* Generic ops like reset and PLL */
#define REG_CORE_RESET_B 0x600000
#define REG_RESET 0x6C0000
#define PLL_R1 0x6C0080
#define PLL_R2 0x6C00C0
#define PLL_R3 0x6C0100
#define PLL_R4 0x6C0140
#define PLL_R5 0x6C0180
#define PLL_SOFTVALUE_EN 0x6C0200

#define REG_ROM_VER 0x00100000 /**< Chip version */
#define REG_PATCH_VER 0x000004A8 /**< Patch version */
#define REG_ERROR_MSG 0x0000042C
#define REG_RX_CMD 0x00000400
#define REG_CORE_RDY_WORD 0x00000434

/* Generic ops */
#define DEMOD_OP_NOOP 0x00
#define DEMOD_OP_LD_DEFAULT 0x01
#define DEMOD_OP_INIT_GO 0x02
#define DEMOD_OP_RESET_BERPER 0x03
#define DEMOD_OP_HALT 0x04
#define DEMOD_OP_SLEEP 0x05
#define DEMOD_OP_WAKE 0x06
#define DEMOD_OP_ND_SCAN 0x08
#define DEMOD_OP_STDOUT_MODE 0x09
#define DEMOD_OP_TUNER_LOCK 0x0b
#define DEMOD_OP_TUNER_LOCK_ST 0x0c
#define DEMOD_OP_HELLO 0xFF

#define CI_FLAG_IQ_BIT 0x00
#define CI_FLAG_IQ_BIT_MASK 0x01
#define CI_FLAG_IQ_NO_SWAPPED 0x00

#define CI_FLAG_DVBS2_BIT 0x02
#define CI_FLAG_DVBS2_BIT_MASK 0x1C
#define CI_FLAG_DVBS2_UNDEF 0x04

#define CI_FLAG_IQ_AUTO_BIT 0x05
#define CI_FLAG_IQ_AUTO_BIT_MASK 0x20
#define CI_FLAG_IQ_AUTO_BIT_AUTO 0x01

#define CI_FLAG_LOCK_MODE_BIT 0x06
#define CI_FLAG_LOCK_MODE_BIT_MASK 0x40

#define TUNER_LOCKED 0x1

#define FUNC_MODE_DEMOD 0x0
#define FUNC_MODE_BLIND 0x1

#define LOCK_MODE_FIXED 0x0
#define LOCK_MODE_ADAPTIVE 0x1

/* RF AGC Polarisation */
#define AGC_POL_NORMAL 0x0
#define AGC_POL_INVERT 0x1

/* Tuner registers */
#define REG_TUNER_FREQ_100KHZ 0x000005D0 /**< Tuner frequency in 100KHz */
#define REG_TUNER_LPF_100KHZ 0x000005D2 /**< Tuner Low Pass Filter */
#define REG_TUNER_SLAVE_ADDR 0x000005D4 /**< Tuner slave address */
#define REG_TUNER_MAX_LPF_100KHZ 0x000005D6 /**< Tuner Low Pass Filter maximum */
#define REG_TUNER_LPF_MARGIN_100KHZ 0x000005D8 /**< Tuner Low Pass Filter margin */
#define REG_TUNER_USE_INTERNAL_CTRL 0x000005DA /**< Tuner control mode select */

#define TUNER_SLAVE_ADDR 0xC0

/* i2c repeater registers for tuner */
#define REG_I2C_SPEED_KHZ 0x000005CC
#define REG_I2C_CMD 0x00000404
#define REG_I2C_RSP 0x00000418
#define REG_TUNER_STATUS 0x000007B2

#define TUNER_I2C_CLK 200 /**< Tuner's clock speed in KHz dedicated to the I2C bus */

#define I2C_CMD_LEN 0x14
#define I2C_RSP_LEN 0x14
#define I2C_NOOP 0x00
#define I2C_INIT 0x01
#define I2C_WRITE 0x02
#define I2C_READ 0x03

/* Blind-scan registers */
#define REG_BLIND_SCAN_RETRIES 0x000005F8 /**< Blind scan retries */
#define REG_BLIND_SCAN_SRATE_TO_HZ 0x00002472
#define REG_BLIND_SCAN_CARIER_FREQ_TO_KHZ 0x00002484
#define REG_BLIND_SCAN_CARRIER_DB 0x00002486

/* Receiver registers */
#define REG_RF_AGC_POL 0x0000043c
#define REG_SPEC_INV 0x00000470
#define REG_MPEG_ERR_SIGNAL_POL 0x00000458 /**< MPEG error signal polarity */
#define REG_MPEG_SERIAL_OUT_SEL 0x00000460 /**< MPEG serial output pin select */
#define REG_MPEG_SERIAL_BIT_SEQ 0x00000468 /**< MPEG serial bit sequence */
#define REG_MPEG_POS_EDGE 0x000004DC /**< MPEG clock edge select */
#define REG_MPEG_SERIAL 0x000004DE /**< MPEG output mode */
#define REG_MPEG_MODE 0x0000045C /**< MPEG output mode select */
#define REG_MPEG_SERIAL_CLK_N 0x000004E0 /**< MPEG serial clock N */
#define REG_MPEG_SERIAL_CLK_D 0x000004E2 /**< MPEG serial clock D */
#define REG_MPEG_OUTPUT 0x006C0024 /**< MPEG output enable */
#define REG_ERR_MODE_CTRL 0x000004F8 /**< Error mode control */
#define REG_SNR_DB 0x00000680
#define REG_DMD_CLK_MHZ 0x00000580
#define REG_FEC_CLK_MHZ 0x00000582
#define REG_MPEG_CLK_MHZ 0x00000584
#define REG_SRATE_HZ 0x00000490
#define REG_FORMAT 0x0000044C
#define REG_ALPHA 0x00000440
#define REG_ALPHA_SETTING 0x0000046C
#define REG_DECODE_MODE 0x0000248A
#define REG_IQ_SWAP_MODE 0x00002488
#define REG_CARRIER_FREQ_HALF_RANGE_MHZ 0x0000058A
#define REG_FEC_LOCK 0x00000790
#define REG_BER 0x000007F8
#define REG_AAGC_REF 0x000004CA
#define REG_AAGC 0x0040004C
#define REG_DVBS_BER_ADDR 0x00002480
#define REG_LOCK_MODE 0x00002478
#define REG_FUNCTIONAL_MODE 0x00002476
#define REG_MPEG_PERSISTENT_CLK_MODE 0x0000246E /**< MPEG persistent clk mode */

/* Diseqc registers */
#define REG_DISEQC_TX_CTRL 0x00700000
#define REG_DISEQC_TONE_FRAC_N 0x00700004
#define REG_DISEQC_TONE_FRAC_D 0x00700008
#define REG_DISEQC_TX_ST 0x0070000C
#define REG_DISEQC_RX_MSG_TMR 0x00700014
#define REG_DISEQC_RX_CTRL 0x0070001C
#define REG_DISEQC_SRST 0x00700020
#define REG_DISEQC_SAMP_FRAC_N 0x00700028
#define REG_DISEQC_SAMP_FRAC_D 0x0070002C
#define REG_DISEQC_TX_FIFO_MAP 0x00700080

/* Diseqc data */
#define DISEQC_TONE_FREQ 22 /*< In KHz */

#define DISEQC_TX_GAP_15 0x0000 /*< 15ms gap: When transmitting, there is gap between TONE0 and TONE1 */
#define DISEQC_TX_GAP_20 0x0001 /*< 20ms gap */
#define DISEQC_TX_GAP_25 0x0002 /*< 25ms gap */
#define DISEQC_TX_GAP_30 0x0003 /*< 30ms gap */

#define DISEQC_TX_MODE_MOD 0x0000 /*< Modulation mode */
#define DISEQC_TX_MODE_TONE0 0x0010 /*< Send tone 0 */
#define DISEQC_TX_MODE_TONE1 0x0020 /*< Send tone 1 */
#define DISEQC_TX_MODE_CONT 0x0030 /*< Continuous send pulses */

#define DISEQC_WAVEFORM_NORMAL 0x0000 /*< Normal wave mode */
#define DISEQC_WAVEFORM_ENVELOPE 0x0100 /*< Wave is sent using envelopes */

#define DISEQC_RX_150 0x0000 /*< Wait 150 ms before closing the input FIFO */
#define DISEQC_RX_170 0x1000 /*< Wait 170 ms */
#define DISEQC_RX_190 0x2000 /*< Wait 190 ms */
#define DISEQC_RX_210 0x3000 /*< Wait 210 ms */

/* Diseqc status */
#define DISEQC_STATUS_UNINIT 0x00
#define DISEQC_STATUS_INIT 0x01
#define DISEQC_STATUS_CONTINUOUS 0x02
#define DISEQC_STATUS_TONE 0x03
#define DISEQC_STATUS_MOD 0x04

/* MPEG data */
#define MPEG_FORMAT_TS 0x0 /*< Transport stream */
#define MPEG_FORMAT_TS_PAR 0x1 /*< Transport stream + Parity */

#define MPEG_MODE_PARALLEL 0x0 /*< Output parallel data */
#define MPEG_MODE_SERIAL 0x1 /*< Output serial data */

#define MPEG_CLK_MODE_FALLING 0x0 /*< MPEG data sampled on clock's falling edge */
#define MPEG_CLK_MODE_RISING 0x1 /*< MPEG data sampled on clock's rising edge */

#endif /* _AVL2108_REG_H */
