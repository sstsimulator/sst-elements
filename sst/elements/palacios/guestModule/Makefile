# set your module name here
# provide the following files
#
# MODULENAME.c	the driver
# MODULENAME.h  the include file
# MODULENAME_test.c	the driver test program
# MODULENAME_test.sh	the driver test script
#
MODULENAME 	:= p4
### should not need to change stuff below ######################


#KDIR		:= /lib/modules/$(shell uname -r)/build
KDIR		:= linux-2.6.35.7
#KDIR		:= /exports/linux-2.6.12.2/
PWD		:= $(shell pwd)

CC		:= gcc -Wall

ifeq ($(KERNELRELEASE),)
all:	$(MODULENAME)_test
all:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules
	cp $(MODULENAME)_test gato
	cp $(MODULENAME).ko gato
	cp $(MODULENAME).ko gato
	cp start gato
	cp stop gato
else
  obj-m	:= $(MODULENAME).o
endif

$(MODULENAME)_test: $(MODULENAME)_test.c
	$(CC) -g -o $(MODULENAME)_test $(MODULENAME)_test.c


load:
	insmod ./$(MODULENAME).ko

unload:
	rmmod $(MODULENAME)

test: all
	./$(MODULENAME)_test.sh

clean:
	rm -f *.o *.ko Modules.symvers *.mod.c .*.cmd $(MODULENAME)_test

