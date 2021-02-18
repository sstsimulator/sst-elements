include ../Makefile.inc

ompmybarrier.x: ompmybarrier.o
	$(CC) $(CFLAGS) -o $@ $< -lgomp -L ../pthread/m5threads -lpthread

ompmybarrier.o: ompmybarrier.c
	$(CC) $(CFLAGS) $(OMP) -o $@ -c $<

clean:
	rm -f ompmybarrier.x *.o
