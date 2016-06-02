CXX=g++ 

stream: stream.o
	$(CXX) -O3 -o stream -fopenmp stream.o

stream.o: stream.c
	$(CXX) -O3 -o stream.o -fopenmp -c stream.c

clean:
	rm stream stream.o

