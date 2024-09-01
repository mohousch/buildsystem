TREE_ROOT?=$(DRIVER_TOPDIR)/
CONFIG_STGFB_PATH?=$(DRIVER_TOPDIR)/stgfb/stmfb

CCFLAGSY += -I$(TREE_ROOT)/player2/linux/include/linux/stm/
CCFLAGSY += -I$(TREE_ROOT)/player2/linux/drivers/media/video/stm/
CCFLAGSY += -I$(KERNEL_LOCATION)/drivers/media/dvb/dvb-core
CCFLAGSY += -I$(CONFIG_STGFB_PATH)/include
CCFLAGSY += -I$(TREE_ROOT)/include
CCFLAGSY += -DDEBUG_DVB


ifdef SPARK
CCFLAGSY += -I$(TREE_ROOT)/frontends/spark_dvbapi5
CCFLAGSY += -I$(TREE_ROOT)/frontends/spark_common
endif

ifdef SPARK7162
CCFLAGSY += -I$(TREE_ROOT)/frontends/spark7162
CCFLAGSY += -I$(TREE_ROOT)/frontends/spark7162/stv0367
CCFLAGSY += -I$(TREE_ROOT)/frontends/spark7162/stv0367/stv0367qam
CCFLAGSY += -I$(TREE_ROOT)/frontends/spark7162/stv0367/stv0367ter
CCFLAGSY += -I$(TREE_ROOT)/frontends/spark7162/6158
CCFLAGSY += -I$(TREE_ROOT)/frontends/spark7162/tuner
CCFLAGSY += -I$(TREE_ROOT)/frontends/spark7162/tuner/MxL301_Ext
CCFLAGSY += -I$(TREE_ROOT)/frontends/spark7162/tuner/MxL603
CCFLAGSY += -I$(TREE_ROOT)/frontends/spark_dvbapi5
CCFLAGSY += -I$(TREE_ROOT)/frontends/spark_common

CCFLAGSY += -Wall -DCONFIG_DVB_DUMMY_FE #-Werror

CCFLAGSY += -DSTB_BOARD_TYPE=1
CCFLAGSY += -DFULAN_3606_220_1CA_1CI=0
CCFLAGSY += -DFULAN_3606_220_SIMPLE=0
CCFLAGSY += -DSYS_TUN_MODULE=1
CCFLAGSY += -DMXL603=1
CCFLAGSY += -DANY_TUNER=0
endif

ccflags-y += $(CCFLAGSY)

# >= 2.6.30
ifdef HL101
fe-core-objs += hl101/stb0899_algo.o
fe-core-objs += hl101/stb0899_drv.o
fe-core-objs += hl101/stb6100.o
fe-core-objs += hl101/stv090x.o
fe-core-objs += hl101/stv6110x.o
fe-core-objs += hl101/ix7306.o
fe-core-objs += hl101/core.o
obj-m += fe-core.o
endif

ifdef VIP1_V2
fe-core-objs += vip2/stv090x.o
fe-core-objs += vip2/stv6110x.o
fe-core-objs += vip2/ix7306.o
fe-core-objs += vip2/zl10353.o
fe-core-objs += vip2/core.o
obj-m += fe-core.o
endif

ifdef VIP2_V1
fe-core-objs += vip2/stv090x.o
fe-core-objs += vip2/stv6110x.o
fe-core-objs += vip2/ix7306.o
fe-core-objs += vip2/zl10353.o
fe-core-objs += vip2/core.o
obj-m += fe-core.o
endif

ifdef SPARK
stv090x-objs += spark_dvbapi5/stv090x.o
stv090x-objs += spark_dvbapi5/stv6110x.o
stv090x-objs += spark_dvbapi5/ix7306.o
stv090x-objs += spark_dvbapi5/core.o
stv090x-objs += spark_common/vz7903.o
obj-m += stv090x.o
obj-y += lnb/
endif

ifdef SPARK7162
spark7162-objs += spark7162/spark7162.o
spark7162-objs += spark_common/ioarch.o
spark7162-objs += spark7162/D3501.o

spark7162-objs += spark_dvbapi5/ix7306.o
spark7162-objs += spark_common/vz7903.o
spark7162-objs += spark_common/dvb_dummy_fe.o
spark7162-objs += spark7162/base/sharp7803.o
spark7162-objs += spark7162/base/sharp6465.o
spark7162-objs += spark7162/base/sharp5469c.o

spark7162-objs += spark7162/stv0367/D0367.o
spark7162-objs += spark7162/stv0367/D0367ter.o
spark7162-objs += spark7162/stv0367/D0367cab.o

spark7162-objs += spark7162/stv0367/stv0367ter/chip_0367ter.o
spark7162-objs += spark7162/stv0367/stv0367ter/stv0367ofdm_drv.o
spark7162-objs += spark7162/stv0367/stv0367ter/d0367_ter.o

spark7162-objs += spark7162/stv0367/stv0367qam/chip_0367qam.o
spark7162-objs += spark7162/stv0367/stv0367qam/stv0367qam_drv.o
spark7162-objs += spark7162/stv0367/stv0367qam/d0367_qam.o

spark7162-objs += spark7162/6158/d6158.o
spark7162-objs += spark7162/6158/d6158_dvb.o
spark7162-objs += spark7162/6158/nim_panic6158.o
spark7162-objs += spark7162/6158/MN88472_autoctrl.o
spark7162-objs += spark7162/6158/MN88472_register.o
spark7162-objs += spark7162/6158/MN88472_variable.o

spark7162-objs += spark7162/tuner/MxL301_Ext/mxl301rf.o
spark7162-objs += spark7162/tuner/MxL301_Ext/mxl_common.o
spark7162-objs += spark7162/tuner/MxL301_Ext/mxl_spur_table.o
spark7162-objs += spark7162/tuner/MxL301_Ext/mxL_user_define.o
spark7162-objs += spark7162/tuner/MxL301_Ext/tun_mxl301.o

spark7162-objs += spark7162/tuner/MxL603/MxL603_TunerApi.o
spark7162-objs += spark7162/tuner/MxL603/MxL603_TunerCfg.o
spark7162-objs += spark7162/tuner/MxL603/tun_mxl603.o

obj-m += spark7162.o
endif

ifdef UFS912
stv090x-objs += stv090x/stv090x.o
stv090x-objs += stv090x/stv6110x.o
stv090x-objs += stv090x/ix7306.o
stv090x-objs += stv090x/core.o
obj-m += stv090x.o
endif

ifdef UFS913
obj-y += multituner/
endif

ifdef ATEMIO520
stv090x-objs += stv090x/stv090x.o
stv090x-objs += stv090x/stv6110x.o
stv090x-objs += stv090x/ix7306.o
stv090x-objs += stv090x/core.o
obj-m += stv090x.o
obj-y += lnb/
endif

ifdef ATEMIO530
stv090x-objs += stv090x/stv090x.o
stv090x-objs += stv090x/stv6110x.o
stv090x-objs += stv090x/ix7306.o
stv090x-objs += stv090x/core.o
obj-m += stv090x.o
obj-y += lnb/
endif

ifdef HS7110
stv090x-objs += stv090x/stv090x.o
stv090x-objs += stv090x/stv6110x.o
stv090x-objs += stv090x/ix7306.o
stv090x-objs +=stv090x/core.o
obj-m += stv090x.o
obj-y += lnb/
endif 

ifdef HS7119
stv090x-objs += stv090x/stv090x.o
stv090x-objs += stv090x/stv6110x.o
stv090x-objs += stv090x/ix7306.o
stv090x-objs +=stv090x/core.o
obj-m += stv090x.o
obj-y += lnb/
endif

ifdef HS7420
stv090x-objs += stv090x/stv090x.o
stv090x-objs += stv090x/stv6110x.o
stv090x-objs += stv090x/ix7306.o
stv090x-objs += stv090x/core.o
obj-m += stv090x.o
obj-y += lnb/
endif

ifdef HS7429
stv090x-objs += stv090x/stv090x.o
stv090x-objs += stv090x/stv6110x.o
stv090x-objs += stv090x/ix7306.o
stv090x-objs +=stv090x/core.o
obj-m += stv090x.o
obj-y += lnb/
endif 

ifdef HS7810A
stv090x-objs += stv090x/stv090x.o
stv090x-objs += stv090x/stv6110x.o
stv090x-objs += stv090x/ix7306.o
stv090x-objs += stv090x/core.o
obj-m += stv090x.o
obj-y += lnb/
endif

ifdef HS7819
stv090x-objs += stv090x/stv090x.o
stv090x-objs += stv090x/stv6110x.o
stv090x-objs += stv090x/ix7306.o
stv090x-objs +=stv090x/core.o
obj-m += stv090x.o
obj-y += lnb/
endif 

ifdef FORTIS_HDBOX
stv090x-objs += stv090x/stv090x.o
stv090x-objs += stv090x/stv6110x.o
stv090x-objs += stv090x/ix7306.o
stv090x-objs += stv090x/core.o
obj-m += stv090x.o
endif

ifdef UFS922
cx21143-objs += cx24116/cx24116.o
obj-m += cx21143.o
ifeq (,$(wildcard $(DRIVER_TOPDIR)/frontends/avl2108_ufs922 ))
avl2108-objs += avl2108/avl2108.o
avl2108-objs += avl2108/stv6110a.o
avl2108-objs += avl2108/stv6306.o
avl2108-objs += avl2108/lnb_pio.o
avl2108-objs += avl2108/lnbh221.o
obj-m += avl2108.o
avl2108_platform-objs += avl2108/avl2108_platform.o
obj-m += avl2108_platform.o
else
avl2108-objs += avl2108_ufs922/avl2108.o
obj-m += avl2108.o
endif
endif

ifdef UFC960
tda10023-objs +=  ufc960/tda10023.o
tda10023-objs += ufc960/core.o
obj-m += tda10023.o
endif

ifdef UFS910
cx24116-objs += cx24116_dvbapi5/cx24116.o
obj-m += cx24116.o
endif

ifdef TF7700
cx24116-objs += cx24116_dvbapi5/cx24116.o
obj-m += cx24116.o
endif

ifdef OCTAGON1008
avl2108-objs += avl2108/avl2108.o
avl2108-objs += avl2108/stv6110a.o
avl2108-objs += avl2108/stv6306.o
avl2108-objs += avl2108/lnb_pio.o
avl2108-objs += avl2108/lnbh221.o
obj-m += avl2108.o

avl2108_platform-objs += avl2108/avl2108_platform.o
obj-m += avl2108_platform.o
endif

ifdef ATEVIO7500
#avl2108-objs += avl2108/avl2108.o
#avl2108-objs += avl2108/stv6110a.o
#avl2108-objs += avl2108/stv6306.o
#avl2108-objs += avl2108/lnb_pio.o
#avl2108-objs += avl2108/lnbh221.o
#obj-m += avl2108.o
#avl2108_platform-objs += avl2108/avl2108_platform.o
#obj-m += avl2108_platform.o
obj-y += multituner/
endif

ifdef IPBOX9900
fe-core-objs +=  ipbox/stv090x.o
fe-core-objs += ipbox/stv6110x.o
fe-core-objs += ipbox/zl10353.o
fe-core-objs += ipbox/mxl5007t.o
fe-core-objs += ipbox/tda10023.o
fe-core-objs += ipbox/core.o
obj-m += fe-core.o
endif

ifdef IPBOX99
fe-core-objs += ipbox/stv090x.o
fe-core-objs += ipbox/stv6110x.o
fe-core-objs += ipbox/zl10353.o
fe-core-objs += ipbox/mxl5007t.o
fe-core-objs += ipbox/tda10023.o
fe-core-objs += ipbox/core.o
obj-m += fe-core.o
endif

ifdef IPBOX55
fe-core-objs += ipbox/stv090x.o
fe-core-objs += ipbox/stv6110x.o
fe-core-objs += ipbox/core.o
obj-m += fe-core.o
endif

ifdef CUBEREVO
obj-y += multituner/
endif

ifdef CUBEREVO_MINI
obj-y += multituner/
endif

ifdef CUBEREVO_MINI2
obj-y += multituner/
endif

ifdef CUBEREVO_250HD
obj-y += multituner/
endif

ifdef CUBEREVO_9500HD
obj-y += multituner/
endif

ifdef CUBEREVO_2000HD
obj-y += multituner/
endif

ifdef CUBEREVO_MINI_FTA
obj-y += multituner/
endif

ifdef CUBEREVO_3000HD
obj-y += multituner/
endif

ifdef ADB_BOX
# Single ----------------------------------
# stb0899
fe-core-objs += adb/stb0899_algo.o
fe-core-objs += adb/stb0899_drv.o
fe-core-objs += adb/stb6100.o
fe-core-objs += adb/core.o
obj-m += fe-core.o

# Dual -------------------------------------
# stb0899
fe-core-twin1-objs += adb_dual/stb0899_algo.o
fe-core-twin1-objs += adb_dual/stb0899_drv.o
fe-core-twin1-objs += adb_dual/stb6100.o
fe-core-twin1-objs +=  adb_dual/core.o
obj-m += fe-core-twin1.o

# stv0900
fe-core-twin2-objs += adb_stv900/stv090x.o
fe-core-twin2-objs += adb_stv900/stb6100.o
fe-core-twin2-objs += adb_stv900/core.o
obj-m += fe-core-twin2.o

# DVB-C ----------------------------------
# stv0297 + mt2060
fe-mmp-objs += adb_mmp/mt2060.o
fe-mmp-objs += adb_mmp/stv0297.o
fe-mmp-objs += adb_mmp/core.o
obj-m += fe-mmp.o

#------------------------------------------------- 
endif

ifdef VITAMIN_HD5000
stv090x-objs += vitamin_hd5000/stv090x.o
stv090x-objs += vitamin_hd5000/stv6110x.o
stv090x-objs += vitamin_hd5000/core.o
obj-m += stv090x.o
endif

ifdef ARIVALINK200
cx24116-objs += cx24116_dvbapi5/cx24116.o
obj-m += cx24116.o
endif

ifdef SAGEMCOM88
ifneq (,$(wildcard $(DRIVER_TOPDIR)/frontends/sagemcom88 ))
fe_core-objs += sagemcom88/tda18218.o
fe_core-objs += sagemcom88/tda10048.o
fe_core-objs += sagemcom88/stv090x.o
fe_core-objs += sagemcom88/stv6110x.o
fe_core-objs += sagemcom88/core.o
obj-m += fe_core.o
endif
endif

ifdef PACE7241
fe_core-objs += pace7241/tda18218.o
fe_core-objs += pace7241/tda10048.o
fe_core-objs += pace7241/stv090x.o
fe_core-objs += pace7241/stv6110x.o
fe_core-objs += pace7241/core.o
obj-m += fe_core.o
endif

