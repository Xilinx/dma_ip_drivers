SHELL = /bin/bash
build_dir = $(PWD)/../build
lib_dir = $(build_dir)/lib
CFLAGS += -O2 -fno-inline -Wall -Wstrict-prototypes
CFLAGS += -I. -I../include -I../drv/ -I../drv/xvsec_mcap/
CFLAGS += $(EXTRA_FLAGS)

XVSEC = libxvsec
XVSEC_OBJS := $(patsubst %.c,%.o,$(wildcard *.c))

all: clean $(XVSEC)

$(XVSEC): $(XVSEC_OBJS)
	@mkdir -p -m 755 $(build_dir)
	@mkdir -p -m 755 $(lib_dir)
	@mkdir -p -m 755 $(lib_dir)/obj
	$(AR) -r $@ $^
	ranlib $@
	@mv -f $(XVSEC) $(lib_dir)/$(XVSEC).a
	@cp -f xvsec.h $(lib_dir)/
	@mv -f *.o $(lib_dir)/obj/

clean:
	@rm -rf *.o */*.o $(XVSEC).a $(lib_dir)/
