/*---- UART Register definitions ------------------------------*/

/* Register offsets */

#define ASC_BAUDRATE			0x00
#define ASC_TXBUF			0x04
#define ASC_RXBUF			0x08
#define ASC_CTL				0x0C
#define ASC_INTEN			0x10
#define ASC_STA				0x14
#define ASC_GUARDTIME			0x18
#define ASC_TIMEOUT			0x1C
#define ASC_TXRESET			0x20
#define ASC_RXRESET			0x24
#define ASC_RETRIES			0x28

/* ASC_RXBUF */
#define ASC_RXBUF_PE			0x100
#define ASC_RXBUF_FE			0x200

/* ASC_CTL */

#define ASC_CTL_MODE_MSK		0x0007
#define  ASC_CTL_MODE_8BIT		0x0001
#define  ASC_CTL_MODE_7BIT_PAR		0x0003
#define  ASC_CTL_MODE_9BIT		0x0004
#define  ASC_CTL_MODE_8BIT_WKUP		0x0005
#define  ASC_CTL_MODE_8BIT_PAR		0x0007
#define ASC_CTL_STOP_MSK		0x0018
#define  ASC_CTL_STOP_HALFBIT		0x0000
#define  ASC_CTL_STOP_1BIT		0x0008
#define  ASC_CTL_STOP_1_HALFBIT		0x0010
#define  ASC_CTL_STOP_2BIT		0x0018
#define ASC_CTL_PARITYODD		0x0020
#define ASC_CTL_LOOPBACK		0x0040
#define ASC_CTL_RUN			0x0080
#define ASC_CTL_RXENABLE		0x0100
#define ASC_CTL_SCENABLE		0x0200
#define ASC_CTL_FIFOENABLE		0x0400
#define ASC_CTL_CTSENABLE		0x0800
#define ASC_CTL_BAUDMODE		0x1000

/* ASC_GUARDTIME */

#define ASC_GUARDTIME_MSK		0x00FF

/* ASC_INTEN */

#define ASC_INTEN_RBE			0x0001
#define ASC_INTEN_TE			0x0002
#define ASC_INTEN_THE			0x0004
#define ASC_INTEN_PE			0x0008
#define ASC_INTEN_FE			0x0010
#define ASC_INTEN_OE			0x0020
#define ASC_INTEN_TNE			0x0040
#define ASC_INTEN_TOI			0x0080
#define ASC_INTEN_RHF			0x0100

/* ASC_RETRIES */

#define ASC_RETRIES_MSK			0x00FF

/* ASC_RXBUF */

#define ASC_RXBUF_MSK			0x03FF

/* ASC_STA */

#define ASC_STA_RBF			0x0001
#define ASC_STA_TE			0x0002
#define ASC_STA_THE			0x0004
#define ASC_STA_PE			0x0008
#define ASC_STA_FE			0x0010
#define ASC_STA_OE			0x0020
#define ASC_STA_TNE			0x0040
#define ASC_STA_TOI			0x0080
#define ASC_STA_RHF			0x0100
#define ASC_STA_TF			0x0200
#define ASC_STA_NKD			0x0400

/* ASC_TIMEOUT */

#define ASC_TIMEOUT_MSK			0x00FF

/* ASC_TXBUF */

#define ASC_TXBUF_MSK			0x01FF

/*---- Values for the BAUDRATE Register -----------------------*/

/*
 * MODE 0
 * recommended for low bit rates (below 19.2K)
 *
 *                       ICCLK
 * ASCBaudRate =   ----------------
 *                   baudrate * 16
 *
 * MODE 1
 * recommended for high bit rates (above 19.2K)
 *
 *                   baudrate * 16 * 2^16
 * ASCBaudRate =   ------------------------
 *                          ICCLK
 */

#define ADJ 1
#define BAUDRATE_VAL_M0(bps, clk) \
	((clk) / (16 * (bps)))
#define BAUDRATE_VAL_M1(bps, clk) \
	(((bps * (1 << 14)) / ((clk) / (1 << 6))) + ADJ)

/*---- Access macros ------------------------------------------*/

#define ASC_FUNC(name, offset)		\
	static inline unsigned int asc_##name##_in(struct uart_port *port) \
	{ \
		return (readl(port->membase + (offset))); \
	} \
	\
	static inline void asc_##name##_out(struct uart_port *port, \
			unsigned int value) \
	{ \
		writel(value, port->membase + (offset)); \
	}

ASC_FUNC(BAUDRATE,  ASC_BAUDRATE)
ASC_FUNC(TXBUF,     ASC_TXBUF)
ASC_FUNC(RXBUF,     ASC_RXBUF)
ASC_FUNC(CTL,       ASC_CTL)
ASC_FUNC(INTEN,     ASC_INTEN)
ASC_FUNC(STA,       ASC_STA)
ASC_FUNC(GUARDTIME, ASC_GUARDTIME)
ASC_FUNC(TIMEOUT,   ASC_TIMEOUT)
ASC_FUNC(TXRESET,   ASC_TXRESET)
ASC_FUNC(RXRESET,   ASC_RXRESET)
ASC_FUNC(RETRIES,   ASC_RETRIES)

#define asc_in(port, reg)		asc_ ## reg ## _in (port)
#define asc_out(port, reg, value)	asc_ ## reg ## _out ((port), (value))

