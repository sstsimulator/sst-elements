LDFLAGS = -Llibs/lib

test: test.c pthread.c
	${CC} -I. -Wall -static $? -o $@ ${LDFLAGS}

clean:
	rm -f test.o pthread.o test
