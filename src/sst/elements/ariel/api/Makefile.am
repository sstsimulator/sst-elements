if SST_USE_ARIEL_MPI
CC_LOCAL = $(ARIEL_MPICC)
CFLAGS_LOCAL = -fopenmp $(ARIEL_MPI_CFLAGS) -DENABLE_ARIEL_MPI=1
else
CC_LOCAL = $(CC)
CFLAGS_LOCAL =
endif

all-local: libarielapi.so libarielapi.a

libarielapi.so: arielapi.c arielapi.h
	$(CC_LOCAL) $(CFLAGS_LOCAL) -fPIC -shared -o libarielapi.so $(LDFLAGS) arielapi.c

arielapi.o: arielapi.c arielapi.h
	$(CC_LOCAL) $(CFLAGS_LOCAL) -c -o arielapi.o arielapi.c

libarielapi.a: arielapi.o
	ar rcs $@ $^

clean-local:
	rm -f arielapi.o libarielapi.a libarielapi.so
