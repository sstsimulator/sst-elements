
CXX=g++
CC=gcc

LDFLAGS += -static -u ptlNic -lportals -lptlNic 
LDFLAGS += -lruntime 

LDFLAGS += -L../usr_lib/api -L../usr_lib/nic 
LDFLAGS += -L../usr_lib/runtime

CPPFLAGS += -I../ptlNic/ 
CPPFLAGS += -I../usr_lib/runtime

apps: put  put2

DEPS = ../usr_lib/api/libportals.a\
	../usr_lib/nic/libptlNic.a\
	../usr_lib/runtime/libruntime.a

put: put.o  ${DEPS}
	${CXX} put.o -o $@ ${LDFLAGS}

put.o: put.c
	${CC} -c ${CPPFLAGS} put.c
	
put2: put2.o  ${DEPS}
	${CXX} put2.o -o $@ ${LDFLAGS}

clean:
	rm -rf put.o put 
