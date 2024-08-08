if SST_USE_ARIEL_MPI
CFLAGS+=-fopenmp $(ARIEL_MPI_CFLAGS)
CC_LOCAL = $(ARIEL_MPICC)
else
CC_LOCAL = $(CC)
endif

all-local: libarielapi.so libarielapi.a

libarielapi.so: arielapi.c arielapi.h
	$(CC_LOCAL) -DENABLE_ARIEL_MPI $(AM_CFLAGS) $(ARIEL_MPI_CFLAGS) -fPIC -shared -o libarielapi.so arielapi.c -fopenmp

arielapi.o: arielapi.c arielapi.h
	$(CC_LOCAL) -DENABLE_ARIEL_MPI $(AM_CFLAGS) $(ARIEL_MPI_CFLAGS) -c -o arielapi.o arielapi.c -fopenmp

libarielapi.a: arielapi.o
	ar rcs $@ $^

clean-local:
	rm arielapi.o libarielapi.a libarielapi.so
