	Because spark7167 tuner'i2c and boot flash'spi share the common GPIO.
so i2s.ko is used to change the i2c to spi interface.
	after insmod i2s.ko kernel will print as follow:

i2s_init > 
ssc_i2c_dev_release...
i2s_init hdk7105_configure_ssc_spi 
m25p80 spi0.20: en25f16 (2048 Kbytes)
Creating 1 MTD partitions on "m25p80":
0x000000000000-0x000000100000 : "uboot"
spi-stm spi-stm.0: registered SPI Bus 0
i2s_init <

	then you can use flashcp to upgrade u-boot.bin

flashcp -v u-boot.bin /dev/mtd7
	
	after upgrade u-boot.bin you must reboot, because the tuner'function was abolished.

