
CC=gcc
CFLAGS=-O3
LDFLAGS=-O3
LD=gcc

all: array

array: array.o
	$(LD) $(LDFLAGS) -o $@ $<	

array.o: array.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f *.o array
