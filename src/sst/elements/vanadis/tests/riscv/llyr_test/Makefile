#CXX=mipsel-linux-musl-gcc
CXX=riscv64-linux-musl-gcc

all:  llyr llyr-dump

llyr: llyr_test.c
	$(CXX) -o llyr -static llyr_test.c

llyr-dump: llyr
	riscv64-linux-musl-objdump -D -S  llyr > llyr.out

clean:
	rm llyr llyr.out
