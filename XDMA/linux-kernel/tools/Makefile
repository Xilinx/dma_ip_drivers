CC ?= gcc

#all: reg_rw dma_to_device dma_from_device performance
COMMON_SRC := ../libs/esther-daq-lib.c

#VPATH := ../lib
#src1:../src2

#COMMON_OBJS := $(patsubst %.c, %.o, $(notdir $(COMMON_SRC)))

COMMON_OBJS :=  $(COMMON_SRC:.c=.o) 
COMMON_DEPS :=  $(COMMON_OBJS:.o=.d) 


all: estherdaq

CFLAGS =  -D_FILE_OFFSET_BITS=64 -D_GNU_SOURCE -D_LARGE_FILE_SOURCE
CFLAGS += -I../include

estherdaq: estherdaq.o $(COMMON_OBJS)
	echo $(COMMON_OBJS)
	$(CC) $(CFLAGS) -lrt -o $@ $^


#$(CC) $(CFLAGS) -lrt -o $@ $< -D_FILE_OFFSET_BITS=64 -D_GNU_SOURCE -D_LARGE_FILE_SOURCE

dma_to_device: dma_to_device.o ../libs/esther-daq-lib.o 
	$(CC) -lrt -o $@ $< -D_FILE_OFFSET_BITS=64 -D_GNU_SOURCE -D_LARGE_FILE_SOURCE

dma_from_device: dma_from_device.o
	$(CC) -lrt -o $@ $< -D_FILE_OFFSET_BITS=64 -D_GNU_SOURCE -D_LARGE_FILE_SOURCE

performance: performance.o
	$(CC) -o $@ $< -D_FILE_OFFSET_BITS=64 -D_GNU_SOURCE -D_LARGE_FILE_SOURCE

reg_rw: reg_rw.o
	$(CC) -o $@ $<

%.o: %.c
	$(CC) $(CFLAGS) -c -std=c99 -o $@ $< -D_FILE_OFFSET_BITS=64 -D_GNU_SOURCE -D_LARGE_FILE_SOURCE

clean:
	rm -rf estherdaq reg_rw *.o *.bin dma_to_device dma_from_device performance

