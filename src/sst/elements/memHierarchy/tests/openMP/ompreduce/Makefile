include ../Makefile.inc

ompreduce.x: ompreduce.o
	$(CC) $(CFLAGS) -o $@ $< -lgomp -L ../pthread/m5threads -lpthread

ompreduce.o: ompreduce.c
	$(CC) $(CFLAGS) $(OMP) -o $@ -c $<

clean:
	rm -f ompreduce.x *.o
