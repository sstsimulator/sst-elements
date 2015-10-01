LDFLAGS = -Llibs/lib
LIBS=-lruntime
CPPFLAGS += -Ilibs/include

apps: libs hello pthread

libs:
	cd libs && $(MAKE)

hello: hello.c
	${CC} -static hello.c -o $@ ${LDFLAGS}

pthread: 
	make -C pthread_test

rt: rt.c  libs/lib/libruntime.a
	${CC} ${CPPFLAGS} ${LDFLAGS} -static rt.c -o $@ ${LIBS}

clean:
	cd libs && $(MAKE) clean
	rm -rf hello.o hello rt rt.o
