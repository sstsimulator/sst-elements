CXX=g++ 

ompmybarrier: ompmybarrier.o
	$(CXX) -O3 -o ompmybarrier -fopenmp ompmybarrier.o

ompmybarrier.o: ompmybarrier.c
	$(CXX) -O3 -o ompmybarrier.o -fopenmp -c ompmybarrier.c

clean:
	rm ompmybarrier ompmybarrier.o

