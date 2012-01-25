
all:
	cd libc && $(MAKE)
	cd rt && $(MAKE)
	test -d lib || mkdir lib
	cd lib && test -f libm5rt.a || ln -s ../rt/libm5rt.a
	test -d include || mkdir include 
	cd include && test -f cnos_mpi_os.h || ln -s ../rt/m5rt.h cnos_mpi_os.h

clean:
	cd libc && $(MAKE) clean
	cd rt && $(MAKE) clean
