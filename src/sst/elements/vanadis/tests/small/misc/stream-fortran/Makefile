
CC=$(ARCH)-linux-musl-gcc
FF=$(ARCH)-linux-musl-gfortran

CFLAGS=-O3
CXXFLAGS=-O3
LDFLAGS=-static

PROG=stream-fortran

$(PROG): stream.f mysecond.o
	$(CC) $(CFLAGS) -c mysecond.c
	$(FF) $(FFLAGS) -c stream.f
	$(FF) $(FFLAGS) -static stream.o mysecond.o -o $(ARCH)/$@ 

#$(PROG) : $(PROG).c
#	$(CC) $(CFLAGS) $(LDFLAGS) -o $(ARCH)/$@ $<

clean:
	rm -r $(ARCH)/$(PROG)


