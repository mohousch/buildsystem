ifneq (,$(findstring 2.6.3,$(KERNELVERSION)))
ccflags-y += $(CCFLAGSY)
else
CFLAGS += $(CCFLAGSY)
endif

obj-m 		+= clk.o
clk-objs	+= clk_main.o clk_ufs912.o clk_ufs922.o clk_spark.o clk_hs7110.o clk_hs7810a.o clk_hs7119.o clk_hs7819.o
