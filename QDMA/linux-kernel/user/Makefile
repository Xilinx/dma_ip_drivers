SHELL = /bin/bash

CFLAGS += -g
#CFLAGS += -O2 -fno-inline -Wall -Wstrict-prototypes
CFLAGS += -I. -I../include
CFLAGS += $(EXTRA_FLAGS)

DMACTL = dmactl
QDMAUTILS_OBJS := $(patsubst %.c,%.o,$(wildcard ../qdmautils/*.c))
DMACTL_OBJS := $(patsubst %.c,%.o,$(wildcard cli/*.c))
DMACTL_OBJS += $(QDMAUTILS_OBJS)
XFER_APP = qdma_xfer_app
XFER_OBJS := $(patsubst %.c,%.o,$(wildcard qdma_xfer_app/*.c))
XFER_OBJS += $(QDMAUTILS_OBJS)

ifneq ($(CROSS_COMPILE_FLAG),)
	CC=$(CROSS_COMPILE_FLAG)gcc
endif
 
all: clean dmactl qdma_xfer_app

dmactl: $(DMACTL_OBJS)
	@mkdir -p -m 755 $(build_dir)
	$(CC) -pthread -lrt $^ -o cli/$(DMACTL) -laio -D_FILE_OFFSET_BITS=64 -D_GNU_SOURCE -D_LARGE_FILE_SOURCE
	@cp -f cli/$(DMACTL) $(build_dir)
qdma_xfer_app: $(XFER_OBJS)
	@mkdir -p -m 755 $(build_dir)
	$(CC) -pthread -lrt $^ -o qdma_xfer_app/$(XFER_APP) -laio -D_FILE_OFFSET_BITS=64 -D_GNU_SOURCE -D_LARGE_FILE_SOURCE
	@cp -f qdma_xfer_app/$(XFER_APP) $(build_dir)
	
%.o: %.c
	$(CC) -g -c -std=c99 -o $@ $< -D_FILE_OFFSET_BITS=64 -D_GNU_SOURCE -D_LARGE_FILE_SOURCE -D_AIO_AIX_SOURCE

clean:
	@rm -f *.o */*.o ../qdmautils/*.o cli/$(DMACTL) qdma_xfer_app/$(XFER_APP) $(build_dir)/$(DMACTL)* $(build_dir)/$(XFER_APP)*
