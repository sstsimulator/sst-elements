
# to amekf MIPS
# make ARCH=mipsel ADDR_TYPE=uint32_t PRIxBITS=PRIx32 PRIuBITS=PRIu32

ARCH ?= riscv64
ADDR_TYPE ?= uint64_t
PRIxBITS ?= PRIx64
PRIuBITS ?= PRIu64

AR=$(ARCH)-linux-musl-ar
CC=$(ARCH)-linux-musl-gcc

CFLAGS=-Iinclude -I../../.. -Wattributes -Wall -DADDR_TYPE=$(ADDR_TYPE)  -DPRIxBITS=$(PRIxBITS) -DPRIuBITS=$(PRIuBITS)
LIBS=-lrdma -L$(ARCH)

OBJS=$(ARCH)/base.o $(ARCH)/rdma.o

all: librdma.a write msg incast incast-v2 barrier
librdma.a: ${OBJS}
	$(AR) rcs $(ARCH)/librdma.a $^

$(ARCH)/rdma.o: src/rdma.c include/rdma.h ../../../rdmaNicHostInterface.h
	$(CC) $(CFLAGS) -c $< -o $@

$(ARCH)/base.o: src/base.c include/base.h ../../../rdmaNicHostInterface.h
	$(CC) $(CFLAGS) -c $< -o $@

msg: msg.c $(ARCH)/librdma.a
	$(CC) $(CFLAGS) -static -o $(ARCH)/$@ $< $(LIBS)

write: write.c $(ARCH)/librdma.a
	$(CC) $(CFLAGS) -static -o $(ARCH)/$@ $< $(LIBS)

incast: incast.c $(ARCH)/librdma.a
	$(CC) $(CFLAGS) -static -o $(ARCH)/$@ $< $(LIBS)

incast-v2: incast-v2.c $(ARCH)/librdma.a
	$(CC) $(CFLAGS) -static -o $(ARCH)/$@ $< $(LIBS)

barrier: barrier.c $(ARCH)/librdma.a
	$(CC) $(CFLAGS) -static -o $(ARCH)/$@ $< $(LIBS)

clean:
	rm -f $(ARCH)/librdma.a $(ARCH)/${OBJS} $(ARCH)/msg $(ARCH)/write $(ARCH)/incast $(ARCH)/incast-v2 $(ARCH)/barrier

