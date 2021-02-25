include ../Makefile.inc

ompcritical.x: ompcritical.o
	$(CC) $(CFLAGS) -o $@ $< -lgomp -L ../pthread/m5threads -lpthread

ompcritical.o: ompcritical.c
	$(CC) $(CFLAGS) $(OMP) -o $@ -c $<

clean:
	rm -f ompcritical.x *.o
