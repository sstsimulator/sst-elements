CXX=g++ 

stream: stream.o
	$(CXX) -O3 -o stream -fopenmp stream.o

stream.o: stream.c
	$(CXX) -O3 -o stream.o -fopenmp -c stream.c

stream_mlm: stream_mlm.o
	$(CXX) -O3 -o stream_mlm -fopenmp -I$(ARIELAPI) -L$(ARIELAPI) -larielapi stream_mlm.o

stream_mlm.o: stream_malloc.c
	$(CXX) -O3 -o stream_mlm.o -fopenmp -I$(ARIELAPI) -L$(ARIELAPI) -larielapi -c stream_malloc.c

all: stream stream_mlm

clean:
	rm stream stream.o stream_mlm stream_mlm.o

