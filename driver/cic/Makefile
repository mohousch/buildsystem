TREE_ROOT?=$(DRIVER_TOPDIR)/player2
CONFIG_STGFB_PATH?=$(DRIVER_TOPDIR)/stgfb/stmfb
CCFLAGSY += -I$(TREE_ROOT)/linux/include/linux/stm/
CCFLAGSY += -I$(TREE_ROOT)/linux/drivers/media/video/stm/
CCFLAGSY += -I$(KERNEL_LOCATION)/drivers/media/dvb/dvb-core
CCFLAGSY += -I$(CONFIG_STGFB_PATH)/include
CCFLAGSY += -DDEBUG_DVB
ifneq (,$(findstring 2.6.3,$(KERNELVERSION)))
ccflags-y += $(CCFLAGSY)
else
CFLAGS += $(CCFLAGSY)
endif

ifdef UFS910
cimax-objs += cimax/cimax.o
obj-m += cimax.o
endif

ifdef UFS922
ufs922_cic-objs += ufs9xx_cic/ufs9xx_cic.o
obj-m += ufs922_cic.o
endif

ifdef UFC960
ufc960_cic-objs += ufs9xx_cic/ufs9xx_cic.o
obj-m += ufc960_cic.o
endif

ifdef UFS912
ufs912_cic-objs += ufs9xx_cic/ufs9xx_cic.o
obj-m += ufs912_cic.o
endif

ifdef UFS913
ufs913_cic-objs += ufs9xx_cic/ufs9xx_cic.o
obj-m += ufs913_cic.o
endif

ifdef FORTIS_HDBOX
obj-m += starci.o
starci-objs:= starci2win/starci2win.o
endif

ifdef OCTAGON1008
cimax-objs += cimax/cimax.o
obj-m += cimax.o
endif

ifdef VIP1_V2
cimax-objs += cimax/cimax.o
obj-m += cimax.o
endif

ifdef TF7700
obj-m += starci.o
starci-objs:= starci2win/starci2win.o
endif

ifdef CUBEREVO
obj-m += starci.o
starci-objs:= starci2win/starci2win.o
endif

ifdef CUBEREVO_MINI
obj-m += starci.o
starci-objs:= starci2win/starci2win.o
endif

ifdef CUBEREVO_MINI2
obj-m += starci.o
starci-objs:= starci2win/starci2win.o
endif

ifdef CUBEREVO_250HD
obj-m += starci.o
starci-objs:= starci2win/starci2win.o
endif

ifdef CUBEREVO_9500HD
obj-m += starci.o
starci-objs:= starci2win/starci2win.o
endif

ifdef CUBEREVO_2000HD
obj-m += starci.o
starci-objs:= starci2win/starci2win.o
endif

ifdef CUBEREVO_MINI_FTA
obj-m += starci.o
starci-objs:= starci2win/starci2win.o
endif

ifdef CUBEREVO_3000HD
obj-m += starci.o
starci-objs:= starci2win/starci2win.o
endif

ifdef HL101
cimax-objs += cimax/cimax.o
obj-m += cimax.o
endif

ifdef ATEVIO7500
obj-m += starci.o
starci-objs:= starci2win/starci2win.o
endif

ifdef HS7110
obj-m += hs711x.o
hs711x-objs:= hs711x/hs711x_cic.o
endif

ifdef HS7119
obj-m += hs711x.o
hs711x-objs:= hs711x/hs711x_cic.o
endif

ifdef HS7420
obj-m += starci.o
starci-objs:= starci2win/starci2win.o
endif

ifdef HS7429
obj-m += starci.o
starci-objs:= starci2win/starci2win.o
endif

ifdef HS7810A
obj-m += starci.o
starci-objs:= starci2win/starci2win.o
endif

ifdef HS7819
obj-m += starci.o
starci-objs:= starci2win/starci2win.o
endif

ifdef ATEMIO520
obj-m += cnbox_cic.o
cnbox_cic-objs:= cnbox_cic/cnbox_cic.o
endif

ifdef ATEMIO530
obj-m += cnbox_cic.o
cnbox_cic-objs:= cnbox_cic/cnbox_cic.o
endif

ifdef SPARK
dummy_cic-objs += dummy_cic/dummy_cic.o
obj-m += dummy_cic.o
endif

ifdef IPBOX9900
obj-m += starci.o
starci-objs:= ipbox9900/starci2win.o
endif

ifdef IPBOX99
dummy_cic-objs += dummy_cic/dummy_cic.o
obj-m += dummy_cic.o
endif

ifdef IPBOX55
dummy_cic-objs += dummy_cic/dummy_cic.o
obj-m += dummy_cic.o
endif

ifdef VITAMIN_HD5000
obj-m += starci.o
starci-objs:= vitamin_hd5000/starci2win.o
endif

ifdef ARIVALINK200
dummy_cic-objs += dummy_cic/dummy_cic.o
obj-m += dummy_cic.o
endif
