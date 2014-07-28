
CC=clang
CFLAGS=-fPIC -O3
LDFLAGS=-shared -O3

libprospero.so: prospero.o
	$(CC) $(LDFLAGS) -o $@ $<

prospero.o: prospero.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm *.o libprospero.so
