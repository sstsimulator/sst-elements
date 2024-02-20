SYSROOT=/opt/riscv/riscv64-unknown-elf
GCC_TOOLCHAIN=/opt/riscv
TARGET=riscv64
ARCH=rv64g

CC=/opt/llvm/bin/clang++ --sysroot=$(SYSROOT) --gcc-toolchain=$(GCC_TOOLCHAIN) --target=$(TARGET) -march=$(ARCH) -fno-vectorize -fno-slp-vectorize -O0

PROG=mvm

$(PROG) : $(PROG).cpp
	$(CC) -o $@ $<
	$(CC) -S -o mvm.S $<

clean:
	rm -f mvm mvm.S
