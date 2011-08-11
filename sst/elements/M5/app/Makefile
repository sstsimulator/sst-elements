
export CC=alphaev67-unknown-linux-gnu-cc
export CXX=alphaev67-unknown-linux-gnu-c++
export AR=alphaev67-unknown-linux-gnu-ar

#export CC=sparc64-unknown-linux-gnu-gcc
#export CXX=sparc64-unknown-linux-gnu-g++
#export AR=sparc64-unknown-linux-gnu-ar

LDFLAGS += -static -lm5rt
LDFLAGS += -Llib/rt -Llib/libc

CPPFLAGS += -Ilib/rt -Ilib/libc

apps: libs hello rt portals

libs:
	cd lib && $(MAKE)

portals:
	cd portals4Tests && $(MAKE)


hello: hello.o
	${CC} hello.o -o $@ ${LDFLAGS}

rt: rt.o
	${CC} rt.o -o $@ ${LDFLAGS}

clean:
	cd portals4Tests && $(MAKE) clean
	cd lib && $(MAKE) clean
	rm -rf hello.o hello rt rt.o
