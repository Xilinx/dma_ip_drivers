XVSEC_HOME := $(shell pwd)
XVSEC_KVER := $(shell uname -r)

build_path := ../build
module_path := $(build_path)/modules
obj-m += xvsec.o
xvsec-objs := xvsec_drv.o xvsec_cdev.o xvsec_util.o
xvsec-objs += ./xvsec_mcap/xvsec_mcap.o
xvsec-objs += ./xvsec_mcap/us/xvsec_mcap_us.o
xvsec-objs += ./xvsec_mcap/versal/xvsec_mcap_versal.o

ccflags-y := -I$(PWD) -I$(PWD)/xvsec_mcap
ccflags-y += -I$(PWD)/xvsec_mcap/us
ccflags-y += -I$(PWD)/xvsec_mcap/versal

all:
	@mkdir -p -m 755 $(build_path)
	@mkdir -p -m 755 $(module_path)
	@mkdir -p -m 755 $(module_path)/obj
	@mkdir -p -m 755 $(module_path)/obj/xvsec_mcap
	@mkdir -p -m 755 $(module_path)/obj/xvsec_mcap/us
	@mkdir -p -m 755 $(module_path)/obj/xvsec_mcap/versal
	make -C /lib/modules/$(XVSEC_KVER)/build M=$(XVSEC_HOME) modules
	@mv *.ko $(module_path)/
	@cp xvsec_drv.h $(module_path)/
	@cp ./xvsec_mcap/xvsec_mcap.h $(module_path)/
	@mv *.o  $(module_path)/obj/
	@mv .*cmd $(module_path)/obj/
	@mv *.symvers $(module_path)/obj/
	@mv *.mod* $(module_path)/obj/
	@mv *.order* $(module_path)/obj/
	@mv .tmp* $(module_path)/obj/
	@mv ./xvsec_mcap/*.o $(module_path)/obj/xvsec_mcap/
	@mv ./xvsec_mcap/us/*.o $(module_path)/obj/xvsec_mcap/us/
	@mv ./xvsec_mcap/versal/*.o $(module_path)/obj/xvsec_mcap/versal/

clean:
	@rm -rf $(module_path)
	make -C /lib/modules/$(XVSEC_KVER)/build M=$(XVSEC_HOME) clean
