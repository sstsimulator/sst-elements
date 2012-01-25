LDFLAGS += -static -u ptlNic -lportals -lptlNic -lruntime 
LDFLAGS += -L../lib/rt -L../usr_lib/api -L../usr_lib/nic -L../usr_lib/runtime

CPPFLAGS += -I../ptlNic/ -I../usr_lib/runtime

apps: put 

put: put.o
	${CXX} put.o -o $@ ${LDFLAGS}

clean:
	rm -rf put.o put 
