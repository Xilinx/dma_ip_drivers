src = $(wildcard *.c) $(wildcard ../libs/*.c)
obj = $(src:.c=.o)
dep = $(obj:.o=.d)  # one dependency file for each source

TARGET = estherdaq

CFLAGS =  -D_FILE_OFFSET_BITS=64 -D_GNU_SOURCE -D_LARGE_FILE_SOURCE
CFLAGS += -I../include
#CFLAGS += -MMD     # option to generate a .d file during compilation

#LDFLAGS = lrt
#-include $(dep)   # include all dep files in the makefile

#$(TARGET): $(obj)
#	echo $(src)
#	$(CC)  $(CFLAGS) -o $@ $<
#	$(CC)  $(CFLAGS) -o $@ $^

estherdaq: $(obj)
	echo $(src)
	$(CC)  $(CFLAGS) -o $@ $@.o ../libs/esther-daq-lib.o


#%.o: $(src)
#	$(CC)  $(CFLAGS) -c -std=c99 -o $@ $<

%.o: %.c
	$(CC)  $(CFLAGS) -c -std=c99 -o $@ $<

.PHONY : clean
clean: cleandep
	rm -f  $(obj) *.d $(TARGET)

.PHONY: cleandep
cleandep:
	rm -f $(dep)
