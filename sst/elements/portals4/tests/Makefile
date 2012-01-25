LDFLAGS += -static -u ptlNic -lportals -lptlNic -lruntime 
LDFLAGS += -L../usr_lib/api -L../usr_lib/nic -L../usr_lib/runtime

CPPFLAGS += -I../ptlNic/ -I../usr_lib/runtime

apps: put 

DEPS = ../usr_lib/api/libportals.a\
	../usr_lib/nic/libptlNic.a\
	../usr_lib/runtime/libruntime.a

put: put.o  ${DEPS}
	${CXX} put.o -o $@ ${LDFLAGS}

clean:
	rm -rf put.o put 
