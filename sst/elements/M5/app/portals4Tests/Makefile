

LDFLAGS += -static -u ptlNic -lportals -lptlNic -lm5rt -lm5c
LDFLAGS += -L../lib/rt -L../lib/libc -L../lib/portals4/api -L../lib/portals4/nic

CPPFLAGS += -I../../ptlNic -I../lib/rt -I../lib/libc

apps: put 

put: put.o
	${CXX} put.o -o $@ ${LDFLAGS}

clean:
	rm -rf put.o put 
