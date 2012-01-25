
SRCS =  init.cc
OBJS = ${SRCS:.cc=.o}

LIB= libptlNic.a

CPPFLAGS += -Wall 
CPPFLAGS += -I../api/include -I.  -I../..//ptlNic -I../../libc/
CPPFLAGS += -DDEBUG
CPPFLAGS += -DUSE_PALACIOS_DEV

${LIB}: ${OBJS}
	${AR} rcs $@ ${OBJS} 

clean:
	rm -f ${OBJS} ${LIB}
