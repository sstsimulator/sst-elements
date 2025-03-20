
CC=$(ARCH)-linux-musl-gcc
CXX=$(ARCH)-linux-musl-g++

CFLAGS=-O3
CXXFLAGS=-O3
LDFLAGS=-static -fopenmp

PROG=single_array

$(PROG) : $(PROG).cpp
	$(CXX) $(CFLAGS) $(LDFLAGS) -o $(ARCH)/$@ $<

clean:
	rm -r $(ARCH)/$(PROG)


