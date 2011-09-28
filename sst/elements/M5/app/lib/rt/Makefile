
SRCS = m5rt.c m5_syscall.c

OBJS = ${SRCS:.c=.o}

CPPFLAGS += -I. -I../libc
LIB = libm5rt.a

${LIB}: ${OBJS}
	${AR} rcs $@ ${OBJS}

clean:
	rm -f ${OBJS} ${LIB}
