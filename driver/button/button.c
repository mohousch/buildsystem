/*
 * Kathrein UFS-910 frontpanel buttons driver - 20.Jun 2008 - captaintrip
 *
 * ToDo:
 * + Replace the bad polling with interrupt handling (IRQ5&13?)
 * + Find out how the wheel left/right and press works
 * + .... 
 *
 */

#include <linux/input.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/timer.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/workqueue.h>

#include "button.h"
#include "pio.h"

static char *button_driver_name = "Kathrein UFS-910 frontpanel buttons";
static struct input_dev *button_dev;
static unsigned char button_value = 0;
static int bad_polling = 1;

static struct workqueue_struct *wq;

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20)
void button_bad_polling(void)
#else
void button_bad_polling(struct work_struct *ignored)
#endif
{
	while(bad_polling == 1)
	{
		msleep(50);
		button_value = (STPIO_GET_PIN(PIO_PORT(1),0) <<2) | (STPIO_GET_PIN(PIO_PORT(1),1) <<1)  | (STPIO_GET_PIN(PIO_PORT(1),2) <<0);
		if (button_value != 7) {
			switch(button_value) {
				// v-format
				case 6: {
					input_report_key(button_dev, BTN_0, 1);
					input_sync(button_dev);
					break;
				}
				// menu
				case 4: {
					input_report_key(button_dev, BTN_1, 1);
					input_sync(button_dev);
					break;
				}
				// option
				case 3: {
					input_report_key(button_dev, BTN_2, 1);
					input_sync(button_dev);
					break;
				}
				// exit
				case 5: {
					input_report_key(button_dev, BTN_3, 1);
					input_sync(button_dev);
					break;
				}
				default:
					dprintk("[BTN] unknown button_value?");
			}
		}
		else {
			input_report_key(button_dev, BTN_0, 0);
			input_report_key(button_dev, BTN_1, 0);
			input_report_key(button_dev, BTN_2, 0);
			input_report_key(button_dev, BTN_3, 0);
			input_sync(button_dev);
		}
	}
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20)
static DECLARE_WORK(button_obj, button_bad_polling, NULL); 
#else
static DECLARE_WORK(button_obj, button_bad_polling); 
#endif
static int button_input_open(struct input_dev *dev)
{
	wq = create_workqueue("button");                   
	if(queue_work(wq, &button_obj))
	{                    
		dprintk("[BTN] queue_work successful ...\n");
	}
	else
	{
		dprintk("[BTN] queue_work not successful, exiting ...\n");
		return 1;
	}

	return 0;
}

static void button_input_close(struct input_dev *dev)
{
	bad_polling = 0;
	msleep(55);
	bad_polling = 1;

	if (wq)
	{
		destroy_workqueue(wq);                           
		dprintk("[BTN] workqueue destroyed\n");
	}
}

int button_dev_init(void)
{
	int error;

	dprintk("[BTN] allocating and registering button device\n");

	button_dev = input_allocate_device();
	if (!button_dev)
		return -ENOMEM;

	button_dev->name = button_driver_name;
	button_dev->open = button_input_open;
	button_dev->close = button_input_close;


	set_bit(EV_KEY, button_dev->evbit);
	set_bit(BTN_0, button_dev->keybit); 
	set_bit(BTN_1, button_dev->keybit); 
	set_bit(BTN_2, button_dev->keybit); 
	set_bit(BTN_3, button_dev->keybit); 

	error = input_register_device(button_dev);
	if (error) {
		input_free_device(button_dev);
		return error;
	}

	return 0;
}

void button_dev_exit(void)
{
	dprintk("[BTN] unregistering button device\n");
	input_unregister_device(button_dev);
}

int __init button_init(void)
{
	dprintk("[BTN] initializing ...\n");

	if(button_dev_init() != 0)
		return 1;

	return 0;
}

void __exit button_exit(void)
{
	dprintk("[BTN] unloading ...\n");

	button_dev_exit();
}

module_init(button_init);
module_exit(button_exit);

MODULE_DESCRIPTION("Kathrein UFS-910 frontpanel buttons driver");
MODULE_AUTHOR("Team Ducktales");
MODULE_LICENSE("GPL");
