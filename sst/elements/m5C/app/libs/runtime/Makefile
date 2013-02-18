
SRCS = runtime.c

OBJS = ${SRCS:.c=.o}

LIB = libruntime.a

CPPFLAGS += -I../include -Wall

${LIB}: ${OBJS}
	${AR} rcs $@ ${OBJS}

clean:
	rm -f ${OBJS} ${LIB}

install:
	cp ${LIB} ../lib
	cp runtime.h ../include
