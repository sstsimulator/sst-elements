PREFIX := mipsel-linux-musl-
CC     := $(PREFIX)gcc
CFLAGS := -I. -static 
OBJDUMPFLAGS := -Mno-aliases -Mnumeric --disassemble-all --disassemble-zeroes
# TODO Check RDMA test makefile for the arch and addr_type changing
ADDR_TYPE ?= uint32_t

all: vanadisHandshake

vanadisHandshake: vanadisHandshake.c cuda_runtime_api.c
	$(CC) -o $@ $(CFLAGS) -DADDR_TYPE=$(ADDR_TYPE) $^

vanadisHandshake.dump: vanadisHandshake
	$(PREFIX)objdump $(OBJDUMPFLAGS) $< > $@

clean:
	rm vanadisHandshake

