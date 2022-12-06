
CC=$(ARCH)-linux-musl-gcc
CXX=$(ARCH)-linux-musl-g++

CFLAGS=-O3
CXXFLAGS=-O3
LDFLAGS=-static

PROG=hello-world-cpp

$(PROG) : $(PROG).cc
	$(CXX) $(CFLAGS) $(LDFLAGS) -o $(ARCH)/$@ $<

clean:
	rm -r $(ARCH)/$(PROG)


