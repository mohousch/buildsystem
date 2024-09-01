#ifndef _asc_123
#define _asc_123

/* ************************************************** */
/* Access ASC3; from u-boot; copied from TF7700 ;-)   */
/* ************************************************** */

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


//-------------------------------------
#define PIO4BaseAddress   0xb8024000

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

int serial_putc(char Data);
void serial_init(void);

extern unsigned int InterruptLine;
extern unsigned int ASCXBaseAddress;

#endif
