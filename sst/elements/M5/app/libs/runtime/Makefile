
SRCS = m5rt.c

OBJS = ${SRCS:.c=.o}

LIB = libm5rt.a

CPPFLAGS += -I../include

${LIB}: ${OBJS}
	${AR} rcs $@ ${OBJS}

clean:
	rm -f ${OBJS} ${LIB}

install:
	cp ${LIB} ../lib
	cp runtime.h ../include
