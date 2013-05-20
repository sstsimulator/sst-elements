
SRCS = pmi.c simple_pmiutil.c barrier.c

OBJS = ${SRCS:.c=.o}
OBJS += kvs.o



LIB = libpmi.a

CPPFLAGS += -Wall -I. -I../../ptlNic -DUSE_M5

${LIB}: ${OBJS}
	${AR} rcs $@ ${OBJS}

clean:
	rm -f ${OBJS} ${LIB}
