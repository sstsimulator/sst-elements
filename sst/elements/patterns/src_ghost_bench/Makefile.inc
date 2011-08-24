# -*- Makefile -*-
#
#

ghost_bench_SOURCES = \
	src_ghost_bench/ghost.c \
	src_ghost_bench/ghost.h \
	src_ghost_bench/memory.c \
	src_ghost_bench/memory.h \
	src_ghost_bench/neighbors.c \
	src_ghost_bench/neighbors.h \
	src_ghost_bench/ranks.c \
	src_ghost_bench/ranks.h \
	src_ghost_bench/work.c \
	src_ghost_bench/work.h

minavgmax_LDFLAGS = -lm
minavgmax_SOURCES = \
	src_ghost_bench/minavgmax.c 

bin_PROGRAMS += ghost_bench minavgmax
