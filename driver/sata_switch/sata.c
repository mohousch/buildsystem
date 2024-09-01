/*
 * sata.c
 * 
 * nit 06.01.2013
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

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/proc_fs.h>

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,17)
#include <linux/stm/pio.h>
#else
#include <linux/stpio.h>
#endif

#ifdef DEBUG
#define dprintk(fmt, args...) printk(fmt, ##args)
#else
#define dprintk(fmt, args...)
#endif

struct stpio_pin* sata_pin = NULL;
static struct proc_dir_entry *sata_dir, *sata;

static int read_sata(char *page, char **start, off_t off, int count, int *eof, void *data)
{
  int len = 0;

  if(sata_pin != NULL)
    len += sprintf(page + len, "%d\n", stpio_get_pin(sata_pin));

  return len;
}

static int write_sata(struct file *file, const char *buffer, unsigned long count, void *data)
{
  int i = 0;

  i = simple_strtoul(buffer, NULL, 10);

  if(sata_pin != NULL)
  {
    if(i == 0)
      stpio_set_pin(sata_pin, 0); //extern
    else
      stpio_set_pin(sata_pin, 1); //intern
  }

  return count;
}

int __init sata_init(void)
{
  dprintk("[SATA] initializing ...\n");

  sata_dir = proc_mkdir("sata", NULL);
  if(sata_dir == NULL)
    return -ENOMEM;

  sata = create_proc_entry("sata_switch", 0644, sata_dir);
  sata->read_proc = read_sata;
  sata->write_proc = write_sata;

  sata_pin = stpio_request_pin(15, 3, "sata_switch", STPIO_OUT);

  return 0;
}

void __exit sata_exit(void)
{
  dprintk("[SATA] unloading ...\n");
  remove_proc_entry("sata_switch", sata_dir);
  //remove_proc_entry("sata", &proc_root);

  if(sata_pin != NULL) stpio_free_pin(sata_pin);
  sata_pin = NULL;
}

module_init(sata_init);
module_exit(sata_exit);

MODULE_DESCRIPTION("Get/Set SATA PIO");
MODULE_AUTHOR("nit");
MODULE_LICENSE("GPL");
