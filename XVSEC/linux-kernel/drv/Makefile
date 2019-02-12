XVSEC_HOME := $(shell pwd)
XVSEC_KVER := $(shell uname -r)

build_path := ../build
module_path := $(build_path)/modules
obj-m += xvsec.o
xvsec-objs := xvsec_cdev.o xvsec_drv.o

all:
	@mkdir -p -m 755 $(build_path)
	@mkdir -p -m 755 $(module_path)
	@mkdir -p -m 755 $(module_path)/obj
	make -C /lib/modules/$(XVSEC_KVER)/build M=$(XVSEC_HOME) modules
	@mv *.ko $(module_path)/
	@cp xvsec_cdev.h $(module_path)/
	@mv *.o  $(module_path)/obj/
	@mv .*cmd $(module_path)/obj/
	@mv *.symvers $(module_path)/obj/
	@mv *.mod* $(module_path)/obj/
	@mv *.order* $(module_path)/obj/
	@mv .tmp* $(module_path)/obj/

clean:
	@rm -rf $(module_path)
	make -C /lib/modules/$(XVSEC_KVER)/build M=$(XVSEC_HOME) clean
