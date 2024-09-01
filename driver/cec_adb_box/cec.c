/*
 */

#include <linux/proc_fs.h>  	/* proc fs */ 
#include <asm/uaccess.h>    	/* copy_from_user */

#include <linux/string.h>
#include <linux/module.h>

#include <asm/io.h>
#include <asm/uaccess.h>
#include <linux/version.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/mm.h>

#include <linux/interrupt.h>

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,17)
#include <linux/stm/pio.h>
#else
#include <linux/stpio.h>
#endif

int remove_e2_procs(char *path, read_proc_t *read_func, write_proc_t *write_func);
int install_e2_procs(char *path, read_proc_t *read_func, write_proc_t *write_func, void *data);

unsigned long asc_registers;
struct stpio_pin* cec_tx;
struct stpio_pin* cec_rx;

static volatile unsigned char buf[3000];
static volatile unsigned int buf_len=0;
static volatile unsigned int buf_pos=0;

#define DEBUG_CEC

#ifdef DEBUG_CEC
#define debug_cec(args...) printk(args)
#else
#define debug_cec(args...)
#endif

unsigned long asc_registers;

#define ASC_BAUDRATE		(asc_registers + 0x00)
#define ASC_TXBUF			(asc_registers + 0x04)
#define ASC_RXBUF			(asc_registers + 0x08)
#define ASC_CTL				(asc_registers + 0x0C)
#define ASC_INTEN			(asc_registers + 0x10)
#define ASC_STA				(asc_registers + 0x14)
#define ASC_GUARDTIME		(asc_registers + 0x18)
#define ASC_TIMEOUT			(asc_registers + 0x1C)
#define ASC_TXRESET			(asc_registers + 0x20)
#define ASC_RXRESET			(asc_registers + 0x24)
#define ASC_RETRIES			(asc_registers + 0x28)

#define ADJ 1
#define BAUDRATE_VAL_M1(bps, clk) \
	(((bps * (1 << 14)) / ((clk) / (1 << 6))) + ADJ)

static int wait_idle ( int timeout )
{
    unsigned long start = jiffies;
    int status;

	while(1)
	{
		if (buf_len==0) break;
        if (jiffies - start > timeout) {
            debug_cec ("%s: timeout!!\n", __FUNCTION__);
            return -ETIMEDOUT;
        }
        msleep(10);
    };
    return 0;
}

int prepare_to_buf_300us(char	*myString,unsigned long count)
{

	unsigned char len;
	int i,j,k;
	unsigned char byte;
	unsigned char buf_[10];
	unsigned int value;

	len = count / 2;
	
	i=0;k=0;
	for (j=0;j<len;j++)
        {
		buf[0]='0';
		buf[1]='x';
		buf[2]=myString[k];k=k+1;
		buf[3]=myString[k];k=k+1;
		buf[4]= '\0';
		sscanf((char*)buf, "%x", &value);
		buf_[j]=value;
	}

	buf_pos=1;
	buf_len=0;

	//start bit
	for (i=0;i<12;i++){buf_len=buf_len+1;buf[buf_len]=0;} //3.6
	for (i=0;i<3;i++){buf_len=buf_len+1;buf[buf_len]=1;}  //0.9

	for (k=0;k<len;k++)
        {
	byte=buf_[k];
	debug_cec("[%d]=%x\n",k,byte);

	for (i=0;i<8;i++)
        {
		if((byte&128)==128) 
		{	//1
			for (j=0;j<2;j++){buf_len=buf_len+1;buf[buf_len]=0;}
			for (j=0;j<6;j++){buf_len=buf_len+1;buf[buf_len]=1;}
		}
		else
		{	//0
			for (j=0;j<5;j++){buf_len=buf_len+1;buf[buf_len]=0;}
			for (j=0;j<3;j++){buf_len=buf_len+1;buf[buf_len]=1;}
		}
		byte=byte<<1;
	}

	if ((k+1)==len)
	{
	debug_cec("End_of_message\n");
	//end of message (1)
	for (j=0;j<2;j++){buf_len=buf_len+1;buf[buf_len]=0;}
	for (j=0;j<6;j++){buf_len=buf_len+1;buf[buf_len]=1;}
	}
	else
	{
	//end of message (0)
	for (j=0;j<5;j++){buf_len=buf_len+1;buf[buf_len]=0;}
	for (j=0;j<3;j++){buf_len=buf_len+1;buf[buf_len]=1;}
	}

	//ack (0)
	for (j=0;j<5;j++){buf_len=buf_len+1;buf[buf_len]=0;}
	for (j=0;j<3;j++){buf_len=buf_len+1;buf[buf_len]=1;}

	}

	//3036	standby
	//3004	imageviewon
	//30820000 activesource

}


int prepare_to_buf_100us(char	*myString,unsigned long count)
{

	unsigned char len;
	int i,j,k;
	unsigned char byte;
	unsigned char buf_[10];
	unsigned int value;

	len = count / 2;
	
	i=0;k=0;
	for (j=0;j<len;j++)
        {
		buf[0]='0';
		buf[1]='x';
		buf[2]=myString[k];k=k+1;
		buf[3]=myString[k];k=k+1;
		buf[4]= '\0';
		sscanf((char*)buf, "%x", &value);
		buf_[j]=value;
	}

	buf_pos=1;
	buf_len=0;

	//start bit
	for (i=0;i<37;i++){buf_len=buf_len+1;buf[buf_len]=0;}
	for (i=0;i<8;i++){buf_len=buf_len+1;buf[buf_len]=1;}

	for (k=0;k<len;k++)
        {
	byte=buf_[k];
	debug_cec("[%d]=%x\n",k,byte);

	for (i=0;i<8;i++)
        {
		if((byte&128)==128) 
		{	//1
			for (j=0;j<6;j++){buf_len=buf_len+1;buf[buf_len]=0;}
			for (j=0;j<18;j++){buf_len=buf_len+1;buf[buf_len]=1;}
		}
		else
		{	//0
			for (j=0;j<15;j++){buf_len=buf_len+1;buf[buf_len]=0;}
			for (j=0;j<9;j++){buf_len=buf_len+1;buf[buf_len]=1;}
		}
		byte=byte<<1;
	}

	if ((k+1)==len)
	{
	debug_cec("End_of_message\n");
	//end of message (1)
	for (j=0;j<6;j++){buf_len=buf_len+1;buf[buf_len]=0;}
	for (j=0;j<18;j++){buf_len=buf_len+1;buf[buf_len]=1;}
	}
	else
	{
	//end of message (0)
	for (j=0;j<15;j++){buf_len=buf_len+1;buf[buf_len]=0;}
	for (j=0;j<9;j++){buf_len=buf_len+1;buf[buf_len]=1;}
	}

	//ack (0)
	for (j=0;j<15;j++){buf_len=buf_len+1;buf[buf_len]=0;}
	for (j=0;j<9;j++){buf_len=buf_len+1;buf[buf_len]=1;}

	}

	//3036	standby
	//3004	imageviewon
	//30820000 activesource

}

//echo "3036" > /proc/stb/hdmi/cec
//echo "3004" > /proc/stb/hdmi/cec
//echo "3f821100" > /proc/stb/hdmi/cec

//echo "3f821000" > /proc/stb/hdmi/cec

//3f821100 - przelaczenie na hdmi1
//3f822100 - przelaczenie na hdmi2

//3f821000 - przelaczenie na hdmi1
//3f822000 - przelaczenie na hdmi2


int proc_cec_write(struct file *file, const char __user *buf,
                           unsigned long count, void *data)
{
	char 		*page;
	char		*myString;
	ssize_t 	ret = -ENOMEM;
	
	debug_cec("%s %d - ", __FUNCTION__, (int) count);

	page = (char *)__get_free_page(GFP_KERNEL);
	if (page) 
	{
		
		
		ret = -EFAULT;
		if (copy_from_user(page, buf, count))
			goto out;

		myString = (char *) kmalloc(count + 1, GFP_KERNEL);
		strncpy(myString, page, count);
		myString[count] = '\0';

		debug_cec("%s\n", myString);
		if (wait_idle (100) < 0) goto out;
//		prepare_to_buf_100us(myString,count);
		prepare_to_buf_300us(myString,count);
		writel(2,ASC_INTEN);//int tx empty
		writel(0xff,ASC_TXBUF);//tx

		kfree(myString);
	}
	ret = count;
out:
	
	free_page((unsigned long)page);
	return ret;
}


int proc_cec_read(char *page, char **start, off_t off, int count,
			  int *eof, void *data_unused)
{
	printk("%s\n", __FUNCTION__);
        return 0;
}


static irqreturn_t asc_irq(int irq, void *dev_id)
{
	if (buf_len>0)
	{
		if (buf[buf_pos]==1) 
			stpio_set_pin (cec_tx, 1);
		else
			stpio_set_pin (cec_tx, 0);
		buf_pos=buf_pos+1;buf_len=buf_len-1;
	}
	else {stpio_set_pin (cec_tx, 1);writel(0,ASC_INTEN);return IRQ_HANDLED;}
	writel(0xff,ASC_TXBUF);//tx
	return IRQ_HANDLED;
}

#define CLOCKGEN_PLL1_CFG	(0xb9213000 + 0x24)
#define CONFIG_SH_EXTERNAL_CLOCK 27000000

static int get_pll_freq(unsigned long addr)
{
	unsigned long freq, data, ndiv, pdiv, mdiv;

	data = readl(addr);
	mdiv = (data >>  0) & 0xff;
	ndiv = (data >>  8) & 0xff;
	pdiv = (data >> 16) & 0x7;
	freq = (((2 * (CONFIG_SH_EXTERNAL_CLOCK / 1000) * ndiv) / mdiv) /
		(1 << pdiv)) * 1000;

	return freq;
}

struct e2_procs
{
  char *name;
  read_proc_t *read_proc;
  write_proc_t *write_proc;
} e2_procs[] =
{
  {"stb/hdmi/cec", proc_cec_read, proc_cec_write}
};

static int __init init_cec_module(void)
{
	int f,t;

	debug_cec("CEC Init>>\n");

	install_e2_procs(e2_procs[0].name, e2_procs[0].read_proc, e2_procs[0].write_proc, NULL);

	cec_tx = stpio_request_pin (4, 4, "pin_cec_tx", STPIO_BIDIR);//STPIO_OUT STPIO_BIDIR STPIO_BIDIR_Z1
	if (cec_tx==NULL) {debug_cec("FAIL : request pin 4 4\n");goto err;}
	stpio_set_pin (cec_tx, 1);

	asc_registers = (unsigned long) ioremap(0x18033000, 0x2c);

	if (request_irq(120, asc_irq ,IRQF_DISABLED , "asc_irq", NULL)) {debug_cec("FAIL : request irq asc\n");goto err;}

	f=get_pll_freq(CLOCKGEN_PLL1_CFG);
	f = (f / 4);
	debug_cec("f_system=%d\n",f);

//	t = BAUDRATE_VAL_M1(100000, f);	//10bit x 10us = 100us
	t = BAUDRATE_VAL_M1(33333, f);	//10bit x 30us = 300us

	debug_cec("baud=%d\n",t);
	writel(t,ASC_BAUDRATE);
	writel(0x1089,ASC_CTL);//8n1 off_fifo >> //0 1 0 0 0 0 1 0 0 01 001 =0x1089

	//cec_rx = stpio_request_pin (3, 7, "pin_cec_rx", STPIO_IN);
	//if (cec_rx==NULL) {debug_cec("FAIL : request pin 3 7\n");goto err;}
	//stpio_set_pin (cec_rx, 1);

//	if (request_irq(126, asc_irq ,IRQF_DISABLED , "asc_irq", NULL)) {debug_cec("FAIL : request irq asc\n");goto err;}

	debug_cec("CEC Init<<\n");

err:
  return 0;
}

static void __exit cleanup_cec_module(void)
{
    remove_e2_procs(e2_procs[0].name, e2_procs[0].read_proc, e2_procs[0].write_proc);
}

module_init(init_cec_module);
module_exit(cleanup_cec_module);

MODULE_DESCRIPTION("ADB_BOX CEC");
MODULE_AUTHOR("plfreebox@gmail.com");
MODULE_LICENSE("GPL");

