SRCS=gups8.s gups16.s gups32.s gups64.s
OBJS=$(SRCS:.s=.o)
all: $(OBJS)

.s.o: $<
	vas $<

clean:
	rm -f $(OBJS)

