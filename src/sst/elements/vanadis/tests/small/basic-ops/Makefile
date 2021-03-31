CXX=mipsel-linux-musl-gcc

all: test-branch test-shift

test-branch: test-branch.c
	$(CXX) -o test-branch -static test-branch.c

test-shift: test-shift.c
	$(CXX) -o test-shift -static test-shift.c

clean:
	rm test-branch test-shift
