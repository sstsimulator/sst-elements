include ../Makefile.inc

omptriangle.x: omptriangle.o
	$(CC) $(CFLAGS) -o $@ $< -lgomp -L ../pthread/m5threads -lpthread

omptriangle.o: omptriangle.c
	$(CC) $(CFLAGS) $(OMP) -o $@ -c $<

clean:
	rm -f omptriangle.x *.o
