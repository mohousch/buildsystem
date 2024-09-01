/*
    Driver for the Topfield TF7700HDPVR Scart switch (on the rear side).

    This driver provides the status of the Scart switch on the rear side
    via the proc entry /proc/tfswitch (read-only). "0" corresponds to the
    upper position ("Scart") and "1" corresponds to the lower position
    ("YPbPr").

    Copyright (C) 2010 arccos.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/


#include <linux/module.h>
#include <linux/version.h>
#include <linux/proc_fs.h>
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,17)
#include <linux/stm/pio.h>
#else
#include <linux/stpio.h>
#endif

#define PROC_ENTRY_NAME "tfswitch"

static struct proc_dir_entry *pEntry;
static struct stpio_pin *pPin;

static int tfswitch_read_proc(char *page, char **start, off_t off, int count,
                  int *eof, void *data_unused)
{
  int cnt = 0;

  if((page != NULL) && (count > 1))
  {
    page[0] = (char)stpio_get_pin(pPin) + 0x30;
    page[1] = '\n';
    cnt = 2;
  }

  return cnt;
}

static int __init tfswitch_init_module(void)
{
  printk("Loading TF7700 switch driver\n");

  /* create a proc entry and set the read function */
  pEntry = create_proc_entry(PROC_ENTRY_NAME, 0, NULL);
  if(pEntry == NULL)
  {
    printk("TF7700 switch driver: cannot allocate proc entry\n");
    return 1;
  }
  pEntry->read_proc = tfswitch_read_proc;

  /* allocate the I/O pin */
  pPin = stpio_request_pin(4, 4, "rear switch", STPIO_IN);
  if(pEntry == NULL)
  {
    printk("TF7700 switch driver: cannot allocate pin\n");
    remove_proc_entry(PROC_ENTRY_NAME, pEntry->parent);
    return 1;
  }

  return 0;
}

static void __exit tfswitch_cleanup_module(void)
{
  printk("Removing TF7700 switch driver\n");
  remove_proc_entry(PROC_ENTRY_NAME, pEntry->parent);
  stpio_free_pin(pPin);
}

module_init(tfswitch_init_module);
module_exit(tfswitch_cleanup_module);

MODULE_AUTHOR("arccos");
MODULE_DESCRIPTION("TF7700 switch driver");
MODULE_LICENSE("GPL");
