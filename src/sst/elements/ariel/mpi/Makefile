CC=gcc
CXX=g++
MPICXX=mpic++

all: mpilauncher hello fakepin reduce libarielapi.so

clean:
	rm -rf mpilauncher mpilauncher hello fakepin reduce *.o *.so

mpilauncher: mpilauncher.cc
	g++ -g -o mpilauncher mpilauncher.cc

hello: hello.cc
	mpic++ -fopenmp -g -o hello hello.cc
reduce: reduce.cc libarielapi.so
	mpic++ -fopenmp -g -o reduce reduce.cc -L. -larielapi

fakepin: fakepin.cc
	g++ -g -o fakepin fakepin.cc

libarielapi.so: libarielapi.o
		gcc -shared -o libarielapi.so libarielapi.o

libarielapi.o: libarielapi.c
		gcc -o libarielapi.o -fPIC -I./ -c libarielapi.c

.PHONY: all clean

