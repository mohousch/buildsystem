ifeq ($(CONFIG_VIDEO_DEV),)
$(warning V4L2 not configured in kernel, skipping V4L2 modules)
else
stmvout-objs := stmvout_buffers.o                             \
		stmvout_display.o                             \
		stmvout_ctrl.o                                \
		stmvout_driver.o

stmvbi-objs := stmvbi_driver.o

obj-m += stm_v4l2.o
obj-m += stmvout.o
obj-m += stmvbi.o
endif
