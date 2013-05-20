
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
CPPFLAGS += -I../../ptlNic -Iinclude
#CPPFLAGS += -DDEBUG

${LIB}: ${OBJS}
	${AR} rcs $@ ${OBJS} 

install:
	cp ${LIB} ../lib
	cp ../../ptlNic/portals4.h ../include

clean:
	rm -f ${OBJS} ${LIB}
