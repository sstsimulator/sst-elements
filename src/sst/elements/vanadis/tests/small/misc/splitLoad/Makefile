
CC=$(ARCH)-linux-musl-gcc
CXX=$(ARCH)-linux-musl-g++

CFLAGS=
CXXFLAGS=
LDFLAGS=-static

PROG=splitLoad

$(PROG) : $(PROG).c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(ARCH)/$@ $<

clean:
	rm -r $(ARCH)/$(PROG)


