
CC=$(ARCH)-linux-musl-gcc
CXX=$(ARCH)-linux-musl-g++

CFLAGS=-O3
CXXFLAGS=-O3
LDFLAGS=-static

PROG=gettime

$(PROG) : $(PROG).c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(ARCH)/$@ $<

clean:
	rm -r $(ARCH)/$(PROG)


