.PHONY.:	all clean realclean

MPICXX = mpicxx
MPICC = mpicc

DEFS = -DNOT_PART_OF_SIM -D_PATTERNS_H_ -DSTANDALONECPLUSPLUS
EXECS = memcpy \
	vector_add \
	allreduce_bench \
	alltoall_bench \
	pingpong_bench \
	router_bench \
	msgrate_bench \
	ghost_bench \
	work_stealing_bench \
	fft_bench

all:	$(EXECS)

memcpy:	memcpy.cc ../util/stats.cc ../util/stats.h stat_p.c stat_p.h util.c util.h
	$(MPICXX) $(DEFS) -I.. memcpy.cc ../util/stats.cc stat_p.c util.c -o $@ -lm

vector_add:	vector_add.cc ../util/stats.cc ../util/stats.h stat_p.c stat_p.h util.c util.h
	$(MPICXX) $(DEFS) -I.. vector_add.cc ../util/stats.cc stat_p.c util.c -o $@ -lm

pingpong_bench:	pingpong_bench.c stat_p.c stat_p.h util.c util.h
	$(MPICC) $(DEFS) -USTANDALONECPLUSPLUS -I.. pingpong_bench.c stat_p.c util.c -o $@ -lm

router_bench:	router_bench.c stat_p.c stat_p.h util.c util.h
	$(MPICC) $(DEFS) -USTANDALONECPLUSPLUS -I.. router_bench.c stat_p.c util.c -o $@ -lm

msgrate_bench:	msgrate_bench.c stat_p.c stat_p.h util.c util.h
	$(MPICC) $(DEFS) -USTANDALONECPLUSPLUS -I.. msgrate_bench.c stat_p.c util.c -o $@ -lm

allreduce_bench:	allreduce_bench.cc ../util/stats.cc ../util/stats.h stat_p.c stat_p.h \
	../collective_patterns/collective_topology.h util.c util.h Collectives/allreduce.cc \
	Collectives/allreduce.h
	$(MPICXX) $(DEFS) -I.. -I../collective_patterns allreduce_bench.cc ../util/stats.cc stat_p.c util.c \
	../collective_patterns/collective_topology.cc Collectives/allreduce.cc -o $@ -lm

alltoall_bench:	alltoall_bench.cc ../util/stats.cc ../util/stats.h stat_p.c stat_p.h \
	util.c util.h Collectives/alltoall.cc Collectives/alltoall.h
	$(MPICXX) $(DEFS) -I.. alltoall_bench.cc ../util/stats.cc stat_p.c util.c \
	Collectives/alltoall.cc -o $@ -lm

include src_ghost_bench/Makefile.inc
ghost_bench:	$(addprefix ../,$(ghost_bench_SOURCES))
	$(MPICXX) $(DEFS) -I.. $(addprefix ../,$(ghost_bench_SOURCES)) -o $@ -lm

include src_work_stealing_bench/Makefile.inc
work_stealing_bench:	$(addprefix ../,$(ws_bench_SOURCES))
	$(MPICXX) $(DEFS) -I.. $(addprefix ../,$(ws_bench_SOURCES)) -o $@ -lm

include src_fft_bench/Makefile.inc
fft_bench:	$(addprefix ../,$(fft_bench_SOURCES))
	$(MPICC) $(DEFS) -USTANDALONECPLUSPLUS -I.. $(addprefix ../,$(fft_bench_SOURCES)) -o $@ -lm


clean:

realclean:	clean
	@rm -f $(EXECS)
