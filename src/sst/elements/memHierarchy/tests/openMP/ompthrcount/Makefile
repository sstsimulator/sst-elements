include ../Makefile.inc

ompthrcount.x: ompthrcount.o
	$(CC) $(CFLAGS) -o $@ $< -lgomp -L ../pthread/m5threads -lpthread

ompthrcount.o: ompthrcount.c
	$(CC) $(CFLAGS) $(OMP) -o $@ -c $<

clean:
	rm -f ompthrcount.x *.o
