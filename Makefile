PATH=/opt/buildroot-gcc463/usr/bin/
CROSS_COMPILE	=$(PATH)mipsel-linux-
CC=$(CROSS_COMPILE)gcc

CFLAGS = -Wall -I ./include 
LDFLAGS= -lpthread -lrt  -lbase463  -lsystools463   


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
