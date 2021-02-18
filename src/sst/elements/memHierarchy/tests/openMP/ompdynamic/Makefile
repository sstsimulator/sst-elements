include ../Makefile.inc

ompdynamic.x: ompdynamic.o
	$(CC) $(CFLAGS) -o $@ $< -lgomp -L ../pthread/m5threads -lpthread

ompdynamic.o: ompdynamic.c
	$(CC) $(CFLAGS) $(OMP) -o $@ -c $<

clean:
	rm -f ompdynamic.x *.o
