# -*- Makefile -*-

fft_bench_CPPFLAGS = -DNOT_PART_OF_SIM -D_PATTERNS_H_ $(MPI_CPPFLAGS)
fft_bench_LDADD = $(MPILIBS) -lm
fft_bench_SOURCES = \
		benchmarks/src_fft_bench/pfft_drive.c \
		benchmarks/src_fft_bench/dft.h \
		benchmarks/src_fft_bench/dft.c \
		benchmarks/src_fft_bench/timestats_structures.h \
		benchmarks/src_fft_bench/pfft_r2_dit.h \
		benchmarks/src_fft_bench/pfft_r2_dit.c \
		benchmarks/stat_p.h \
		benchmarks/stat_p.c

bin_PROGRAMS += fft_bench

