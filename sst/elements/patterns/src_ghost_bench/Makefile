.PHONY.:	all clean realclean

CFILES = ghost ranks neighbors memory work
CC = mpicc
CFLAGS += -Wall



all:	ghost minavgmax

ghost:	$(addsuffix .o, $(CFILES))
	$(CC) $(CFLAGS) $^ -o $@ -lm

%.o:	%.c %.h
	$(CC) $(CFLAGS) -c $< -o $@

minavgmax:	minavgmax.c
	$(CC) $(CFLAGS) $< -o $@

clean:
	@rm -f $(addsuffix .o, $(CFILES))

realclean:	clean
	@rm -f ghost minavgmax
