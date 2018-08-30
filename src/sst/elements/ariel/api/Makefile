all: libarielapi.so libarielapi.a

libarielapi.so: arielapi.c arielapi.h
	$(CC) -fPIC -shared -o libarielapi.so arielapi.c

arielapi.o: arielapi.c arielapi.h
	$(CC) -c -o arielapi.o arielapi.c

libarielapi.a: arielapi.o
	ar rcs $@ $^
