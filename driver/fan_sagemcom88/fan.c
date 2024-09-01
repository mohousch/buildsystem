/*
 * ufs922_fan.c
 * 
 * (c) 2009 Dagobert@teamducktales
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or 
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

/* 
 * Description:
 *
 * ufs922 fan controller controlling driver
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
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,17)
#include <linux/stm/pio.h>
#else
#include <linux/stpio.h>
#endif


int remove_e2_procs(char *path, read_proc_t *read_func, write_proc_t *write_func);
int install_e2_procs(char *path, read_proc_t *read_func, write_proc_t *write_func, void *data);

unsigned long fan_registers;
struct stpio_pin* fan_pin;
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,17)
struct stpio_pin* fan_pin1;
#endif

int proc_fan_write(struct file *file, const char __user *buf,
                           unsigned long count, void *data)
{
	char 		*page;
	char		*myString;
	ssize_t 	ret = -ENOMEM;
	
	printk("%s %d - ", __FUNCTION__, (int) count);

	page = (char *)__get_free_page(GFP_KERNEL);
	if (page) 
	{
		unsigned int value;
		
		ret = -EFAULT;
		if (copy_from_user(page, buf, count))
			goto out;

		myString = (char *) kmalloc(count + 1, GFP_KERNEL);
		strncpy(myString, page, count);
		myString[count] = '\0';

		sscanf(myString, "%d", &value);

		printk(" PWM0=%d\n", value);

		//maruapp: 0xff = 1500, 0xaa = 1000, 0x73 = 500 U/min
		//we are a little bit more nervy ;)
//		if (value >= 50 && value <= 255)
		if (value <= 255)
		{
		    ctrl_outl(value, fan_registers + 0x00);//0x04-pwm1 0x00-pwm0
		}

		kfree(myString);
	}
	
	ret = count;
out:
	
	free_page((unsigned long)page);
	return ret;
}

int proc_fan_read(char *page, char **start, off_t off, int count,
			  int *eof, void *data_unused)
{
	int len = 0;
        unsigned int value;
	
	printk("%s\n", __FUNCTION__);

	value = ctrl_inl(fan_registers + 0x4);
	len = sprintf(page, "%d\n", value);
	
        return len;
}


struct e2_procs
{
  char *name;
  read_proc_t *read_proc;
  write_proc_t *write_proc;
} e2_procs[] =
{
  {"stb/fan/fan_ctrl", proc_fan_read, proc_fan_write}
};

#define SysConfigBaseAddress    0xFE001000

static int __init init_fan_module(void)
{
	int ret;
	void *reg_sys_config = NULL;
	reg_sys_config = ioremap(SysConfigBaseAddress, 0x1000);

	install_e2_procs(e2_procs[0].name, e2_procs[0].read_proc, e2_procs[0].write_proc, NULL);

	fan_registers = (unsigned long) ioremap(0xfd010000, 0x68);
	printk("fan_registers = 0x%.8lx\n", fan_registers);

	fan_pin = stpio_request_pin (4, 4, "fan ctrl", STPIO_ALT_OUT);
	if (fan_pin==NULL) printk("FAN: Request pio failed !!!\n");

	//alternate 3 - PIO4-4 PWM0
	ret = ctrl_inl(reg_sys_config + 0x188);
	ret = ret & ~(1<<4);//0
	ret = ret | (1<<(4+8));//1
	ctrl_outl(ret, reg_sys_config + 0x188); // sys_cfg34

//  ctrl_outl(0x200, fan_registers + 0x50);
  ctrl_outl(0x1a04, fan_registers + 0x50);//500us
  
  ctrl_outl(0xff, fan_registers + 0x00);//0x04-pwm1 0x00-pwm0

  return 0;
}


static void __exit cleanup_fan_module(void)
{
    remove_e2_procs(e2_procs[0].name, e2_procs[0].read_proc, e2_procs[0].write_proc);


//#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,17) 
//    if (fan_pin1 != NULL)
//      stpio_free_pin (fan_pin1);
//      
//    if (fan_pin != NULL) {
//    	stpio_set_pin(fan_pin, 0);
//    	stpio_free_pin (fan_pin);
//    }	
//#else      
    if (fan_pin != NULL)
	stpio_free_pin (fan_pin);
//#endif

}

module_init(init_fan_module);
module_exit(cleanup_fan_module);

MODULE_DESCRIPTION("FAN ESI88");
MODULE_AUTHOR("Team Ducktales mod Nemo");
MODULE_LICENSE("GPL");

