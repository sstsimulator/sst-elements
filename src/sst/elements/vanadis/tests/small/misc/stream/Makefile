
CC=$(ARCH)-linux-musl-gcc
CXX=$(ARCH)-linux-musl-g++

CFLAGS=-O3 -D NTIMES=3 -D STREAM_ARRAY_SIZE=10000
CXXFLAGS=-O3
LDFLAGS=-static

PROG=stream

$(PROG) : $(PROG).c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(ARCH)/$@ $<

clean:
	rm -r $(ARCH)/$(PROG)


