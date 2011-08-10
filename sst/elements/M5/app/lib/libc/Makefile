
SRCS = m5-sst.c  m5_syscall.c

OBJS = ${SRCS:.c=.o}

LIB = libm5c.a

${LIB}: ${OBJS}
	${AR} rcs $@ ${OBJS}

clean:
	rm -f ${OBJS} ${LIB}
