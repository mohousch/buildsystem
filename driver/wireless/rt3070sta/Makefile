ifneq (,$(findstring 2.6.3,$(KERNELVERSION)))
ccflags-y += $(CCFLAGSY)
else
CFLAGS += $(CCFLAGSY)
endif
MODULE = $(word 1, $(CHIPSET))

RT28xx_MODE               = STA
CHIPSET                   = 3070
OSABL                     = NO
MODULE_DAT                = 2870

#ifdef CONFIG_STA_SUPPORT

ifeq ($(RT28xx_MODE), STA)
MOD_NAME = rt$(MODULE)sta
DAT_PATH = /etc/Wireless/RT$(MODULE_DAT)STA
DAT_FILE_NAME = RT$(MODULE_DAT)STA.dat
endif
#endif // CONFIG_STA_SUPPORT //

HAS_WPA_SUPPLICANT	  = y
HAS_NATIVE_WPA_SUPPLICANT_SUPPORT=y
HAS_DOT11_N_SUPPORT       = y
HAS_DOT11N_DRAFT3_SUPPORT = y
HAS_KTHREAD_SUPPORT       = n

EXTRA_CFLAGS += -I$(src)/include
EXTRA_CFLAGS += -Wall -O2 -Wundef -Wstrict-prototypes
EXTRA_CFLAGS += -Wno-trigraphs -Wdeclaration-after-statement 
EXTRA_CFLAGS += -Wno-pointer-sign -fno-strict-aliasing -fno-common 
EXTRA_CFLAGS += -fomit-frame-pointer -ffreestanding -m4-nofpu

WFLAGS := -DAGGREGATION_SUPPORT -DPIGGYBACK_SUPPORT -DWMM_SUPPORT  -DLINUX -Wall -Wstrict-prototypes -Wno-trigraphs 
WFLAGS += -DSYSTEM_LOG_SUPPORT  -DRT28xx_MODE=$(RT28xx_MODE) -DCHIPSET=$(MODULE)
WFLAGS += -DST -DCONFIG_STA_SUPPORT
# Support ATE function
HAS_ATE=n

# Support QA ATE function
HAS_QA_SUPPORT=n

HAS_RSSI_FEEDBACK=n

# Support XLINK mode
HAS_XLINK=n


HAS_NINTENDO=n

# Support LLTD function
HAS_LLTD=n

# Support WDS function
HAS_WDS=n

# Support AP-Client function
HAS_APCLI=n


#Support Net interface block while Tx-Sw queue full
HAS_BLOCK_NET_IF=n

#Support IGMP-Snooping function.
HAS_IGMP_SNOOP_SUPPORT=n

#Support DFS function
HAS_DFS_SUPPORT=n

#Support Carrier-Sense function
HAS_CS_SUPPORT=n


# Support user specific transmit rate of Multicast packet.
HAS_MCAST_RATE_SPECIFIC_SUPPORT=n

# Support for Multiple Cards
HAS_MC_SUPPORT=n

#Support for PCI-MSI
HAS_MSI_SUPPORT=n


#Support for IEEE802.11e DLS
HAS_QOS_DLS_SUPPORT=n

#Support for EXT_CHANNEL
HAS_EXT_BUILD_CHANNEL_LIST=n

#Support for IDS 
HAS_IDS_SUPPORT=n


#Support for Net-SNMP
HAS_SNMP_SUPPORT=n

#Support features of Single SKU. 
HAS_SINGLE_SKU_SUPPORT=n

#Support for 2860/2880 co-exist 
HAS_RT2880_RT2860_COEXIST=n





#Support for WiFi Display
HAS_WFD_SUPPORT=n

#Support for Auto channel select enhance
HAS_AUTO_CH_SELECT_ENHANCE=n

#Support statistics count
HAS_STATS_COUNT=y

#Support TSSI Antenna Variation
HAS_TSSI_ANTENNA_VARIATION=n

#Support USB_BULK_BUF_ALIGMENT
HAS_USB_BULK_BUF_ALIGMENT=n

#Support for USB_SUPPORT_SELECTIVE_SUSPEND
HAS_USB_SUPPORT_SELECTIVE_SUSPEND=n

#Support USB load firmware by multibyte
HAS_USB_FIRMWARE_MULTIBYTE_WRITE=n


#Support ANDROID_SUPPORT
HAS_ANDROID_SUPPORT=y

#HAS_IFUP_IN_PROBE_SUPPORT
HAS_IFUP_IN_PROBE_SUPPORT=n




#Support TXRX SW Antenna Diversity
HAS_TXRX_SW_ANTDIV_SUPPORT=n

#Client support WDS function
HAS_CLIENT_WDS_SUPPORT=n

#Support for Bridge Fast Path & Bridge Fast Path function open to other module
HAS_BGFP_SUPPORT=n
HAS_BGFP_OPEN_SUPPORT=n

# Support HOSTAPD function
HAS_HOSTAPD_SUPPORT=n

#Support GreenAP function
HAS_GREENAP_SUPPORT=n

#Support MAC80211 LINUX-only function
#Please make sure the version for CFG80211.ko and MAC80211.ko is same as the one
#our driver references to.
HAS_CFG80211_SUPPORT=n

#Support RFKILL hardware block/unblock LINUX-only function
HAS_RFKILL_HW_SUPPORT=y




HAS_APCLI_WPA_SUPPLICANT=n

HAS_RTMP_FLASH_SUPPORT=n

ifeq ($(OSABL),YES)
HAS_OSABL_FUNC_SUPPORT=y
HAS_OSABL_OS_PCI_SUPPORT=y
HAS_OSABL_OS_USB_SUPPORT=y
HAS_OSABL_OS_RBUS_SUPPORT=n
HAS_OSABL_OS_AP_SUPPORT=y
HAS_OSABL_OS_STA_SUPPORT=y
endif

HAS_LED_CONTROL_SUPPORT=y


#Support WIDI feature
#Must enable HAS_WSC at the same time.


HAS_STREAM_MODE_SUPPORT=n

HAS_NEW_RATE_ADAPT_SUPPORT=n



#RT5572 RT5592
HAS_TSO_SUPPORT=n



HAS_SWITCH_CHANNEL_OFFLOAD=n

HAS_RESOURCE_PRE_ALLOC=n
HAS_RESOURCE_BOOT_ALLOC=n

HAS_NEW_MBSSID_MODE=y

HAS_MULTI_CHANNEL=n
#################################################

ifeq ($(HAS_RESOURCE_PRE_ALLOC),y)
WFLAGS += -DRESOURCE_PRE_ALLOC
endif

ifeq ($(HAS_RESOURCE_BOOT_ALLOC),y)
WFLAGS += -DRESOURCE_BOOT_ALLOC  -I$(LINUX_SRC)/include/linux/usb -I$(LINUX_SRC)/drivers/usb/core
endif

#APsoc Specific
WFLAGS += -DCONFIG_RA_NAT_NONE
#end of /* APsoc Specific */






ifeq ($(HAS_KTHREAD_SUPPORT),y)
WFLAGS += -DKTHREAD_SUPPORT
endif

ifeq ($(HAS_RTMP_FLASH_SUPPORT),y)
WFLAGS += -DRTMP_FLASH_SUPPORT
endif

ifeq ($(HAS_STREAM_MODE_SUPPORT),y)
WFLAGS += -DSTREAM_MODE_SUPPORT
endif

ifeq ($(HAS_SINGLE_SKU_SUPPORT),y)
WFLAGS += -DSINGLE_SKU
endif

#################################################

# config for STA mode

ifeq ($(RT28xx_MODE),STA)
WFLAGS += -DCONFIG_STA_SUPPORT -DDBG

ifeq ($(HAS_XLINK),y)
WFLAGS += -DXLINK_SUPPORT
endif


ifeq ($(HAS_WPA_SUPPLICANT),y)
WFLAGS += -DWPA_SUPPLICANT_SUPPORT
ifeq ($(HAS_NATIVE_WPA_SUPPLICANT_SUPPORT),y)
WFLAGS += -DNATIVE_WPA_SUPPLICANT_SUPPORT
endif
endif




ifeq ($(HAS_ATE),y)
WFLAGS += -DRALINK_ATE
WFLAGS += -DCONFIG_RT2880_ATE_CMD_NEW
WFLAGS += -I$(RT28xx_DIR)/ate/include
ifeq ($(HAS_QA_SUPPORT),y)
WFLAGS += -DRALINK_QA
endif
endif


ifeq ($(HAS_SNMP_SUPPORT),y)
WFLAGS += -DSNMP_SUPPORT
endif

ifeq ($(HAS_USB_BULK_BUF_ALIGMENT),y)
WFLAGS += -DUSB_BULK_BUF_ALIGMENT -DALIGMENT_BULKAGGRE_SIZE=14 -DBUF_ALIGMENT_RINGSIZE=3
endif

ifeq ($(HAS_QOS_DLS_SUPPORT),y)
WFLAGS += -DQOS_DLS_SUPPORT
endif

ifeq ($(HAS_DOT11_N_SUPPORT),y)
WFLAGS += -DDOT11_N_SUPPORT

ifeq ($(HAS_DOT11N_DRAFT3_SUPPORT),y)
WFLAGS += -DDOT11N_DRAFT3
endif


ifeq ($(HAS_NEW_RATE_ADAPT_SUPPORT),y)
WFLAGS += -DNEW_RATE_ADAPT_SUPPORT
endif

endif


ifeq ($(HAS_WFD_SUPPORT),y)
HAS_P2P_SUPPORT=y
HAS_P2P_SPECIFIC_WIRELESS_EVENT=y
WFLAGS += -DWFD_SUPPORT
endif


ifeq ($(HAS_CS_SUPPORT),y)
WFLAGS += -DCARRIER_DETECTION_SUPPORT
endif

ifeq ($(HAS_STATS_COUNT),y)
WFLAGS += -DSTATS_COUNT_SUPPORT
endif

ifeq ($(HAS_TSSI_ANTENNA_VARIATION),y)
WFLAGS += -DTSSI_ANTENNA_VARIATION
endif

ifeq ($(HAS_ANDROID_SUPPORT),y)
WFLAGS += -DANDROID_SUPPORT
endif


ifeq ($(HAS_IFUP_IN_PROBE_SUPPORT),y)
WFLAGS += -DIFUP_IN_PROBE
endif

ifeq ($(HAS_USB_SUPPORT_SELECTIVE_SUSPEND),y)
WFLAGS += -DUSB_SUPPORT_SELECTIVE_SUSPEND
endif

ifeq ($(HAS_USB_FIRMWARE_MULTIBYTE_WRITE),y)
WFLAGS += -DUSB_FIRMWARE_MULTIBYTE_WRITE -DMULTIWRITE_BYTES=4
endif

ifeq ($(HAS_CFG80211_SUPPORT),y)
WFLAGS += -DRT_CFG80211_SUPPORT -DEXT_BUILD_CHANNEL_LIST
ifeq ($(HAS_RFKILL_HW_SUPPORT),y)
WFLAGS += -DRFKILL_HW_SUPPORT
endif
endif

ifeq ($(OSABL),YES)
WFLAGS += -DOS_ABL_SUPPORT
ifeq ($(HAS_OSABL_FUNC_SUPPORT),y)
WFLAGS += -DOS_ABL_FUNC_SUPPORT
endif
ifeq ($(HAS_OSABL_OS_PCI_SUPPORT),y)
WFLAGS += -DOS_ABL_OS_PCI_SUPPORT
endif
ifeq ($(HAS_OSABL_OS_USB_SUPPORT),y)
WFLAGS += -DOS_ABL_OS_USB_SUPPORT
endif
ifeq ($(HAS_OSABL_OS_RBUS_SUPPORT),y)
WFLAGS += -DOS_ABL_OS_RBUS_SUPPORT
endif
ifeq ($(HAS_OSABL_OS_AP_SUPPORT),y)
WFLAGS += -DOS_ABL_OS_AP_SUPPORT
endif
ifeq ($(HAS_OSABL_OS_STA_SUPPORT),y)
WFLAGS += -DOS_ABL_OS_STA_SUPPORT
endif
endif


ifeq ($(HAS_TXRX_SW_ANTDIV_SUPPORT),y)
WFLAGS += -DTXRX_SW_ANTDIV_SUPPORT
endif


ifeq ($(HAS_WIDI_SUPPORT),y)
WFLAGS += -DWIDI_SUPPORT

ifeq ($(HAS_INTEL_L2SD_TOGGLE_SCAN_SUPPORT),y)
WFLAGS += -DINTEL_L2SD_TOGGLE_SCAN_SUPPORT
endif

ifeq ($(HAS_P2P_SUPPORT),y)
ifeq ($(HAS_INTEL_WFD_SUPPORT),y)
WFLAGS += -DINTEL_WFD_SUPPORT
endif

ifeq ($(HAS_WFA_WFD_SUPPORT),y)
WFLAGS += -DWFA_WFD_SUPPORT
endif
endif

endif


ifeq ($(HAS_WOW_IFDOWN_SUPPORT),y)
WFLAGS += -DWOW_IFDOWN_SUPPORT
endif

endif
# endif of ifeq ($(RT28xx_MODE),STA)

#################################################

#################################################

#
# Common compiler flag
#






ifeq ($(HAS_EXT_BUILD_CHANNEL_LIST),y)
WFLAGS += -DEXT_BUILD_CHANNEL_LIST
endif

ifeq ($(HAS_IDS_SUPPORT),y)
WFLAGS += -DIDS_SUPPORT
endif


ifeq ($(OSABL),YES)
WFLAGS += -DEXPORT_SYMTAB
endif

ifeq ($(HAS_CLIENT_WDS_SUPPORT),y)
WFLAGS += -DCLIENT_WDS
endif

ifeq ($(HAS_BGFP_SUPPORT),y)
WFLAGS += -DBG_FT_SUPPORT
endif

ifeq ($(HAS_BGFP_OPEN_SUPPORT),y)
WFLAGS += -DBG_FT_OPEN_SUPPORT
endif


ifeq ($(HAS_LED_CONTROL_SUPPORT),y)
WFLAGS += -DLED_CONTROL_SUPPORT
endif

ifeq ($(HAS_SWITCH_CHANNEL_OFFLOAD), y)
WFLAGS += -DCONFIG_SWITCH_CHANNEL_OFFLOAD
endif


ifeq ($(HAS_MULTI_CHANNEL),y)
WFLAGS += -DCONFIG_SWITCH_CHANNEL_OFFLOAD -DCONFIG_MULTI_CHANNEL
endif

#################################################
# ChipSet specific definitions.
#
ifneq ($(findstring 2860,$(CHIPSET)),)
WFLAGS +=-DRTMP_MAC_PCI -DRTMP_PCI_SUPPORT -DRT2860 -DRT28xx -DA_BAND_SUPPORT
CHIPSET_DAT = 2860
endif

ifneq ($(findstring 3090,$(CHIPSET)),)
WFLAGS +=-DRTMP_MAC_PCI -DRT30xx -DRT3090  -DRTMP_PCI_SUPPORT -DRTMP_RF_RW_SUPPORT -DRTMP_EFUSE_SUPPORT -DVCORECAL_SUPPORT
CHIPSET_DAT = 2860
endif

ifneq ($(findstring 2870,$(CHIPSET)),)
WFLAGS +=-DRTMP_MAC_USB -DRTMP_USB_SUPPORT -DRT2870 -DRT28xx -DRTMP_TIMER_TASK_SUPPORT -DA_BAND_SUPPORT
CHIPSET_DAT = 2870
endif

ifneq ($(findstring 2070,$(CHIPSET)),)
WFLAGS +=-DRTMP_MAC_USB -DRT30xx -DRT3070 -DRT2070 -DRTMP_USB_SUPPORT -DRTMP_TIMER_TASK_SUPPORT -DRTMP_RF_RW_SUPPORT -DRTMP_EFUSE_SUPPORT
CHIPSET_DAT = 2870
endif

ifneq ($(findstring 3070,$(CHIPSET)),)
WFLAGS +=-DRTMP_MAC_USB -DRT30xx -DRT3070 -DRTMP_USB_SUPPORT -DRTMP_TIMER_TASK_SUPPORT -DRTMP_RF_RW_SUPPORT -DRTMP_EFUSE_SUPPORT -DVCORECAL_SUPPORT
CHIPSET_DAT = 2870

ifneq ($(findstring $(RT28xx_MODE),AP),)
ifeq ($(HAS_CS_SUPPORT), y)
WFLAGS += -DCARRIER_DETECTION_FIRMWARE_SUPPORT 
endif
endif

endif

ifneq ($(findstring 2880,$(CHIPSET)),)
WFLAGS += -DRT2880 -DRT28xx -DRTMP_MAC_PCI -DCONFIG_RALINK_RT2880_MP -DRTMP_RBUS_SUPPORT -DMERGE_ARCH_TEAM -DA_BAND_SUPPORT -DCONFIG_SWMCU_SUPPORT
ifeq ($(HAS_WIFI_LED_SHARE), y)
WFLAGS += -DCONFIG_WIFI_LED_SHARE
endif
endif

ifneq ($(findstring 3572,$(CHIPSET)),)
WFLAGS +=-DRTMP_MAC_USB -DRTMP_USB_SUPPORT -DRT2870 -DRT28xx -DRT30xx -DRT35xx -DRT3572 -DRTMP_TIMER_TASK_SUPPORT -DRTMP_RF_RW_SUPPORT -DRTMP_EFUSE_SUPPORT -DA_BAND_SUPPORT -DVCORECAL_SUPPORT
CHIPSET_DAT = 2870

ifneq ($(findstring $(RT28xx_MODE),AP),)
ifeq ($(HAS_CS_SUPPORT), y)
WFLAGS +=  -DCARRIER_DETECTION_FIRMWARE_SUPPORT
endif
endif

endif

ifneq ($(findstring 3573,$(CHIPSET)),)
WFLAGS += -DRTMP_MAC_USB -DRTMP_USB_SUPPORT -DRT30xx -DRT35xx -DRT3593 -DRT3573\
          -DRTMP_TIMER_TASK_SUPPORT -DRTMP_RF_RW_SUPPORT -DRTMP_EFUSE_SUPPORT\
          -DA_BAND_SUPPORT -DDOT11N_SS3_SUPPORT \
		  -DVCORECAL_SUPPORT

ifeq ($(HAS_NEW_MBSSID_MODE),y)
WFLAGS += -DNEW_MBSSID_MODE
endif

#WFLAGS += -DNEW_RATE_ADAPT_SUPPORT
CHIPSET_DAT = 2870

ifneq ($(findstring $(RT28xx_MODE),STA APSTA),)
WFLAGS += -DRTMP_FREQ_CALIBRATION_SUPPORT
endif

ifneq ($(findstring $(RT28xx_MODE),AP),)
WFLAGS += -DSPECIFIC_BCN_BUF_SUPPORT
endif

ifneq ($(findstring $(RT28xx_MODE),AP),)
ifeq ($(HAS_CS_SUPPORT), y)
WFLAGS +=  -DCARRIER_DETECTION_FIRMWARE_SUPPORT
endif
endif

endif

ifneq ($(findstring 3062,$(CHIPSET)),)
WFLAGS +=-DRTMP_MAC_PCI -DRT2860 -DRT28xx -DRT30xx -DRT35xx -DRT3062 -DRTMP_PCI_SUPPORT -DRTMP_RF_RW_SUPPORT -DRTMP_EFUSE_SUPPORT -DVCORECAL_SUPPORT
CHIPSET_DAT = 2860
endif

ifneq ($(findstring 3562,$(CHIPSET)),)
WFLAGS +=-DRTMP_MAC_PCI -DRT2860 -DRT28xx -DRT30xx -DRT35xx -DRT3562 -DRTMP_PCI_SUPPORT -DRTMP_RF_RW_SUPPORT -DRTMP_EFUSE_SUPPORT -DA_BAND_SUPPORT -DVCORECAL_SUPPORT

CHIPSET_DAT = 2860
endif

ifneq ($(findstring 3593,$(CHIPSET)),)
WFLAGS +=-DRTMP_MAC_PCI -DDOT11N_SS3_SUPPORT -DRT3593 -DRT28xx -DRT30xx -DRT35xx -DRTMP_PCI_SUPPORT -DRTMP_RF_RW_SUPPORT -DRTMP_EFUSE_SUPPORT -DA_BAND_SUPPORT -DVCORECAL_SUPPORT

ifeq ($(HAS_NEW_MBSSID_MODE),y)
WFLAGS += -DNEW_MBSSID_MODE
endif

#WFLAGS += -DNEW_RATE_ADAPT_SUPPORT
CHIPSET_DAT = 2860

ifneq ($(findstring $(RT28xx_MODE),STA APSTA),)
WFLAGS += -DRTMP_FREQ_CALIBRATION_SUPPORT
endif

ifneq ($(findstring $(RT28xx_MODE),AP),)
WFLAGS += -DSPECIFIC_BCN_BUF_SUPPORT
endif
endif

ifneq ($(findstring 3390,$(CHIPSET)),)
WFLAGS +=-DRTMP_MAC_PCI -DRT30xx -DRT33xx -DRT3090 -DRT3390 -DRTMP_PCI_SUPPORT -DRTMP_RF_RW_SUPPORT -DRTMP_EFUSE_SUPPORT -DRTMP_INTERNAL_TX_ALC -DVCORECAL_SUPPORT
CHIPSET_DAT = 2860
endif

ifneq ($(findstring 3370,$(CHIPSET)),)
WFLAGS +=-DRTMP_MAC_USB -DRT30xx -DRT33xx -DRT3070 -DRT3370 -DRTMP_USB_SUPPORT -DRTMP_TIMER_TASK_SUPPORT -DRTMP_RF_RW_SUPPORT -DRTMP_EFUSE_SUPPORT -DRTMP_INTERNAL_TX_ALC -DVCORECAL_SUPPORT
CHIPSET_DAT = 2870

ifneq ($(findstring $(RT28xx_MODE),AP),)
ifeq ($(HAS_CS_SUPPORT), y)
WFLAGS +=  -DCARRIER_DETECTION_FIRMWARE_SUPPORT
endif
endif

endif

ifneq ($(findstring 5390,$(CHIPSET)),)
WFLAGS +=-DRTMP_MAC_PCI -DRT30xx -DRT33xx -DRT3090 -DRT3390 -DRT5390 -DRTMP_PCI_SUPPORT -DRTMP_RF_RW_SUPPORT -DRTMP_EFUSE_SUPPORT -DRTMP_FREQ_CALIBRATION_SUPPORT -DRTMP_INTERNAL_TX_ALC -DVCORECAL_SUPPORT -DIQ_CAL_SUPPORT -DRTMP_TEMPERATURE_COMPENSATION

ifeq ($(HAS_NEW_MBSSID_MODE),y)
WFLAGS += -DNEW_MBSSID_MODE
endif

CHIPSET_DAT = 2860

ifneq ($(findstring $(RT28xx_MODE),AP),)
WFLAGS += -DSPECIFIC_BCN_BUF_SUPPORT
endif
endif

ifneq ($(findstring 5370,$(CHIPSET)),)
WFLAGS +=-DRTMP_MAC_USB -DRT30xx -DRT33xx -DRT3070 -DRT3370 -DRT5370 -DRTMP_USB_SUPPORT -DRTMP_TIMER_TASK_SUPPORT -DRTMP_RF_RW_SUPPORT -DRTMP_EFUSE_SUPPORT -DRTMP_INTERNAL_TX_ALC -DRTMP_FREQ_CALIBRATION_SUPPORT -DVCORECAL_SUPPORT -DIQ_CAL_SUPPORT -DRTMP_TEMPERATURE_COMPENSATION

ifeq ($(HAS_NEW_MBSSID_MODE),y)
WFLAGS += -DNEW_MBSSID_MODE
endif

CHIPSET_DAT = 2870

ifneq ($(findstring $(RT28xx_MODE),AP),)
ifeq ($(HAS_CS_SUPPORT), y)
WFLAGS +=  -DCARRIER_DETECTION_FIRMWARE_SUPPORT
endif
endif

ifneq ($(findstring $(RT28xx_MODE),AP),)
WFLAGS += -DSPECIFIC_BCN_BUF_SUPPORT
endif
endif

ifneq ($(findstring 3052,$(CHIPSET)),)
WFLAGS += -DRTMP_MAC_PCI -DRTMP_RBUS_SUPPORT -DRT3052 -DRT305x -DRTMP_RF_RW_SUPPORT -DCONFIG_SWMCU_SUPPORT -DVCORECAL_SUPPORT
CHIPSET_DAT = 2870
ifeq ($(HAS_WIFI_LED_SHARE), y)
WFLAGS += -DCONFIG_WIFI_LED_SHARE
endif
endif

ifneq ($(findstring 3352,$(CHIPSET)),)
WFLAGS += -DRTMP_MAC_PCI -DRTMP_RBUS_SUPPORT -DRT3352 -DRT305x -DRTMP_RF_RW_SUPPORT -DVCORECAL_SUPPORT -DCONFIG_SWMCU_SUPPORT -DRTMP_INTERNAL_TX_ALC

ifeq ($(HAS_NEW_MBSSID_MODE),y)
WFLAGS += -DNEW_MBSSID_MODE
endif

CHIPSET_DAT = 2860
ifeq ($(HAS_WIFI_LED_SHARE), y)
WFLAGS += -DCONFIG_WIFI_LED_SHARE
endif
ifneq ($(findstring $(RT28xx_MODE),AP),)
WFLAGS += -DSPECIFIC_BCN_BUF_SUPPORT
endif
endif

ifneq ($(findstring 5350,$(CHIPSET)),)
WFLAGS += -DRTMP_MAC_PCI -DRTMP_RBUS_SUPPORT -DRT5350 -DRT305x -DRT3050 -DRT3350 -DRTMP_RF_RW_SUPPORT -DVCORECAL_SUPPORT -DCONFIG_SWMCU_SUPPORT -DRTMP_INTERNAL_TX_ALC -DRTMP_FREQ_CALIBRATION_SUPPORT -DIQ_CAL_SUPPORT

ifeq ($(HAS_NEW_MBSSID_MODE),y)
WFLAGS += -DNEW_MBSSID_MODE
endif

CHIPSET_DAT = 2860
ifeq ($(HAS_WIFI_LED_SHARE), y)
WFLAGS += -DCONFIG_WIFI_LED_SHARE
endif
ifneq ($(findstring $(RT28xx_MODE),AP),)
WFLAGS += -DSPECIFIC_BCN_BUF_SUPPORT
endif
endif

ifneq ($(findstring 5592,$(CHIPSET)),)
WFLAGS += -DRTMP_MAC_PCI -DRTMP_PCI_SUPPORT -DRT30xx -DRT5592\
		  -DRTMP_RF_RW_SUPPORT -DRTMP_EFUSE_SUPPORT\
		  -DA_BAND_SUPPORT -DIQ_CAL_SUPPORT -DRX_DMA_SCATTER -DVCORECAL_SUPPORT\
          -DRTMP_TEMPERATURE_COMPENSATION

ifeq ($(HAS_NEW_MBSSID_MODE),y)
WFLAGS += -DNEW_MBSSID_MODE
endif

CHIPSET_DAT = 2860
ifeq ($(HAS_TSO_SUPPORT), y)
WFLAGS += -DCONFIG_TSO_SUPPORT
endif

ifneq ($(findstring $(RT28xx_MODE),STA APSTA),)
WFLAGS += -DRTMP_FREQ_CALIBRATION_SUPPORT
endif

ifneq ($(findstring $(RT28xx_MODE),AP),)
WFLAGS += -DSPECIFIC_BCN_BUF_SUPPORT
endif

endif

ifneq ($(findstring 5572,$(CHIPSET)),)
WFLAGS += -DRTMP_MAC_USB -DRTMP_USB_SUPPORT -DRT30xx -DRT5572 -DRT5592\
		  -DRTMP_RF_RW_SUPPORT -DRTMP_EFUSE_SUPPORT \
		  -DRTMP_TIMER_TASK_SUPPORT -DA_BAND_SUPPORT -DIQ_CAL_SUPPORT -DVCORECAL_SUPPORT\
		  -DRTMP_TEMPERATURE_COMPENSATION
CHIPSET_DAT = 2870
ifeq ($(HAS_TSO_SUPPORT), y)
WFLAGS += -DCONFIG_TSO_SUPPORT
endif

ifneq ($(findstring $(RT28xx_MODE),AP),)
ifeq ($(HAS_CS_SUPPORT), y)
WFLAGS +=  -DCARRIER_DETECTION_FIRMWARE_SUPPORT 
endif
endif

ifneq ($(findstring $(RT28xx_MODE),STA APSTA),)
WFLAGS += -DRTMP_FREQ_CALIBRATION_SUPPORT
endif

ifneq ($(findstring $(RT28xx_MODE),AP),)
#WFLAGS += -DSPECIFIC_BCN_BUF_SUPPORT
endif

endif

ifneq ($(findstring 3290,$(CHIPSET)),)
WFLAGS += -DRTMP_MAC_PCI -DRTMP_PCI_SUPPORT -DRTMP_RF_RW_SUPPORT -DRTMP_EFUSE_SUPPORT -DRTMP_FREQ_CALIBRATION_SUPPORT -DRTMP_INTERNAL_TX_ALC -DRT30xx -DRT3290 -DVCORECAL_SUPPORT
CHIPSET_DAT = 2860

ifneq ($(findstring $(RT28xx_MODE),STA APSTA),)
WFLAGS += -DRTMP_FREQ_CALIBRATION_SUPPORT -DPCIE_PS_SUPPORT
endif

ifneq ($(findstring $(RT28xx_MODE),AP),)
WFLAGS += -DSPECIFIC_BCN_BUF_SUPPORT
endif
endif


ifeq ($(CHIPSET),USB)
#3572
WFLAGS +=-DRTMP_MAC_USB -DRTMP_USB_SUPPORT -DRT2870 -DRT28xx -DRT30xx -DRT35xx -DRTMP_TIMER_TASK_SUPPORT -DRTMP_RF_RW_SUPPORT -DRTMP_EFUSE_SUPPORT -DA_BAND_SUPPORT -DSPECIFIC_VCORECAL_SUPPORT
#3370
WFLAGS += -DRT33xx -DRT3070 -DRT3370 -DRTMP_TIMER_TASK_SUPPORT -DRTMP_INTERNAL_TX_ALC
CHIPSET_DAT = 2870
endif


ifeq ($(CHIPSET),PCI)
#3562		
WFLAGS +=-DRTMP_MAC_PCI -DRT2860 -DRT28xx -DRT30xx -DRT35xx -DRTMP_PCI_SUPPORT -DRTMP_RF_RW_SUPPORT -DRTMP_EFUSE_SUPPORT -DA_BAND_SUPPORT -DSPECIFIC_VCORECAL_SUPPORT
#3390
WFLAGS +=-DRT33xx -DRT3090 -DRT3390 -DRTMP_INTERNAL_TX_ALC
endif


ifeq ($(CHIPSET),RBUS)
WFLAGS += -DMERGE_ARCH_TEAM -DCONFIG_SWMCU_SUPPORT -DCONFIG_RA_NAT_NONE -DRTMP_RBUS_SUPPORT
#5350, 3050, 3350, 3883
WFLAGS +=-DRTMP_MAC_PCI -DRT305x -DRT5350 -DRT3050 -DRT3350 -DRT3883 -DRTMP_PCI_SUPPORT -DRTMP_RF_RW_SUPPORT -DA_BAND_SUPPORT -DVCORECAL_SUPPORT

ifneq ($(findstring $(RT28xx_MODE),AP),)
WFLAGS += -DSPECIFIC_BCN_BUF_SUPPORT
endif
#WFLAGS += -DDBG_CTRL_SUPPORT 
#WFLAGS += -DINCLUDE_DEBUG_QUEUE
#WFLAGS += -DRANGE_EXTEND -DCFO_TRACK -DPRE_ANT_SWITCH
endif


#################################################


ifeq ($(HAS_WPA_SUPPLICANT),y)
WFLAGS += -DWPA_SUPPLICANT_SUPPORT
ifeq ($(HAS_NATIVE_WPA_SUPPLICANT_SUPPORT),y)
WFLAGS += -DNATIVE_WPA_SUPPLICANT_SUPPORT -DCREDENTIAL_STORE -DPROFILE_STORE
endif
endif

ifeq ($(OSABL),YES)
HAS_OSABL_FUNC_SUPPORT=y
HAS_OSABL_OS_PCI_SUPPORT=y
HAS_OSABL_OS_USB_SUPPORT=y
HAS_OSABL_OS_RBUS_SUPPORT=n
HAS_OSABL_OS_AP_SUPPORT=y
HAS_OSABL_OS_STA_SUPPORT=y
endif


ifeq ($(HAS_ATE),y)
WFLAGS += -DRALINK_ATE
WFLAGS += -DCONFIG_RT2880_ATE_CMD_NEW
ifeq ($(HAS_NEW_TXCONT),y)
WFLAGS += -DNEW_TXCONT
endif
ifeq ($(HAS_QA_SUPPORT),y)
WFLAGS += -DRALINK_QA
endif
endif


ifeq ($(HAS_SNMP_SUPPORT),y)
WFLAGS += -DSNMP_SUPPORT
endif

ifeq ($(HAS_QOS_DLS_SUPPORT),y)
WFLAGS += -DQOS_DLS_SUPPORT
endif

ifeq ($(HAS_DOT11_N_SUPPORT),y)
WFLAGS += -DDOT11_N_SUPPORT
ifeq ($(HAS_DOT11N_DRAFT3_SUPPORT),y)
WFLAGS += -DDOT11N_DRAFT3
endif
endif


ifeq ($(HAS_CS_SUPPORT),y)
WFLAGS += -DCARRIER_DETECTION_SUPPORT
endif

ifeq ($(HAS_STATS_COUNT),y)
WFLAGS += -DSTATS_COUNT_SUPPORT
endif

ifeq ($(HAS_USB_SUPPORT_SELECTIVE_SUSPEND),y)
WFLAGS += -DUSB_SUPPORT_SELECTIVE_SUSPEND -DCONFIG_PM
endif


ifeq ($(HAS_CFG80211_SUPPORT),y)
WFLAGS += -DRT_CFG80211_SUPPORT
ifeq ($(HAS_RFKILL_HW_SUPPORT),y)
WFLAGS += -DRFKILL_HW_SUPPORT
endif
endif

EXTRA_CFLAGS += -o $(WFLAGS) 

obj-m := $(MOD_NAME).o


#ifdef CONFIG_STA_SUPPORT
ifeq ($(RT28xx_MODE), STA)
$(MOD_NAME)-objs := \
	common/crypt_md5.o\
	common/crypt_sha2.o\
	common/crypt_hmac.o\
	common/crypt_aes.o\
	common/crypt_arc4.o\
	common/mlme.o\
	common/cmm_wep.o\
	common/action.o\
	common/cmm_data.o\
	common/rtmp_init.o\
	common/rtmp_init_inf.o\
	common/cmm_tkip.o\
	common/cmm_aes.o\
	common/cmm_sync.o\
	common/eeprom.o\
	common/cmm_sanity.o\
	common/cmm_info.o\
	common/cmm_cfg.o\
	common/cmm_wpa.o\
	common/cmm_radar.o\
	common/spectrum.o\
	common/rtmp_timer.o\
	common/rt_channel.o\
	common/cmm_profile.o\
	common/cmm_asic.o\
	common/cmm_cmd.o\
	common/ps.o\
	common/uapsd.o\
	rate_ctrl/ra_ctrl.o\
	rate_ctrl/alg_legacy.o\
	rate_ctrl/alg_ags.o\
	os/linux/rt_profile.o\
	chips/rtmp_chip.o\
	sta/assoc.o\
	sta/auth.o\
	sta/auth_rsp.o\
	sta/sync.o\
	sta/sanity.o\
	sta/rtmp_data.o\
	sta/connect.o\
	sta/wpa.o\
	sta/sta_cfg.o

ifeq ($(OSABL),NO)
$(MOD_NAME)-objs += \
	common/rt_os_util.o\
	os/linux/sta_ioctl.o\
	os/linux/rt_linux.o\
	os/linux/rt_main_dev.o
else
$(MOD_NAME)-objs += \
	os/linux/rt_symb.o
endif

ifeq ($(HAS_NEW_RATE_ADAPT_SUPPORT),y)
$(MOD_NAME)-objs += rate_ctrl/alg_grp.o
endif

#ifdef DOT11_N_SUPPORT
ifeq ($(HAS_DOT11_N_SUPPORT),y)
$(MOD_NAME)-objs += \
	common/ba_action.o

endif
#endif // DOT11_N_SUPPORT //

#ifdef ETH_CONVERT
ifeq ($(HAS_ETH_CONVERT_SUPPORT), y)
$(MOD_NAME)-objs += \
	common/cmm_mat.o \
	common/cmm_mat_iparp.o \
	common/cmm_mat_pppoe.o \
	common/cmm_mat_ipv6.o
endif
#endif // ETH_CONVERT //





ifeq ($(HAS_BLOCK_NET_IF),y)
$(MOD_NAME)-objs += common/netif_block.o
endif

ifeq ($(HAS_QOS_DLS_SUPPORT),y)
$(MOD_NAME)-objs += sta/dls.o
endif




#ifdef LED_CONTROL_SUPPORT
ifeq ($(HAS_LED_CONTROL_SUPPORT),y)
$(MOD_NAME)-objs += \
	common/rt_led.o
endif
#endif // LED_CONTROL_SUPPORT //


ifeq ($(HAS_ATE),y)
$(MOD_NAME)-objs += ate/common/rt_ate.o
endif

ifeq ($(HAS_QA_SUPPORT),y)
$(MOD_NAME)-objs += ate/common/rt_qa.o
endif







#ifdef CRDA_SUPPORT
ifeq ($(OSABL),NO)
ifeq ($(HAS_CFG80211_SUPPORT),y)
$(MOD_NAME)-objs += \
	os/linux/cfg80211.o \
	os/linux/cfg80211drv.o
endif
endif

ifeq ($(OSABL),YES)
ifeq ($(HAS_CFG80211_SUPPORT),y)
$(MOD_NAME)-objs += \
	os/linux/cfg80211drv.o
endif
endif
#endif // CRDA_SUPPORT //


ifeq ($(HAS_DFS_SUPPORT),y)
$(MOD_NAME)-objs += \
        common/cmm_dfs.o
endif

ifeq ($(HAS_CS_SUPPORT),y)
$(MOD_NAME)-objs += \
        common/cmm_cs.o
endif

endif
#endif // CONFIG_STA_SUPPORT //



#chip releated



#ifdef RT2870
ifneq ($(findstring 2870,$(CHIPSET)),)
$(MOD_NAME)-objs += \
	common/cmm_mac_usb.o\
	common/rtusb_io.o\
	common/rtusb_data.o\
	common/cmm_data_usb.o\
	common/rtmp_mcu.o\
	common/rtusb_bulk.o\
	os/linux/rt_usb.o\
	common/ee_prom.o\
	chips/rt28xx.o\

ifeq ($(HAS_ATE),y)
$(MOD_NAME)-objs += \
	ate/chips/rt28xx_ate.o\
	ate/common/ate_usb.o
endif

ifeq ($(OSABL),NO)
$(MOD_NAME)-objs += \
	common/rtusb_dev_id.o\
	os/linux/rt_usb_util.o\
	os/linux/usb_main_dev.o
endif
endif
#endif // RT2870 //

#ifdef RT3070
ifneq ($(findstring 2070,$(CHIPSET)),)
$(MOD_NAME)-objs += \
	common/cmm_mac_usb.o\
	common/rtusb_io.o\
	common/rtusb_data.o\
	common/cmm_data_usb.o\
	common/rtusb_bulk.o\
	common/rtmp_mcu.o\
	common/ee_prom.o\
	common/ee_efuse.o\
	chips/rt30xx.o\
	common/rt_rf.o\
	os/linux/rt_usb.o\
	chips/rt3070.o

ifeq ($(HAS_ATE),y)
$(MOD_NAME)-objs += \
	ate/chips/rt30xx_ate.o\
	ate/common/ate_usb.o
endif

ifeq ($(OSABL),NO)
$(MOD_NAME)-objs += \
	common/rtusb_dev_id.o\
	os/linux/rt_usb_util.o\
	os/linux/usb_main_dev.o
endif
endif

ifneq ($(findstring 3070,$(CHIPSET)),)
$(MOD_NAME)-objs += \
	common/cmm_mac_usb.o\
	common/rtusb_io.o\
	common/rtusb_data.o\
	common/cmm_data_usb.o\
	common/rtusb_bulk.o\
	common/rtmp_mcu.o\
	common/ee_prom.o\
	common/ee_efuse.o\
	common/rt_rf.o\
	os/linux/rt_usb.o\
	chips/rt30xx.o\
	chips/rt3070.o

ifeq ($(HAS_ATE),y)
$(MOD_NAME)-objs += \
	ate/chips/rt30xx_ate.o\
	ate/common/ate_usb.o
endif

ifeq ($(OSABL),NO)
$(MOD_NAME)-objs += \
	common/rtusb_dev_id.o\
	os/linux/rt_usb_util.o\
	os/linux/usb_main_dev.o
endif
endif
#endif // RT3070 //







#ifdef RT3572
ifneq ($(findstring 3572,$(CHIPSET)),)
$(MOD_NAME)-objs += \
	common/cmm_mac_usb.o\
	common/rtusb_io.o\
	common/rtusb_data.o\
	common/cmm_data_usb.o\
	common/rtmp_mcu.o\
	common/ee_prom.o\
	common/ee_efuse.o\
	common/rt_rf.o\
	common/rtusb_bulk.o\
	os/linux/rt_usb.o\
	chips/rt28xx.o\
	chips/rt30xx.o\
	chips/rt35xx.o

ifeq ($(HAS_RTMP_FLASH_SUPPORT),y)
$(MOD_NAME)-objs += \
        common/ee_flash.o
endif

ifeq ($(HAS_ATE),y)
$(MOD_NAME)-objs += \
	ate/chips/rt28xx_ate.o\
	ate/chips/rt35xx_ate.o\
	ate/common/ate_usb.o
endif

ifeq ($(OSABL),NO)
$(MOD_NAME)-objs += \
	common/rtusb_dev_id.o\
	os/linux/rt_usb_util.o\
	os/linux/usb_main_dev.o
endif

endif
#endif // RT3572 //




#ifdef RT3573
ifneq ($(findstring 3573,$(CHIPSET)),)
$(MOD_NAME)-objs += \
	common/cmm_mac_usb.o\
	common/rtusb_io.o\
	common/rtusb_data.o\
	common/cmm_data_usb.o\
	common/rtmp_mcu.o\
	common/ee_prom.o\
	common/ee_efuse.o\
	common/rt_rf.o\
	common/rtusb_bulk.o\
	os/linux/rt_usb.o\
	chips/rt28xx.o\
	chips/rt30xx.o\
	chips/rt35xx.o\
	chips/rt3593.o

ifeq ($(HAS_ATE),y)
$(MOD_NAME)-objs += \
	ate/chips/rt30xx_ate.o\
	ate/chips/rt35xx_ate.o\
	ate/chips/rt3593_ate.o\
	ate/common/ate_usb.o
endif

ifeq ($(OSABL),NO)
$(MOD_NAME)-objs += \
	common/rtusb_dev_id.o\
	os/linux/rt_usb_util.o\
	os/linux/usb_main_dev.o
endif

ifneq ($(findstring STA,$(RT28xx_MODE)),)
$(MOD_NAME)-objs += \
	common/frq_cal.o
endif

endif
#endif // RT3573 //



#ifdef RT3370
ifneq ($(findstring 3370,$(CHIPSET)),)
$(MOD_NAME)-objs += \
	common/cmm_mac_usb.o\
	common/rtusb_io.o\
	common/rtusb_data.o\
	common/cmm_data_usb.o\
	common/rtmp_mcu.o\
	common/ee_prom.o\
	common/ee_efuse.o\
	common/rt_rf.o\
	common/rtusb_bulk.o\
	os/linux/rt_usb.o\
	chips/rt33xx.o\
	chips/rt3370.o\
	chips/rt3070.o\
	chips/rt30xx.o

ifeq ($(HAS_ATE),y)
$(MOD_NAME)-objs += \
	ate/chips/rt33xx_ate.o\
	ate/chips/rt30xx_ate.o\
	ate/common/ate_usb.o
endif

ifeq ($(OSABL),NO)
$(MOD_NAME)-objs += \
	common/rtusb_dev_id.o\
	os/linux/rt_usb_util.o\
	os/linux/usb_main_dev.o
endif
endif
#endif // RT3370 //

#ifdef RT5370
ifneq ($(findstring 5370,$(CHIPSET)),)
$(MOD_NAME)-objs += \
        common/cmm_mac_usb.o\
        common/rtusb_io.o\
        common/rtusb_data.o\
        common/cmm_data_usb.o\
        common/rtmp_mcu.o\
        common/ee_prom.o\
        common/ee_efuse.o\
        common/rt_rf.o\
        common/rtusb_bulk.o\
        os/linux/rt_usb.o\
        chips/rt33xx.o\
        chips/rt3370.o\
        chips/rt3070.o\
        chips/rt30xx.o\
        chips/rt5390.o

ifeq ($(HAS_ATE),y)
$(MOD_NAME)-objs += \
	ate/chips/rt5390_ate.o\
	ate/chips/rt30xx_ate.o\
	ate/chips/rt33xx_ate.o\
	ate/common/ate_usb.o
endif

ifeq ($(OSABL),NO)
$(MOD_NAME)-objs += \
        common/rtusb_dev_id.o\
        os/linux/rt_usb_util.o\
        os/linux/usb_main_dev.o
endif

ifneq ($(findstring STA,$(RT28xx_MODE)),)
$(MOD_NAME)-objs += \
	common/frq_cal.o
endif

endif
#endif // RT5370 //


#ifdef RT3290
ifneq ($(findstring 3290,$(CHIPSET)),)
$(MOD_NAME)-objs += \
	common/cmm_mac_pci.o\
	common/cmm_data_pci.o\
	os/linux/rt_rbus_pci_drv.o\
	common/rtmp_mcu.o\
	common/ee_prom.o\
	common/ee_efuse.o\
	common/rt_rf.o\
	chips/rt30xx.o\
	chips/rt3290.o

ifeq ($(OSABL),NO)
$(MOD_NAME)-objs += \
        os/linux/rt_pci_rbus.o\
        os/linux/rt_rbus_pci_util.o\
        os/linux/pci_main_dev.o
endif

ifneq ($(findstring STA,$(RT28xx_MODE)),)
$(MOD_NAME)-objs += \
	common/frq_cal.o
endif

endif
#endif // RT3290 //

#ifdef RT5572
ifneq ($(findstring 5572,$(CHIPSET)),)
$(MOD_NAME)-objs += \
		common/cmm_mac_usb.o\
		common/rtusb_io.o\
		common/rtusb_data.o\
		common/cmm_data_usb.o\
		common/rtmp_mcu.o\
		common/ee_prom.o\
		common/ee_efuse.o\
		common/rt_rf.o\
		common/rtusb_bulk.o\
		os/linux/rt_usb.o\
		chips/rt30xx.o\
		chips/rt5592.o

ifeq ($(HAS_RTMP_FLASH_SUPPORT),y)
$(MOD_NAME)-objs += \
        common/ee_flash.o
endif

ifeq ($(HAS_ATE),y)
$(MOD_NAME)-objs += \
	ate/chips/rt5592_ate.o\
	ate/common/ate_usb.o
endif

ifeq ($(OSABL),NO)
$(MOD_NAME)-objs += \
		common/rtusb_dev_id.o\
		os/linux/rt_usb_util.o\
		os/linux/usb_main_dev.o
endif

ifneq ($(findstring STA,$(RT28xx_MODE)),)
$(MOD_NAME)-objs += \
	common/frq_cal.o
endif

endif
#endif
#endif // RT5572 //