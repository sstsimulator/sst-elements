
SRCS = runtime.c

OBJS = ${SRCS:.c=.o}

LIB = libruntime.a

CPPFLAGS += -I. -DUSE_M5

${LIB}: ${OBJS}
	${AR} rcs $@ ${OBJS}

clean:
	rm -f ${OBJS} ${LIB}

install:
	cp ${LIB} ../lib
	cp runtime.h ../include
