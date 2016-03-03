LDFLAGS = -Llibs/lib

CFLAGS = -Wall -O3

OBJS = test.o pthread.o

test: ${OBJS} 
	${CC} -I. -static ${OBJS} -o ${@} ${LDFLAGS}

clean:
	rm -f ${OBJS} test
