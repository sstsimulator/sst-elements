
CXX=$(ARCH)-linux-musl-g++

CXXFLAGS=-fopenmp
LDFLAGS=-static

PROG=openmp

$(PROG) : $(PROG).cpp
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $(ARCH)/$@ $<

clean:
	rm -r $(ARCH)/$(PROG)
