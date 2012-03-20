
SRCS =  init.cc
OBJS = ${SRCS:.cc=.o}

LIB= libptlNic.a

CPPFLAGS += -Wall 
CPPFLAGS += -I../api/include -I.  -I../../ptlNic -I../../libc/
CPPFLAGS += -I../../../M5/app/lib/libc
#CPPFLAGS += -DDEBUG
#CPPFLAGS += -DUSE_PALACIOS_DEV
CPPFLAGS += -DUSE_M5

${LIB}: ${OBJS}
	${AR} rcs $@ ${OBJS} 

clean:
	rm -f ${OBJS} ${LIB}
