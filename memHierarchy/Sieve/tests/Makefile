CXX=g++


ompsievetest: ompsievetest.o
	$(CXX) -O3 -o ompsievetest -fopenmp ompsievetest.o

ompsievetest.o: ompsievetest.c
	$(CXX) -O3 -o ompsievetest.o -fopenmp -c ompsievetest.c

clean:
	rm -f ompsievetest *.o
