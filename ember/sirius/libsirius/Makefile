
MPICXX=mpicxx
CXXFLAGS=-O3 -std=c++11 -I ../include

libsirius.a: libsirius.o
	ar rc libsirius.a libsirius.o

libsirius.o: libsirius.cc
	mpicxx $(CXXFLAGS) -c libsirius.cc

clean:
	rm libsirius.o libsirius.a


