PATH=/opt/buildroot-gcc463/usr/bin/
CROSS_COMPILE	=$(PATH)mipsel-linux-
CC=$(CROSS_COMPILE)gcc

SDK_PATH=/home/7620sdk/7688/sdk4300_20140916/RT288x_SDK/source
KERNEL_PATH =$(SDK_PATH)/linux-2.6.36.x

CFLAGS = -Wall -I ./include -I $(SDK_PATH)/lib/libnvram/ -I $(KERNEL_PATH)/include/
LDFLAGS= -lpthread -lrt  -lbase463  -lsystools463  $(SDK_PATH)/lib/libnvram/libnvram-0.9.28.so


TAR = smartconfig
all +=smartconfig.o

export CC
$(TAR): $(all)
	$(CC) $(CFLAGS) -o $(TAR) $(all) $(LDFLAGS)
	$(RM) -f *.gch *.bak $(all) 
	
%.o:%.c
	$(CC) $(CFLAGS) -c -o $@ $< 

.PHONY: clean
clean:
	rm -f $(TAR) $(all) 
