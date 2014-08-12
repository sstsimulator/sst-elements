#!/bin/bash

PINDIR=/home/projects/pin/gcc/2.13.61206
CXX=g++
PREPROC=-DBIGARRAY_MULTIPLIER=1 -DUSING_XED -DTARGET_IA32E -DHOST_IA32E -fPIC -DTARGET_LINUX -DPROSPERO_LIBZ
LD=g++
CC=gcc

all: sstmemtrace.so

sstmemtrace.so: sstmemtrace.o
	$(LD) -Wl,--hash-style=sysv -shared -Wl,-Bsymbolic \
		-Wl,--version-script=$(PINDIR)/source/include/pin/pintool.ver \
		-L$(PINDIR)/extras/xed2-intel64/lib \
		-L$(PINDIR)/intel64/lib \
		-L$(PINDIR)/intel64/lib-ext \
		-o $@ $< \
		-L$(PINDIR)/extras/xed2-intel64/lib \
		-L$(PINDIR)/intel64/lib \
		-L$(PINDIR)/intel64/lib-ext -lpin -lxed \
		-lelf -ldl -lz -ldwarf \

sstmemtrace.o: sstmemtrace.cc
	$(CXX) -g -c -O3 -fomit-frame-pointer \
        -Wl,-Bsymbolic \
        -Wl,--version-script=$(PINDIR)/source/include/pintool.ver \
	-I $(PINDIR)/source/include/pin \
	-I $(PINDIR)/source/include/pin/gen \
	-I $(PINDIR)/extras/xed2-intel64/include \
	-I $(PINDIR)/extras/components/include \
	$(PREPROC) \
	-o $@ \
	$<

clean:
	rm *.o sstmemtrace.so sstmemtrace.dylib
