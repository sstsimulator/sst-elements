PREFIX := $(RISCV_TOOLCHAIN_INSTALL_PATH)/bin/riscv64-unknown-linux-gnu-
CC     := $(PREFIX)gcc
CFLAGS := -I. -static -O3
OBJDUMPFLAGS := -Mno-aliases -Mnumeric --disassemble-all --disassemble-zeroes -S
ADDR_TYPE ?= uint32_t

all: vanadisHandshake

vanadisHandshake: vanadisHandshake.c cuda_runtime_api.c
	$(CC) -o $@ $(CFLAGS) -DADDR_TYPE=$(ADDR_TYPE) $^

vanadisHandshake.dump: vanadisHandshake
	$(PREFIX)objdump $(OBJDUMPFLAGS) $< > $@

clean:
	rm vanadisHandshake

