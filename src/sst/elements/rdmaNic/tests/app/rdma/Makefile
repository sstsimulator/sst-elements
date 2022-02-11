AR=mipsel-linux-musl-ar
CC=mipsel-linux-musl-gcc

CFLAGS=-Iinclude -I../../.. -Wattributes -Wall 
LIBS=-lrdma -L.

OBJS=rdma.o base.o

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
