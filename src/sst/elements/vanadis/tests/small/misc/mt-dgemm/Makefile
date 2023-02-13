CC=$(ARCH)-linux-musl-gcc

CFLAGS=-O3 -ffast-math -static -ftree-vectorizer-verbose=3

PROG=mt-dgemm

$(PROG) : $(PROG).c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(ARCH)/$@ $< -D SIZE=64 -D REPEATS=1

clean:
	rm -r $(ARCH)/$(PROG)


