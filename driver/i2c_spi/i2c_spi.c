#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/leds.h>
#include <linux/lirc.h>
#include <linux/gpio.h>
#include <linux/phy.h>
#include <linux/tm1668.h>
#include <linux/stm/platform.h>
#include <linux/stm/stx7105.h>
// #include <linux/stm/pci-synopsys.h>
#include <linux/stm/emi.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/physmap.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/partitions.h>
#include <linux/spi/spi.h>
#include <linux/spi/flash.h>
#include <linux/spi/spi_gpio.h>
#include <asm/irq-ilc.h>

void hdk7105_unconfigure_ssc_i2c(void);
void hdk7105_configure_ssc_spi(void);
#if 0
void hdk7105_spi_register(void);

static struct platform_device spi_pio_device[] = {
	{
		.name           = "spi_gpio",
		.id             = 0,
		.num_resources  = 0,
		.dev            = {
			.platform_data =
				&(struct spi_gpio_platform_data) {
					stm_gpio(2, 5),
					stm_gpio(2, 6),
					stm_gpio(2, 7),
					1,
				},
		},
	},
};
#endif  /* 0 */

int __init i2s_init(void)
{
    printk("%s > \n", __func__);
    //printk("%s hdk7105_unconfigure_ssc_i2c \n", __func__);
	hdk7105_unconfigure_ssc_i2c();
    //printk("%s hdk7105_spi_register \n", __func__);
	//hdk7105_spi_register();
    printk("%s hdk7105_configure_ssc_spi \n", __func__);
	hdk7105_configure_ssc_spi();

	//platform_device_register(&spi_pio_device[0]);
	printk("%s < \n", __func__);
    return 0;
}
static void i2s_cleanup(void)
{
    printk("%s >\n", __func__);
	//platform_device_unregister(&spi_pio_device[0]);
    printk("%s <\n", __func__);
}

module_init(i2s_init);
module_exit(i2s_cleanup);

MODULE_LICENSE("GPL");

