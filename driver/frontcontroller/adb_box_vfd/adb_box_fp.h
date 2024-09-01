#define VFD_MAJOR			147

#define SCP_TXD_BIT			6
#define SCP_SCK_BIT			8
#define SCP_ENABLE_BIT			5
#define VFD_PORT			0
#define SCP_DATA			3
#define SCP_CLK				4
#define SCP_CS				5

#define PIO_PORT_SIZE			0x1000
#define PIO_BASE			0xb8020000
#define STPIO_SET_OFFSET		0x4
#define STPIO_CLEAR_OFFSET		0x8
#define STPIO_POUT_OFFSET		0x00
#define STPIO_SET_PIN(PIO_ADDR, PIN, V) writel(1<<PIN, PIO_ADDR + STPIO_POUT_OFFSET + ((V)? STPIO_SET_OFFSET : STPIO_CLEAR_OFFSET))
#define PIO_PORT(n) (((n)*PIO_PORT_SIZE) + PIO_BASE)
//------------------------------------------------

// PT6302
#define ADB_BOX_VFD_MAX_CHARS 16

#define ADB_BOX_VFD_PIO_PORT_SCS 1
#define ADB_BOX_VFD_PIO_PIN_SCS  2

// wspolne PIO
#define ADB_BOX_VFD_PIO_PORT_SCL 4
#define ADB_BOX_VFD_PIO_PIN_SCL  0

#define ADB_BOX_VFD_PIO_PORT_SDA 4
#define ADB_BOX_VFD_PIO_PIN_SDA  1

// PT6958
#define PT6958_BUTTON_PIO_PORT_DOUT	2
#define PT6958_BUTTON_PIO_PIN_DOUT	2

#define PT6958_BUTTON_PIO_PORT_STB	1
#define PT6958_BUTTON_PIO_PIN_STB	6

/*
PIO 1.6 [fp_nload  ] [OUT (push-pull)    ] []	- STB
//PIO 2.2 [fp_key    ] [IN  (Hi-Z)         ] []	- DOUT
//PIO 3.3 [fp_ir_in  ] [IN  (Hi-Z)         ] [] - IRDA
PIO 4.0 [fp_clk    ] [Alt-BI (open-drain)] [] 	- CLK
PIO 4.1 [fp_data   ] [Alt-BI (open-drain)] [] 	- DIN
*/


//------------------------------------------------
#define DBG(fmt, args...) if ( debug ) printk(KERN_INFO "[vfd] :: " fmt "\n", ## args )
#define ERR(fmt, args...) printk(KERN_ERR "[vfd] :: " fmt "\n", ## args )

static int debug   	= 0;
static int delay   	= 5;
static int rec  	= 1;


static int led_POW  	= 0x02;

struct scp_driver
{
	struct stpio_pin *scs;
	struct stpio_pin *scl;
	struct stpio_pin *sda;
//  struct stpio_pin *ske;
//  struct stpio_pin *snl;
};

//------------------------------------------------

struct vfd_ioctl_data
{
	unsigned char address;
	unsigned char data[64];
	unsigned char length;
};

struct __vfd_scp
{
	__u8 tr_rp_ctrl;
	__u8 rp_data;
	__u8 tr_data;
	__u8 start_tr;
	__u8 status;
	__u8 reserved;
	__u8 clk_div;
};

struct __vfd_scp *vfd_scp_ctrl = NULL;

#define SCP_TXD_CTRL        (vfd_scp_ctrl->tr_rp_ctrl)
#define SCP_TXD_DATA        (vfd_scp_ctrl->tr_data)
#define SCP_TXD_START       (vfd_scp_ctrl->start_tr)
#define SCP_STATUS          (vfd_scp_ctrl->status)

int SCP_PORT = 0;

