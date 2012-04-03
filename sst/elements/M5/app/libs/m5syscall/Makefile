
CFLAGS=-Wall
SRCS = m5_syscall.c

OBJS = ${SRCS:.c=.o}

LIB = libm5syscall.a

${LIB}: ${OBJS}
	${AR} rcs $@ ${OBJS}

clean:
	rm -f ${OBJS} ${LIB}

install:
	test -d ../include || mkdir ../include 
	test -d ../lib || mkdir ../lib
	cp ${LIB} ../lib
	cp m5_syscall.h ../include
