#CXX=mipsel-linux-musl-gcc
CXX=riscv64-linux-musl-gcc

all:  basic basic-dump

basic: basic.c
	$(CXX) -o basic -static basic.c

basic-dump: basic
	riscv64-linux-musl-objdump -D -S  basic > basic.out


clean:
	rm basic
