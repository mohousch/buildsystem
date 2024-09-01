ifneq (,$(findstring 2.6.3,$(KERNELVERSION)))
ccflags-y += $(CCFLAGSY)
else
CFLAGS += $(CCFLAGSY)
endif

EXTRA_CFLAGS += -O2 -DCONFIG_LITTLE_ENDIAN -Wno-unused-variable -Wno-unused-value 
EXTRA_CFLAGS += -Wno-unused-label -Wno-unused-parameter -Wno-uninitialized
EXTRA_CFLAGS += -I$(src)/include  -Wno-unused -Wno-unused-function

CONFIG_RTL8711			= n
CONFIG_RTL8712			= y

CONFIG_USB_HCI			= y
CONFIG_SDIO_HCI			= n
CONFIG_MP_INCLUDED		= y
CONFIG_MLME_EXT			= n
CONFIG_DRVEXT_MODULE	= n

ifeq ($(CONFIG_RTL8711), y)
RTL871X = rtl8711
MODULE_NAME = 8711
endif

ifeq ($(CONFIG_RTL8712), y)
RTL871X = rtl8712

ifeq ($(CONFIG_SDIO_HCI), y)
MODULE_NAME = 8712s
endif

ifeq ($(CONFIG_USB_HCI), y)
MODULE_NAME = 8712u
endif
endif

ifeq ($(CONFIG_SDIO_HCI), y)
 
_OS_INTFS_FILES := os_intf/osdep_service.o \
                   os_intf/linux/os_intfs.o \
                   os_intf/osdep_sdio_intf.o \
                   os_intf/linux/sdio_intf.o \

_HAL_INTFS_FILES := hal/$(RTL871X)/hal_init.o \
                    hal/$(RTL871X)/sdio_halinit.o \
                    hal/$(RTL871X)/sdio_ops.o \
                    hal/$(RTL871X)/sdio_ops_linux.o
endif

ifeq ($(CONFIG_USB_HCI), y)
 
_OS_INTFS_FILES := os_intf/osdep_service.o \
		   os_intf/linux/os_intfs.o \
		   os_intf/linux/usb_intf.o \


_HAL_INTFS_FILES := hal/$(RTL871X)/hal_init.o \
                    hal/$(RTL871X)/usb_ops.o \
                    hal/$(RTL871X)/usb_ops_linux.o \
                    hal/$(RTL871X)/usb_halinit.o \
		
endif

$(MODULE_NAME)-y += cmd/rtl871x_cmd.o \
                    cmd/$(RTL871X)_cmd.o

$(MODULE_NAME)-y += crypto/rtl871x_security.o 
$(MODULE_NAME)-y += debug/rtl871x_debug.o 

$(MODULE_NAME)-y += eeprom/rtl871x_eeprom.o \
                    efuse/rtl8712_efuse.o
                    
$(MODULE_NAME)-y += $(_HAL_INTFS_FILES)

$(MODULE_NAME)-y += io/rtl871x_io.o \
			        io/$(RTL871X)_io.o

$(MODULE_NAME)-y += ioctl/rtl871x_ioctl_query.o \
          			ioctl/rtl871x_ioctl_set.o \
	   				ioctl/rtl871x_ioctl_linux.o \
					ioctl/rtl871x_ioctl_rtl.o

$(MODULE_NAME)-y += led/rtl8712_led.o

$(MODULE_NAME)-y += mlme/ieee80211.o \
                    mlme/rtl871x_mlme.o
				
$(MODULE_NAME)-$(CONFIG_MLME_EXT) += mlme/rtl871x_mlme_ext.o \
                                     mlme/rtl871x_wlan_mlme.o \
                                     mlme/rtl871x_wlan_sme.o

$(MODULE_NAME)-$(CONFIG_MP_INCLUDED) += mp/rtl871x_mp.o \
                                        mp/rtl871x_mp_ioctl.o

$(MODULE_NAME)-y += os_dep/linux/io_linux.o \
					os_dep/linux/xmit_linux.o \
					os_dep/linux/cmd_linux.o \
					os_dep/linux/mlme_linux.o \
					os_dep/linux/recv_linux.o

$(MODULE_NAME)-y += $(_OS_INTFS_FILES)

$(MODULE_NAME)-y += pwrctrl/rtl871x_pwrctrl.o

$(MODULE_NAME)-y += recv/rtl871x_recv.o \
                    recv/$(RTL871X)_recv.o
                    
$(MODULE_NAME)-y += rf/rtl871x_rf.o \
                    rf/$(RTL871X)_rf.o
                    
$(MODULE_NAME)-y += sta_mgt/rtl871x_sta_mgt.o

$(MODULE_NAME)-y += xmit/rtl871x_xmit.o \
                    xmit/$(RTL871X)_xmit.o

obj-m := $(MODULE_NAME).o
