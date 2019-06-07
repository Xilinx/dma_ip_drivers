SHELL = /bin/bash

CFLAGS += -g
#CFLAGS += -O2 -fno-inline -Wall -Wstrict-prototypes
CFLAGS += -I. -I../include
CFLAGS += $(EXTRA_FLAGS)

DMACTL = dmactl
DMACTL_OBJS := $(patsubst %.c,%.o,$(wildcard cli/*.c))

ifneq ($(CROSS_COMPILE_FLAG),)
	CC=$(CROSS_COMPILE_FLAG)gcc
endif
 
all: clean dmactl

dmactl: $(DMACTL_OBJS)
	@mkdir -p -m 755 $(build_dir)
	$(CC) $^ -o $(DMACTL)
	@cp -f $(DMACTL) $(build_dir)

clean:
	@rm -f *.o */*.o $(DMACTL) $(build_dir)/$(DMACTL)*
