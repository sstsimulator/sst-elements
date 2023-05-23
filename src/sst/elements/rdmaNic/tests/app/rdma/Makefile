ARCH ?= riscv64
ADDR_TYPE ?= uint64_t
PRIxBITS ?= PRIx64
PRIuBITS ?= PRIu64

AR=$(ARCH)-linux-musl-ar
CC=$(ARCH)-linux-musl-gcc

CFLAGS=-Iinclude -I../../.. -Wattributes -Wall -DADDR_TYPE=$(ADDR_TYPE)  -DPRIxBITS=$(PRIxBITS) -DPRIuBITS=$(PRIuBITS)
LIBS=-lrdma -L.

OBJS=base.o rdma.o

all: librdma.a write msg incast incast-v2 barrier
librdma.a: ${OBJS}
	$(AR) rcs librdma.a $^

rdma.o: src/rdma.c include/rdma.h ../../../rdmaNicHostInterface.h
	$(CC) $(CFLAGS) -c $<

base.o: src/base.c include/base.h ../../../rdmaNicHostInterface.h
	$(CC) $(CFLAGS) -c $< 

msg: msg.c librdma.a
	$(CC) $(CFLAGS) -static -o $@ $< $(LIBS)

write: write.c librdma.a
	$(CC) $(CFLAGS) -static -o $@ $< $(LIBS) 

incast: incast.c librdma.a
	$(CC) $(CFLAGS) -static -o $@ $< $(LIBS)

incast-v2: incast-v2.c librdma.a
	$(CC) $(CFLAGS) -static -o $@ $< $(LIBS)

barrier: barrier.c librdma.a
	$(CC) $(CFLAGS) -static -o $@ $< $(LIBS) 

clean:
	rm -f librdma.a ${OBJS} msg write incast incast-v2 barrier
	
