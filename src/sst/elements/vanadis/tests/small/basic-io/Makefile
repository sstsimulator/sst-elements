CC=mipsel-linux-musl-gcc
CXX=mipsel-linux-musl-g++

all: hello-world hello-world-CC

hello-world: hello-world.c
	$(CC) -o hello-world -O3 -static hello-world.c

hello-world-CC: hello-world.cc
	$(CXX) -o hello-world-CC -O3 -static hello-world.cc

clean:
	rm hello-world hello-world-CC
