
SRCS = atomic.c\
		ct.c\
		get.c\
		init.c\
		md.c\
		ni.c\
		pt.c\
		triggered.c\
		bundle.c\
		eq.c\
		handle.c\
		le.c\
		me.c\
		process.c\
		put.c\
 
OBJS = ${SRCS:.c=.o}

LIB= libportals.a

CPPFLAGS += -Wall
CPPFLAGS += -I../../../../ptlNic -Iinclude

${LIB}: ${OBJS}
	${AR} rcs $@ ${OBJS} 

clean:
	rm -f ${OBJS} ${LIB}
