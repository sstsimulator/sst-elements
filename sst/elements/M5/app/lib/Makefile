
all:
	cd libc && $(MAKE)
	cd rt && $(MAKE)
#	cd portals4 && $(MAKE)
	test -d lib || mkdir lib
#	cd lib && test -f libptlNic.a || ln -s ../portals4/nic/libptlNic.a
#	cd lib && test -f libportals.a || ln -s ../portals4/api/libportals.a
	cd lib && test -f libm5rt.a || ln -s ../rt/libm5rt.a
	test -d include || mkdir include 
	cd include && test -f cnos_mpi_os.h || ln -s ../rt/m5rt.h cnos_mpi_os.h
#	cd include && test -f portals4.h || ln -s ../../../ptlNic/portals4.h
#	cd include && test -f portals4_types.h || ln -s ../../../ptlNic/portals4_types.h

clean:
	cd libc && $(MAKE) clean
	cd rt && $(MAKE) clean
#	cd portals4 && $(MAKE) clean
