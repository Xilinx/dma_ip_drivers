prefix := /usr/local
bindir := $(prefix)/bin
PROGRAM_PREFIX :=
CC ?= gcc
CFLAGS ?=
LDFLAGS ?=
EXTRA_CFLAGS = -D_FILE_OFFSET_BITS=64 -D_GNU_SOURCE -D_LARGE_FILE_SOURCE
INSTALL ?= install

PROGRAMS = $(PROGRAM_PREFIX)reg_rw $(PROGRAM_PREFIX)dma_to_device $(PROGRAM_PREFIX)dma_from_device $(PROGRAM_PREFIX)performance $(PROGRAM_PREFIX)test_chrdev

all: $(PROGRAMS)

$(PROGRAM_PREFIX)dma_to_device: dma_to_device.o
	$(CC) -lrt -o $@ $< $(CFLAGS) $(EXTRA_CFLAGS) $(LDFLAGS)

$(PROGRAM_PREFIX)dma_from_device: dma_from_device.o
	$(CC) -lrt -o $@ $< $(CFLAGS) $(EXTRA_CFLAGS) $(LDFLAGS)

$(PROGRAM_PREFIX)performance: performance.o
	$(CC) -o $@ $< $(CFLAGS) $(EXTRA_CFLAGS) $(LDFLAGS)

$(PROGRAM_PREFIX)reg_rw: reg_rw.o
	$(CC) -o $@ $< $(CFLAGS) $(EXTRA_CFLAGS) $(LDFLAGS)

$(PROGRAM_PREFIX)test_chrdev: test_chrdev.o
	$(CC) -o $@ $< $(CFLAGS) $(EXTRA_CFLAGS) $(LDFLAGS)

%.o: %.c
	$(CC) -c -std=c99 -o $@ $< $(CFLAGS) $(EXTRA_CFLAGS)

clean:
	rm -rf *.o *.bin
	rm -fr $(PROGRAMS)

install: all
	install -d -m 0755 "$(DESTDIR)$(bindir)"
	install -m 0755 $(PROGRAMS) "$(DESTDIR)$(bindir)"
