CC=mipsel-linux-musl-gcc
CXX=mipsel-linux-musl-g++

CFLAGS=-O3
CXXFLAGS=-O3
LDFLAGS=-static

APPS=hello-world hello-world-CC printf-check read-write unlink unlinkat openat

all: $(APPS)

hello-world: hello-world.c
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $< 

read-write: read-write.c
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $< 

unlink: unlink.c
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $< 

unlinkat: unlinkat.c
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $< 

openat: openat.c
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $< 

hello-world-CC: hello-world.cc
	$(CXX) -o $@ $(CXXFLAGS) $(LDFLAGS) $< 

printf-check: printf-check.c
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $<

clean:
	rm $(APPS)

