#ifndef TF7700REGISTER_H
#define TF7700REGISTER_H

/* Asynchronous Serial Interface   */

#define ASC0BaseAddress 0xb8030000
#define ASC1BaseAddress 0xb8031000
#define ASC2BaseAddress 0xb8032000
#define ASC3BaseAddress 0xb8033000
#define ASC_BAUDRATE    0x000
#define ASC_TX_BUFF     0x004
#define ASC_RX_BUFF     0x008
#define ASC_CTRL        0x00c
#define ASC_INT_EN      0x010
#define ASC_INT_STA     0x014
#define ASC_GUARDTIME   0x018
#define ASC_TIMEOUT     0x01c
#define ASC_TX_RST      0x020
#define ASC_RX_RST      0x024
#define ASC_RETRIES     0x028

#define ASC_INT_STA_RBF   0x01
#define ASC_INT_STA_TE    0x02
#define ASC_INT_STA_THE   0x04
#define ASC_INT_STA_PE    0x08
#define ASC_INT_STA_FE    0x10
#define ASC_INT_STA_OE    0x20
#define ASC_INT_STA_TONE  0x40
#define ASC_INT_STA_TOE   0x80
#define ASC_INT_STA_RHF   0x100
#define ASC_INT_STA_TF    0x200
#define ASC_INT_STA_NKD   0x400

#define ASC_CTRL_FIFO_EN  0x400

/*  GPIO Pins  */

#define PIO0BaseAddress   0xb8020000
#define PIO1BaseAddress   0xb8021000
#define PIO2BaseAddress   0xb8022000
#define PIO3BaseAddress   0xb8023000
#define PIO4BaseAddress   0xb8024000
#define PIO5BaseAddress   0xb8025000

#define PIO_CLR_PnC0      0x28
#define PIO_CLR_PnC1      0x38
#define PIO_CLR_PnC2      0x48
#define PIO_CLR_PnCOMP    0x58
#define PIO_CLR_PnMASK    0x68
#define PIO_CLR_PnOUT     0x08
#define PIO_PnC0          0x20
#define PIO_PnC1          0x30
#define PIO_PnC2          0x40
#define PIO_PnCOMP        0x50
#define PIO_PnIN          0x10
#define PIO_PnMASK        0x60
#define PIO_PnOUT         0x00
#define PIO_SET_PnC0      0x24
#define PIO_SET_PnC1      0x34
#define PIO_SET_PnC2      0x44
#define PIO_SET_PnCOMP    0x54
#define PIO_SET_PnMASK    0x64
#define PIO_SET_PnOUT     0x04

#define DencBaseAddress   0xb920c000
#define DENC_CFG0         0x000
#define DENC_CFG1         0x004
#define DENC_CFG2         0x008
#define DENC_CFG3         0x00c
#define DENC_CFG4         0x010
#define DENC_CFG5         0x014
#define DENC_CFG6         0x018
#define DENC_CFG7         0x01c
#define DENC_CFG8         0x020
#define DENC_CFG9         0x144
#define DENC_CFG10        0x170
#define DENC_CFG11        0x174
#define DENC_CFG12        0x178
#define DENC_CFG13        0x17c
#define DENC_STA          0x024
#define DENC_DFS_INC0     0x028
#define DENC_DFS_INC1     0x02c
#define DENC_DFS_INC2     0x030
#define DENC_DFS_PHASE0   0x034
#define DENC_DFS_PHASE1   0x038
#define DENC_WSS1         0x03c
#define DENC_WSS2         0x040


#endif
