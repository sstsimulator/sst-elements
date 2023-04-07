PREFIX := mipsel-linux-musl-
CC     := $(PREFIX)gcc
CFLAGS := -I. -static 
OBJDUMPFLAGS := -Mno-aliases -Mnumeric --disassemble-all --disassemble-zeroes

all: vanadisHandshake

vanadisHandshake: vanadisHandshake.c cuda_runtime_api.c
	$(CC) -o $@ $(CFLAGS) $^

vanadisHandshake.dump: vanadisHandshake
	$(PREFIX)objdump $(OBJDUMPFLAGS) $< > $@

clean:
	rm vanadisHandshake

