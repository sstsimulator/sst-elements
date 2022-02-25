CC=mipsel-linux-musl-gcc
CXX=mipsel-linux-musl-g++

CFLAGS=-O3
CXXFLAGS=-O3
LDFLAGS=-static

all: hello-world hello-world-CC printf-check

hello-world: hello-world.c
	$(CC) -o hello-world $(CFLAGS) $(LDFLAGS) hello-world.c

hello-world-CC: hello-world.cc
	$(CXX) -o hello-world-CC $(CXXFLAGS) $(LDFLAGS) hello-world.cc

printf-check: printf-check.c
	$(CC) -o printf-check $(CFLAGS) $(LDFLAGS) printf-check.c

clean:
	rm hello-world hello-world-CC printf-check

