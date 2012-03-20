
SRCS = runtime.c

OBJS = ${SRCS:.c=.o}

CPPFLAGS += -I. -DUSE_M5
LIB = libruntime.a

${LIB}: ${OBJS}
	${AR} rcs $@ ${OBJS}

clean:
	rm -f ${OBJS} ${LIB}
