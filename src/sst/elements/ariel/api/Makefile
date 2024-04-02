all: libarielapi.so libarielapi.a

libarielapi.so: arielapi.c arielapi.h
	$(CC) -fPIC -shared -o libarielapi.so arielapi.c -fopenmp

arielapi.o: arielapi.c arielapi.h
	$(CC) -c -o arielapi.o arielapi.c -fopenmp

libarielapi.a: arielapi.o
	ar rcs $@ $^

clean:
	rm *.o *.a *.so
