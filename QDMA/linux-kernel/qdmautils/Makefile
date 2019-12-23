CC ?= gcc

all: qdmautils.o qdmactl.o qdmactl_reg.o dmaxfer.o

%.o: %.c
	$(CC) -g -c -std=c99 -o $@ $< -D_FILE_OFFSET_BITS=64 -D_GNU_SOURCE -D_LARGE_FILE_SOURCE -D_AIO_AIX_SOURCE

clean:
	rm -rf *.o
