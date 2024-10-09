
CC=$(ARCH)-linux-musl-gcc
CXX=$(ARCH)-linux-musl-g++

LDFLAGS=-static

SRC=$(wildcard *.c)
EXEC=$(SRC:.c=.x)


$(EXEC):$(SRC)
	$(CC) $< $(LDFLAGS) $(CFLAGS) -o $@

all:
	$(EXEC)

clean:
	rm -r $(EXEC)


