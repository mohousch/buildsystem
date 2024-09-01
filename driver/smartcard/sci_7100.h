#ifndef _SCI_7100_H
#define _SCI_7100_H

#if defined(CUBEREVO) || defined(CUBEREVO_MINI) || defined(CUBEREVO_MINI2) || \
    defined(CUBEREVO_250HD) || defined(CUBEREVO_9500HD) || \
    defined(CUBEREVO_2000HD) || defined(CUBEREVO_MINI_FTA) || defined(CUBEREVO_3000HD)
#define CUBEBOX
#else
#undef  CUBEBOX
#endif

#if defined(IPBOX9900) || defined(IPBOX99) || defined(IPBOX55) || defined(ARIVALINK200) || defined(HL101)
#define IPBOX
#else
#undef IPBOX
#endif

/* Hellmaster1024 Octagon1008 and Fortis HDBOX need these defines, maybe other sti7100/sti7101 based STB need this defines too
*/
#if defined(OCTAGON1008) || defined(FORTIS_HDBOX) || defined(CUBEBOX) || defined(IPBOX)
#define SUPPORT_NO_VOLTAGE
#define SUPPORT_NO_AUTOSET
#endif

#if defined(ADB_BOX)
#define SUPPORT_NO_VOLTAGE
#define SUPPORT_NO_AUTOSET
#endif

#define SYS_CFG_BASE_ADDRESS        0x19001000
#define SYS_CFG7                    0x11C

/******* SC 0 *******/

#define SCI0_INT_DETECT             80
#define SCI0_INT_RX_TX              123

#define PIO0_BASE_ADDRESS           0x18020000
#define PIO_P0C0                    0x20
#define PIO_P0C1                    0x30
#define PIO_P0C2                    0x40
#define PIO_SET_P0OUT               0x04
#define PIO_CLR_P0OUT               0x08
#define PIO_P0MASK                  0x60
#define PIO_P0COMP                  0x50
#define PIO_SET_P0COMP              0x54
#define PIO_CLR_P0COMP              0x58

#define SCI0_BASE_ADDRESS           0x18048000

#define ASC0_BASE_ADDRESS           0x18030000
#define ASC0_BAUDRATE               0x00
#define ASC0_TX_BUF                 0x004
#define ASC0_RX_BUF                 0x008
#define ASC0_CTRL                   0x00C
#define ASC0_INT_EN                 0x010
#define ASC0_STA                    0x014
#define ASC0_GUARDTIME              0x018
#define ASC0_TIMEOUT                0x01C
#define ASC0_TX_RST                 0x020
#define ASC0_RX_RST                 0x024
#define ASC0_RETRIES                0x028

/* Test SC0 voltage to 3v. */
#define PIO4_BASE_ADDRESS           0x18024000

#define PIO_CLR_P4C0                0x28
#define PIO_CLR_P4C1                0x38
#define PIO_CLR_P4C2                0x48
#define PIO_SET_P4C0                0x24
#define PIO_SET_P4C1                0x34
#define PIO_SET_P4C2                0x44
#define PIO_SET_P4OUT               0x04
#define PIO_CLR_P4OUT               0x08

/******* SC 1 *******/

#define SCI1_INT_DETECT             84
#define SCI1_INT_RX_TX              122

#define PIO1_BASE_ADDRESS           0x18021000
#define PIO_P1C0                    0x20
#define PIO_P1C1                    0x30
#define PIO_P1C2                    0x40
#define PIO_SET_P1OUT               0x04
#define PIO_CLR_P1OUT               0x08
#define PIO_P1MASK                  0x60
#define PIO_P1COMP                  0x50
#define PIO_SET_P1COMP              0x54
#define PIO_CLR_P1COMP              0x58

#define SCI1_BASE_ADDRESS           0x18049000

#define ASC1_BASE_ADDRESS           0x18031000
#define ASC1_BAUDRATE               0x00
#define ASC1_TX_BUF                 0x004
#define ASC1_RX_BUF                 0x008
#define ASC1_CTRL                   0x00C
#define ASC1_INT_EN                 0x010
#define ASC1_STA                    0x014
#define ASC1_GUARDTIME              0x018
#define ASC1_TIMEOUT                0x01C
#define ASC1_TX_RST                 0x020
#define ASC1_RX_RST                 0x024
#define ASC1_RETRIES                0x028

/* Test SC1 voltage to 3v. */
#define PIO3_BASE_ADDRESS           0x18023000

#define PIO_CLR_P3C0                0x28
#define PIO_CLR_P3C1                0x38
#define PIO_CLR_P3C2                0x48
#define PIO_SET_P3C0                0x24
#define PIO_SET_P3C1                0x34
#define PIO_SET_P3C2                0x44
#define PIO_SET_P3OUT               0x04
#define PIO_CLR_P3OUT               0x08

/******* Board-specific defines *******/

#define ACTIVE_HIGH                 1
#define ACTIVE_LOW                  0
#define SCI_CLASS                   1 //SCI_CLASS_A     /**< Operating class of SCI */

#endif  /* _SCI_7100_H */
