include ../Makefile.inc

ompatomic.x: ompatomic.o
	$(CC) $(CFLAGS) -o $@ $< -lgomp -L ../pthread/m5threads -lpthread

ompatomic.o: ompatomic.c
	$(CC) $(CFLAGS) $(OMP) -o $@ -c $<

clean:
	rm -f ompatomic.x *.o
