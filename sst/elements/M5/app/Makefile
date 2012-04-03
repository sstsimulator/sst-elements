LDFLAGS = -Llibs/lib
LIBS=-lruntime -lm5syscall 
CPPFLAGS += -Ilibs/include

apps: libs hello rt

libs:
	cd libs && $(MAKE)

hello: hello.c
	${CC} -static hello.c -o $@ ${LDFLAGS}

rt: rt.c  libs/lib/libruntime.a libs/lib/libm5syscall.a
	${CC} ${CPPFLAGS} ${LDFLAGS} -static rt.c -o $@ ${LIBS}

clean:
	cd libs && $(MAKE) clean
	rm -rf hello.o hello rt rt.o
