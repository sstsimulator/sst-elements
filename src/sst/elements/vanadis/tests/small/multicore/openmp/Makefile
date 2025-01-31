
CXX=$(ARCH)-linux-musl-g++

CXXFLAGS=-fopenmp
LDFLAGS=-static

PROG=openmp_example

$(PROG) : $(PROG).cpp
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $(ARCH)/$@ $<

clean:
	rm -r $(ARCH)/$(PROG)
