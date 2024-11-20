# Part executed when called from kernel build system
ifneq ($(KERNELRELEASE),)
obj-m += mymodule.o ## name of the generated module
mymodule-objs := skeleton.o ## list of objets needed by that module

# Part executed when called from standard make in the module source directory
else
CPU := arm64
KDIR := /home/lmi/workspace/nano/buildroot/output/build/linux-4.18.5/
TOOLS := /home/lmi/workspace/nano/buildroot/output/host/usr/bin/aarch64-linux-gnu-
PWD := $(shell pwd)
MODPATH := /home/lmi/workspace/nano/rootfs


all:
	$(MAKE) -C $(KDIR) M=$(PWD) ARCH=$(CPU) CROSS_COMPILE=$(TOOLS) modules
clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
install: all
	$(MAKE) -C $(KDIR) M=$(PWD) INSTALL_MOD_PATH=$(MODPATH) modules_install
endif