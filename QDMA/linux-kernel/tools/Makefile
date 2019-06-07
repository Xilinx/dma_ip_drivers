CC ?= gcc
targets=dma_to_device dma_from_device dma_from_device_w_udd 

ifneq ($(CROSS_COMPILE_FLAG),)
	CC=$(CROSS_COMPILE_FLAG)gcc
else
	targets += dmaperf	
endif

all: $(targets)

dma_to_device: dma_to_device.o
	$(CC) -lrt -o $@ $< -D_FILE_OFFSET_BITS=64 -D_GNU_SOURCE -D_LARGE_FILE_SOURCE
dma_from_device: dma_from_device.o
	$(CC) -lrt -o $@ $< -D_FILE_OFFSET_BITS=64 -D_GNU_SOURCE -D_LARGE_FILE_SOURCE
dma_from_device_w_udd: dma_from_device_w_udd.o
	$(CC) -lrt -o $@ $< -D_FILE_OFFSET_BITS=64 -D_GNU_SOURCE -D_LARGE_FILE_SOURCE
	
aioutility: aioutility.o posixaio_engine.o libaio_engine.o
	$(CC) -g -pthread -lrt -laio -o $@ $^ -D_GNU_SOURCE -D_LARGE_FILE_SOURCE -D_AIO_AIX_SOURCE
	
dmaperf: dmaperf.o
	$(CC) -pthread -lrt -o $@ $< -laio -D_FILE_OFFSET_BITS=64 -D_GNU_SOURCE -D_LARGE_FILE_SOURCE
	
%.o: %.c
	$(CC) -g -c -std=c99 -o $@ $< -D_FILE_OFFSET_BITS=64 -D_GNU_SOURCE -D_LARGE_FILE_SOURCE -D_AIO_AIX_SOURCE

clean:
	rm -rf *.o *.bin dma_to_device dma_from_device dma_from_device_w_udd dmaperf
