PROGS=hello-world hello-world-cpp openat printf-check read-write unlink unlinkat

all:
	@for prog in $(PROGS) ; do make -C $${prog} ARCH=mipsel ; done
	@for prog in $(PROGS) ; do make -C $${prog} ARCH=riscv64 ; done

clean:
	@for prog in $(PROGS) ; do make -C $${prog} ARCH=mipsel clean ; done
	@for prog in $(PROGS) ; do make -C $${prog} ARCH=riscv64 clean ; done
