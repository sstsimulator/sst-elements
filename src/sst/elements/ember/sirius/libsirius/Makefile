
MPICXX=mpicxx
CXXFLAGS=-O3 -std=c++11 -I ../include -fPIC -DSIRIUS_BACKTRACE
SHARED=-shared

all: libsirius.so libsirius.a

libsirius.so: libsirius.cc
	$(MPICXX) $(SHARED) $(CXXFLAGS) -o libsirius.so libsirius.cc

libsirius.a: libsirius.o
	ar rc libsirius.a libsirius.o

libsirius.o: libsirius.cc
	$(MPICXX) $(CXXFLAGS) -c libsirius.cc

clean:
	rm libsirius.o libsirius.a libsirius.so


