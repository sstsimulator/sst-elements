# -*- Makefile -*-
#
#

CC=$(CXX)

AM_CPPFLAGS = \
	$(BOOST_CPPFLAGS) \
	$(MPI_CPPFLAGS)

compdir = $(pkglibdir)
comp_LTLIBRARIES = libzesto.la

libzesto_la_SOURCES = \
	core/simplescalar/elf.h \
	core/simplescalar/endian.c \
	core/simplescalar/endian.h \
	core/simplescalar/eval.c \
	core/simplescalar/eval.h \
	core/simplescalar/host.h \
	core/simplescalar/loader.h \
	core/simplescalar/machine.c \
	core/simplescalar/machine.h \
	core/simplescalar/memory.c \
	core/simplescalar/memory.h \
	core/simplescalar/misc.c \
	core/simplescalar/misc.h \
	core/simplescalar/options.c \
	core/simplescalar/options.h \
	core/simplescalar/range.c \
	core/simplescalar/range.h \
	core/simplescalar/regs.c \
	core/simplescalar/regs.h \
	core/simplescalar/sim.h \
	core/simplescalar/stats.c \
	core/simplescalar/stats.h \
	core/simplescalar/symbol.c \
	core/simplescalar/symbol.h \
	core/simplescalar/thread.h \
	core/simplescalar/version.h \
	core/simplescalar/valcheck.h \
	core/simplescalar/machine.def \
	core/simplescalar/x86flow.def \
	\
	core/zesto-alloc.cpp \
	core/zesto-alloc.h \
	core/zesto-bpred.cpp \
	core/zesto-bpred.h \
	core/zesto-commit.cpp \
	core/zesto-commit.h \
	core/zesto-core.cpp \
	core/zesto-core.h \
	core/qsim/qsim-client.h \
	core/qsim/qsim-client.cc \
	core/qsim/qsimclient-core.cc \
	core/qsim/qsimclient-core.h \
	core/qsim/qsimlib-core.cc \
	core/qsim/qsimlib-core.h \
	core/trace-core.cc \
	core/trace-core.h \
	core/zesto-decode.cpp \
	core/zesto-decode.h \
	core/zesto-exec.cpp \
	core/zesto-exec.h \
	core/zesto-fetch.cpp \
	core/zesto-fetch.h \
	core/zesto-memdep.cpp \
	core/zesto-memdep.h \
	core/zesto-opts.c \
	core/zesto-opts.h \
	core/zesto-oracle.cpp \
	core/zesto-oracle.h \
	core/zesto-structs.h \
	\
	core/ZPIPE-alloc.list \
	core/ZPIPE-commit.list \
	core/ZPIPE-decode.list \
	core/ZPIPE-exec.list \
	core/ZPIPE-fetch.list \
	\
    simpleCache/cache_messages.cpp \
    simpleCache/cache_messages.h \
    simpleCache/cache_type.h \
    simpleCache/MemoryControllerMap.h \
    simpleCache/simple_cache.cpp \
    simpleCache/simple_cache.h \
    simpleCache/cache_types.h \
    simpleCache/hash_table.h \
    simpleCache/hash_table.cpp \
    \
    simpleMC/simple_mc.cc \
    simpleMC/simple_mc.h \
    \
    irisNIC/irisNicArbiter.cc \
    irisNIC/irisNicArbiter.h \
    irisNIC/irisNicBuffer.cc \
    irisNIC/irisNicBuffer.h \
    irisNIC/irisNicFlit.h \
    irisNIC/irisNicFlit.cc \
    irisNIC/irisNic.h \
    \
	sst-interface.cc

#files that should be distributed
# Files under ZCOMPS-xxx probably don't need to be distributed.
EXTRA_DIST = \
	ZPIPE-alloc/alloc-atom.cpp \
	ZPIPE-alloc/alloc-DPM.cpp \
	ZPIPE-commit/commit-atom.cpp \
	ZPIPE-commit/commit-DPM.cpp \
	ZPIPE-decode/decode-atom.cpp \
	ZPIPE-decode/decode-DPM.cpp \
	ZPIPE-exec/exec-atom.cpp \
	ZPIPE-exec/exec-DPM.cpp \
	ZPIPE-fetch/fetch-atom.cpp \
	ZPIPE-fetch/fetch-DPM.cpp \
	\
	ZCOMPS-bpred.list \
	ZCOMPS-bpred/bpred-2bc.cpp \
	ZCOMPS-bpred/bpred-2lev.cpp \
	ZCOMPS-bpred/bpred-alloyedperceptron.cpp \
	ZCOMPS-bpred/bpred-bimode.cpp \
	ZCOMPS-bpred/bpred-blg.cpp \
	ZCOMPS-bpred/bpred-btfnt.cpp \
	ZCOMPS-bpred/bpred-nottaken.cpp \
	ZCOMPS-bpred/bpred-pathneural.cpp \
	ZCOMPS-bpred/bpred-perceptron.cpp \
	ZCOMPS-bpred/bpred-perfect.cpp \
	ZCOMPS-bpred/bpred-pwl.cpp \
	ZCOMPS-bpred/bpred-random.cpp \
	ZCOMPS-bpred/bpred-skewed.cpp \
	ZCOMPS-bpred/bpred-tage.cpp \
	ZCOMPS-bpred/bpred-taken.cpp \
	ZCOMPS-bpred/bpred-yags.cpp \
	ZCOMPS-btb.list \
	ZCOMPS-btb/btb-2levbtac.cpp \
	ZCOMPS-btb/btb-btac.cpp \
	ZCOMPS-btb/btb-perfect.cpp \
	ZCOMPS-dram.list \
	ZCOMPS-dram/dram-simplescalar.cpp \
	ZCOMPS-dram/dram-simplesdram.cpp \
	ZCOMPS-fusion.list \
	ZCOMPS-fusion/fusion-priority.cpp \
	ZCOMPS-fusion/fusion-multihybrid.cpp \
	ZCOMPS-fusion/fusion-singleton.cpp \
	ZCOMPS-fusion/fusion-colt.cpp \
	ZCOMPS-fusion/fusion-random.cpp \
	ZCOMPS-fusion/fusion-mcfarling.cpp \
	ZCOMPS-fusion/fusion-oracle.cpp \
	ZCOMPS-MC.list \
	ZCOMPS-MC/MC-simple.cpp \
	ZCOMPS-memdep.list \
	ZCOMPS-memdep/memdep-blind.cpp \
	ZCOMPS-memdep/memdep-lwt.cpp \
	ZCOMPS-memdep/memdep-none.cpp \
	ZCOMPS-memdep/memdep-oracle.cpp \
	\
	ZCOMPS-prefetch.list \
	ZCOMPS-prefetch/prefetch-context.cpp \
	ZCOMPS-prefetch/prefetch-IP.cpp \
	ZCOMPS-prefetch/prefetch-nextline.cpp \
	ZCOMPS-prefetch/prefetch-stream.cpp \
	ZCOMPS-ras.list \
	ZCOMPS-ras/ras-perfect.cpp \
	ZCOMPS-ras/ras-stack.cpp

libzesto_la_LDFLAGS = -module -avoid-version
