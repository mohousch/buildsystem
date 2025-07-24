depsclean:
	( cd $(D) && find . ! -name "*\.*" -delete )

clean: depsclean
	@echo -e "$(TERM_YELLOW)---> cleaning everything except toolchain.$(TERM_NORMAL)"
	@-$(MAKE) kernel-clean
	@-$(MAKE) driver-clean
	@-$(MAKE) tools-clean
	@-rm -rf $(IMAGE_DIR)
	@-rm -rf $(PKGS_DIR)
	@-rm -rf $(RELEASE_DIR)
	@-rm -rf $(TARGET_DIR)
	@-rm -rf $(SOURCE_DIR)
	@-rm -rf $(BUILD_TMP)
	@-rm -rf $(HOST_DIR)
	@-rm -rf $(D)/*.do_compile
	@-rm -rf $(D)/*.do_prepare
	@-rm -rf $(D)/*.config.status
ifeq ($(BOXARCH), sh4)
	@touch $(D)/crosstool-rpminstall
endif
	@echo -e "$(TERM_YELLOW)done\n$(TERM_NORMAL)"

distclean:
	@echo -e "$(TERM_YELLOW)---> cleaning whole build system ... $(TERM_NORMAL)"
	@-rm -rf $(IMAGE_DIR)
	@-rm -rf $(RELEASE_DIR)
	@-rm -rf $(TARGET_DIR)
	@-rm -rf $(HOST_DIR)
	@-rm -rf $(CROSS_DIR)
	@-rm -rf $(BUILD_TMP)
	@-rm -rf $(SOURCE_DIR)
	@-rm -rf $(PKGS_DIR)
	@-rm -rf $(D)
	@echo -e "$(TERM_YELLOW)done\n$(TERM_NORMAL)"

%-clean:
	( cd $(D) && find . -name $(subst -clean,,$@) -delete )

#
# driver-clean
#
driver-clean:
ifeq ($(BOXARCH), sh4)
	$(MAKE) -C $(DRIVER_DIR) ARCH=sh KERNEL_LOCATION=$(KERNEL_DIR) distclean
else
	rm -f $(TARGET_DIR)/lib/modules/$(KERNEL_VER)/extra/*
endif
	rm -f $(D)/driver

#
# kernel-clean
#
kernel-clean:
	-$(MAKE) -C $(KERNEL_DIR) clean
	rm -f $(D)/kernel
	rm -f $(D)/kernel.do_compile
	rm -f $(D)/kernel.do_prepare
	rm -f $(TARGET_DIR)/boot/*

#
# build clean
#
build-clean:
	rm -rf $(TUFSBOX_DIR)

