XVSEC_HOME := $(shell pwd)
XVSEC_KVER := $(shell uname -r)
KERN_MAJ := $(shell echo $(XVSEC_KVER) | awk -F'.' '{print $$1}')
KERN_MIN := $(shell echo $(XVSEC_KVER) | awk -F'.' '{print $$2}')

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

ifeq ($(shell [ $(KERN_MAJ) -lt 4 ] || [ $(KERN_MAJ) -eq 4 ] && [ $(KERN_MIN) -lt 14 ]; echo $$?),0)
  EXTRA_CFLAGS:= -D PRE4_14KERN
endif

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
