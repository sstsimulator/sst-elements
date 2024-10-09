
CC=$(ARCH)-linux-musl-gcc
CXX=$(ARCH)-linux-musl-g++

CFLAGS=-O3 -fopenmp
CXXFLAGS=-O3
LDFLAGS=-static

PROG=openmp

$(PROG) : $(PROG).c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(ARCH)/$@ $<

clean:
	rm -r $(ARCH)/$(PROG)


