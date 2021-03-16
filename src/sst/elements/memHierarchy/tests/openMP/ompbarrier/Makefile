include ../Makefile.inc

ompbarrier.x: ompbarrier.o
	$(CC) $(CFLAGS) -o $@ $< -lgomp -L ../pthread/m5threads -lpthread

ompbarrier.o: ompbarrier.c
	$(CC) $(CFLAGS) $(OMP) -o $@ -c $<

clean:
	rm -f ompbarrier.x *.o
