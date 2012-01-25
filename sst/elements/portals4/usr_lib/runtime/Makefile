
SRCS = runtime.c

OBJS = ${SRCS:.c=.o}

CPPFLAGS += -I.
LIB = libruntime.a

${LIB}: ${OBJS}
	${AR} rcs $@ ${OBJS}

clean:
	rm -f ${OBJS} ${LIB}
