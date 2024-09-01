EXTRA_CFLAGS := -g -Wall

obj-m += \
	lzo-kmod/lzo1x_compress.o \
	lzo-kmod/lzo1x_decompress.o \
	ramzswap.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	@rm -rf *.ko
	@rm -rf lzo-kmod/*.ko
