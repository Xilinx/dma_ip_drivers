SHELL = /bin/bash
build_dir = $(PWD)/../build
CFLAGS += -O2 -fno-inline -Wall -Wstrict-prototypes
CFLAGS += -I. -I$(build_dir)/lib/
CFLAGS += $(EXTRA_FLAGS)
LDFLAGS += -L$(build_dir)/lib/ -lxvsec -lpthread -rdynamic

XVSECCTL = xvsecctl
XVSECCTL_OBJS := $(patsubst %.c,%.o,$(wildcard *.c))

all: clean $(XVSECCTL)

$(XVSECCTL): $(XVSECCTL_OBJS)
	@mkdir -p -m 755 $(build_dir)
	$(CC) $^ -o $(XVSECCTL) $(LDFLAGS)
	@mv -f $(XVSECCTL) $(build_dir)
	@rm -f *.o

clean:
	@rm -f *.o */*.o $(XVSECCTL) $(build_dir)/$(XVSECCTL)* $(build_dir)/*.sh
