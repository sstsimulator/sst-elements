#!/bin/bash

PINDIR=$(HOME)/pin/2.12-gcc
CXX=g++
PREPROC=-DBIGARRAY_MULTIPLIER=1 -DUSING_XED -DTARGET_IA32E -DHOST_IA32E -fPIC -DTARGET_LINUX
LD=g++
CC=gcc

all: arieltool.so

arieltool.so: arieltool.o
	$(LD) -Wl,--hash-style=sysv -shared -Wl,-Bsymbolic \
		-Wl,--version-script=$(PINDIR)/source/include/pin/pintool.ver \
		-L$(PINDIR)/Lib/ \
		-L$(PINDIR)/ExtLib/ \
		-L$(PINDIR)/extras/xed2-intel64/lib \
		-L$(PINDIR)/intel64/lib \
		-L$(PINDIR)/intel64/lib-ext \
		-o $@ $< \
		-L$(PINDIR)/Lib/ \
		-L$(PINDIR)/ExtLib/ \
		-L$(PINDIR)/extras/xed2-intel64/lib \
		-L$(PINDIR)/intel64/lib \
		-L$(PINDIR)/intel64/lib-ext -lpin  -lxed -ldwarf \
		-lelf -ldl -lgmp

arieltool.o: arieltool.c
	$(CXX) -g -c -O3 -fomit-frame-pointer \
	-Wl,-Bsymbolic \
	-Wl,--version-script=$(PINDIR)/source/include/pin/pintool.ver \
	-I $(PINDIR)/source/include/pin \
	-I $(PINDIR)/source/include/pin/gen \
	-I $(PINDIR)/extras/xed2-intel64/include \
	-I $(PINDIR)/extras/components/include \
	$(PREPROC) \
	-o $@ \
	$<

clean:
	rm *.o arieltool.so
