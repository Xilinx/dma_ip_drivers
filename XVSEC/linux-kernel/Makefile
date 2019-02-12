SHELL = /bin/bash
BUILD_DIR = ./build

kernelrelease := $(shell uname -r)

# evaluate install paths
ifeq ($(install_path),)
	# defaults
	kernel_install_path ?= $(PREFIX)/lib/modules/$(kernelrelease)/updates/kernel/drivers/xvsec
	dev_install_path ?= /usr/local/include/
	user_install_path ?= /usr/local/sbin
else # bundled install
	kernel_install_path ?= $(install_path)/modules
	dev_install_path ?= $(install_path)/include/
	user_install_path ?= $(install_path)/bin
endif

.PHONY: all
all: drv libxvsec tools

.PHONY : drv
drv :
	@echo "#######################";
	@echo "####  drv          ####";
	@echo "#######################";
	cd drv && $(MAKE)

.PHONY : libxvsec
libxvsec :
	@echo "#######################";
	@echo "####  libxvsec     ####";
	@echo "#######################";
	cd libxvsec && $(MAKE)

.PHONY : tools
tools : libxvsec app
app :
	@echo "#######################";
	@echo "####  tools        ####";
	@echo "#######################";
	cd tools && $(MAKE)
.PHONY : clean
clean :
	@echo "#########################";
	@echo "#### removing driver ####";
	@echo "#########################";
	cd drv && $(MAKE) clean
	@echo "#########################";
	@echo "#### removing Lib #######";
	@echo "#########################";
	cd libxvsec && $(MAKE) clean
	@echo "#########################";
	@echo "#### removing tools #####";
	@echo "#########################";
	cd tools && $(MAKE) clean

.PHONY : install
install :
	@echo "installing kernel modules to $(kernel_install_path) ..."
	@mkdir -p -m 755 $(kernel_install_path)
	@install -v -m 644 $(BUILD_DIR)/modules/*.ko $(kernel_install_path)
	@depmod -a || true

	@echo "installing development headers to $(dev_install_path) ..."
	@mkdir -p -m 755 $(dev_install_path)
	@install -v -m 755 $(BUILD_DIR)/modules/*.h $(dev_install_path)

	@echo "installing user tools to $(user_install_path) ..."
	@mkdir -p -m 755 $(user_install_path)
	@install -v -m 755 $(BUILD_DIR)/xvsecctl $(user_install_path)

.PHONY : uninstall
uninstall :
	@echo "Un-installing $(kernel_install_path) ..."
	@/bin/rm -rf $(kernel_install_path)/*
	@depmod -a

	@echo "Un-installing user tools under $(user_install_path) ..."
	@/bin/rm -f $(user_install_path)/xvsecctl

	@echo "Un-installing development headers under $(dev_install_path) ..."
	@/bin/rm -r $(dev_install_path)

.PHONY: help
help:
	@echo "Build Targets:";\
	 echo " install             - Installs all compiled drivers.";\
	 echo " uninstall           - Uninstalls drivers.";\
	 echo " clean               - Removes all generated files.";\
	 echo " drv                 - builds the kernel driver"; \
	 echo " libxvsec            - builds the user space library"; \
	 echo " tools               - builds the application tool";
	 @echo;\
	 echo "Build Options:";\
	 echo " kernel_install_path=<path>";\
	 echo "                     - kernel module install path.";\
	 echo " user_install_path=<path>";\
	 echo "                     - user cli tool install path.";\
	 echo;
