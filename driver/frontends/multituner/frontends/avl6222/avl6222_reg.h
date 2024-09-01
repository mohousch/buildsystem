#define _AVL6222_REG_H

#define raptor_status_addr             (0x00000860 + 0x0)
#define rx_rsp_addr                    (0x00000200 + 0x0)
#define rx_cmd_addr                    (0x00000400 + 0x0)
#define i2cm_cmd_addr                  (0x00000404 + 0x0)
#define i2cm_rsp_addr                  (0x00000418 + 0x0)
#define error_msg_addr                 (0x0000042c + 0x0)
#define core_ready_word_addr           (0x00000434 + 0x0)
#define i2cm_status_addr               (0x00000438 + 0x0)
#define rx_config_addr                 (0x0000043c + 0x0)
#define rx_state_addr                  (0x00000690 + 0x0)
#define sharemem_addr                  (0x000008d8 + 0x0)
#define patchglobalvar_addr            (0x00002614 + 0x0)
#define patchtext_addr                 (0x00002654 + 0x0)
#define rom_ver_addr                   (0x00100000 + 0x0)
#define rx_config_rom_addr             (0x00100004 + 0x0)
#define rp_sleep_wake_status_addr      (0x00002632 + 0x0)

/* Generic ops like reset and PLL */
#define REG_CORE_RESET_B               0x600000
#define PLL_R1                         0x6C4100
#define PLL_R2                         0x6C40C0
#define PLL_R3                         0x6C4080
#define PLL_R4                         0x6C4140
#define PLL_R5                         0x6C4180
#define PLL_R6                         0x6C41C0
#define PLL_SOFTVALUE_EN               0x6C4200
#define REG_RESET                      0x6C4000

#define rc_functional_mode_addr        (rx_config_addr + 0x20c)
#define rc_iq_mode_addr                (rx_config_addr + 0x204)

#define REG_ROM_VER                    0x00100000  /**< Chip version */
#define REG_PATCH_VER                  (rx_config_addr + 0x7c)  /**< Patch version */
#define REG_ERROR_MSG                  0x0000042C
#define REG_RX_CMD                     0x00000400
#define REG_CORE_RDY_WORD              0x00000434

/* Generic ops */
#define DEMOD_OP_NOOP                  0x00
#define DEMOD_OP_LD_DEFAULT            0x01
#define DEMOD_OP_INIT_GO               0x02
#define DEMOD_OP_RESET_BERPER          0x03
#define DEMOD_OP_HALT                  0x04
#define DEMOD_OP_SLEEP                 0x05
#define DEMOD_OP_WAKE                  0x06
#define DEMOD_OP_ND_SCAN               0x08
#define DEMOD_OP_STDOUT_MODE           0x09
#define DEMOD_OP_TUNER_LOCK            0x0b
#define DEMOD_OP_TUNER_LOCK_ST         0x0c

#define CI_FLAG_IQ_BIT                 0x00
#define CI_FLAG_IQ_BIT_MASK            0x01
#define CI_FLAG_IQ_NO_SWAPPED          0x00
#define CI_FLAG_IQ_SWAPPED             0x01

#define CI_FLAG_DVBS2_BIT              0x02
#define CI_FLAG_DVBS2_BIT_MASK         0x1C
#define CI_FLAG_DVBS2_UNDEF            0x04
#define CI_FLAG_DVBS                   0x00
#define CI_FLAG_DVBS2                  0x01

#define CI_FLAG_IQ_AUTO_BIT            0x05
#define CI_FLAG_IQ_AUTO_BIT_MASK       0x20
#define CI_FLAG_IQ_AUTO_BIT_AUTO       0x01
#define CI_FLAG_IQ_AUTO_BIT_OFF        0x00

#define CI_FLAG_LOCK_MODE_BIT          0x06
#define CI_FLAG_LOCK_MODE_BIT_MASK     0x40

#define TUNER_LOCKED                   0x1

#define FUNC_MODE_DEMOD                0x0

#define LOCK_MODE_FIXED                0x0
#define LOCK_MODE_ADAPTIVE             0x1

/* RF AGC Polarisation */
#define AGC_POL_NORMAL                 0x0
#define AGC_POL_INVERT                 0x1

/* Tuner registers */
#define REG_TUNER_FREQ_100KHZ          (rx_config_addr + 0x1b2)  /**< Tuner frequency in 100KHz */
#define REG_TUNER_LPF_100KHZ           (rx_config_addr + 0x1b4)  /**< Tuner Low Pass Filter */
#define REG_TUNER_SLAVE_ADDR           (rx_config_addr + 0x1b6)  /**< Tuner slave address */
#define REG_TUNER_MAX_LPF_100KHZ       (rx_config_addr + 0x1b8)  /**< Tuner Low Pass Filter maximum */
#define REG_TUNER_LPF_MARGIN_100KHZ    (rx_config_addr + 0x1ba)  /**< Tuner Low Pass Filter margin */
#define REG_TUNER_USE_INTERNAL_CTRL    (rx_config_addr + 0x1bc)  /**< Tuner control mode select */

#define TUNER_SLAVE_ADDR               0xC0

/* i2c repeater registers for tuner */
#define REG_I2C_SPEED_KHZ              (rx_config_addr + 0x1ae)
#define REG_I2C_CMD                    0x00000404
#define REG_I2C_RSP                    0x00000418
#define REG_TUNER_STATUS               (rx_state_addr + 0x186)

#define TUNER_I2C_CLK                   200  /**< Tuner's clock speed in kHz dedicated to the I2C bus */

#define I2C_CMD_LEN                     0x14
#define I2C_RSP_LEN                     0x14
#define I2C_NOOP                        0x00
#define I2C_INIT                        0x01
#define I2C_WRITE                       0x02
#define I2C_READ                        0x03

#define rc_mpeg_bus_tri_enb             0x006C0028

/* Receiver registers */
#define REG_PER_ENABLED                 0x000004AE
#define REG_BER_ENABLED                 0x000004EC
#define REG_RF_AGC_POL                  (rx_config_addr + 0x0)
#define REG_SPEC_INV                    (rx_config_addr + 0x34)
#define REG_MPEG_ERR_SIGNAL_POL         0x00000458  /**< MPEG error signal polarity */ // ???
#define REG_MPEG_SERIAL_OUT_SEL         0x00000460  /**< MPEG serial output pin select */
#define REG_MPEG_SERIAL_BIT_SEQ         (rx_config_addr + 0x2c)  /**< MPEG serial bit sequence */
#define REG_MPEG_POS_EDGE               (rx_config_addr + 0xbc)  /**< MPEG clock edge select */
#define REG_MPEG_SERIAL                 (rx_config_addr + 0xbe)  /**< MPEG output mode */
#define REG_MPEG_MODE                   (rx_config_addr + 0x20)  /**< MPEG output mode select */
#define REG_MPEG_SERIAL_CLK_N           0x000004E0  /**< MPEG serial clock N */
#define REG_MPEG_SERIAL_CLK_D           0x000004E2  /**< MPEG serial clock D */
#define REG_MPEG_OUTPUT                 0x006C0024  /**< MPEG output enable */
#define REG_ERR_MODE_CTRL               0x000004F8  /**< Error mode control */
#define REG_SNR_DB                      (rx_state_addr + 0x40)
#define REG_SNR_LINEAR                  0x00000690
#define REG_DMD_CLK_MHZ                 (rx_config_addr + 0x162)
#define REG_FEC_CLK_MHZ                 (rx_config_addr + 0x164)
#define REG_MPEG_CLK_MHZ                (rx_config_addr + 0x166)
#define REG_SRATE_HZ                    (rx_config_addr + 0x54)
#define REG_FORMAT                      (rx_config_addr + 0x10)
#define REG_ALPHA                       (rx_config_addr + 0x4)
#define REG_ALPHA_SETTING               (rx_config_addr + 0x30)
#define REG_DECODE_MODE                 (rx_config_addr + 0x202)
#define REG_IQ_SWAP_MODE                (rx_config_addr + 0x204)
#define REG_CARRIER_FREQ_HALF_RANGE_MHZ (rx_config_addr + 0x16c)
#define REG_FEC_LOCK                    (rx_state_addr + 0x164)
#define REG_BER                         (raptor_status_addr + 0x0)
#define REG_AAGC_REF                    (rx_config_addr + 0xaa)
#define REG_AAGC                        (rx_config_addr + 0xf8)
#define REG_DVBS_BER_ADDR               (rx_config_addr + 0x98)
#define REG_LOCK_MODE                   (rx_config_addr + 0x20a)
#define REG_FUNCTIONAL_MODE             (rx_config_addr + 0x20c)
#define REG_MPEG_PERSISTENT_CLK_MODE    (rx_config_addr + 0x210)

/* Diseqc registers */
#define REG_DISEQC_TX_CTRL              0x00700000
#define REG_DISEQC_TONE_FRAC_N          0x00700004
#define REG_DISEQC_TONE_FRAC_D          0x00700008
#define REG_DISEQC_TX_ST                0x0070000C
#define REG_DISEQC_RX_PARITY_ADDR       0x00700010
#define REG_DISEQC_RX_MSG_TMR           0x00700014
#define REG_DISEQC_RX_ST                0x00700018
#define REG_DISEQC_RX_CTRL              0x0070001C
#define REG_DISEQC_SRST                 0x00700020
#define REG_DISEQC_BIT_TIME_ADDR        0x00700024
#define REG_DISEQC_SAMP_FRAC_N          0x00700028
#define REG_DISEQ_BIT_DECODE_RANGE_ADDR 0x00700030
#define REG_DISEQC_SAMP_FRAC_D          0x0070002C
#define REG_DISEQC_TX_FIFO_MAP          0x00700080

/* Diseqc data */
#define DISEQC_TONE_FREQ                22  /*< In kHz */

#define DISEQC_TX_GAP_15                0x0000  /*< 15ms gap: When transmitting, there is gap between TONE0 and TONE1 */
#define DISEQC_TX_GAP_20                0x0001  /*< 20ms gap */
#define DISEQC_TX_GAP_25                0x0002  /*< 25ms gap */
#define DISEQC_TX_GAP_30                0x0003  /*< 30ms gap */

#define DISEQC_TX_MODE_MOD              0x0000  /*< Modulation mode */
#define DISEQC_TX_MODE_TONE0            0x0010  /*< Send tone 0 */
#define DISEQC_TX_MODE_TONE1            0x0020  /*< Send tone 1 */
#define DISEQC_TX_MODE_CONT             0x0030  /*< Continuous send pulses */

#define DISEQC_WAVEFORM_NORMAL          0x0000  /*< Normal wave mode */
#define DISEQC_WAVEFORM_ENVELOPE        0x0100  /*< Wave is sent using envelopes */

#define DISEQC_RX_150                   0x0000  /*< Wait 150 ms before closing the input FIFO */
#define DISEQC_RX_170                   0x1000  /*< Wait 170 ms */
#define DISEQC_RX_190                   0x2000  /*< Wait 190 ms */
#define DISEQC_RX_210                   0x3000  /*< Wait 210 ms */

/* Diseqc status */
#define DISEQC_STATUS_UNINIT            0x00
#define DISEQC_STATUS_INIT              0x01
#define DISEQC_STATUS_CONTINUOUS        0x02
#define DISEQC_STATUS_TONE              0x03
#define DISEQC_STATUS_MOD               0x04

/* MPEG data */
#define MPEG_FORMAT_TS                  0x0  /*< Transport stream */
#define MPEG_FORMAT_TS_PAR              0x1  /*< Transport stream + Parity */

#define MPEG_MODE_PARALLEL              0x0  /*< Output parallel data */
#define MPEG_MODE_SERIAL                0x1  /*< Output serial data */

#define MPEG_CLK_MODE_FALLING           0x0  /*< MPEG data sampled on clock's falling edge */
#define MPEG_CLK_MODE_RISING            0x1  /*< MPEG data sampled on clock's rising edge */

#define REG_RGAGC_TRI_ENB               0x006C002C
// vim:ts=4