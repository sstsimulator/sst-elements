# -*- Makefile -*-
#
#

CC=$(CXX)

if BUILD_ZESTO

compdir = $(pkglibdir)
comp_LTLIBRARIES = libzesto.la

libzesto_la_SOURCES = \
	core/simplescalar/elf.h \
	core/simplescalar/endian.cpp \
	core/simplescalar/endian.h \
	core/simplescalar/eval.cpp \
	core/simplescalar/eval.h \
	core/simplescalar/host.h \
	core/simplescalar/loader.h \
	core/simplescalar/machine.cpp \
	core/simplescalar/machine.h \
	core/simplescalar/memory.cpp \
	core/simplescalar/memory.h \
	core/simplescalar/misc.cpp \
	core/simplescalar/misc.h \
	core/simplescalar/options.cpp \
	core/simplescalar/options.h \
	core/simplescalar/range.cpp \
	core/simplescalar/range.h \
	core/simplescalar/regs.cpp \
	core/simplescalar/regs.h \
	core/simplescalar/sim.h \
	core/simplescalar/stats.cpp \
	core/simplescalar/stats.h \
	core/simplescalar/symbol.cpp \
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
	core/zesto-opts.cpp \
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
	core/ZPIPE-alloc/alloc-atom.cpp \
	core/ZPIPE-alloc/alloc-DPM.cpp \
	core/ZPIPE-commit/commit-atom.cpp \
	core/ZPIPE-commit/commit-DPM.cpp \
	core/ZPIPE-decode/decode-atom.cpp \
	core/ZPIPE-decode/decode-DPM.cpp \
	core/ZPIPE-exec/exec-atom.cpp \
	core/ZPIPE-exec/exec-DPM.cpp \
	core/ZPIPE-fetch/fetch-atom.cpp \
	core/ZPIPE-fetch/fetch-DPM.cpp \
	\
	core/ZCOMPS-bpred.list \
	core/ZCOMPS-bpred/bpred-2bc.cpp \
	core/ZCOMPS-bpred/bpred-2lev.cpp \
	core/ZCOMPS-bpred/bpred-alloyedperceptron.cpp \
	core/ZCOMPS-bpred/bpred-bimode.cpp \
	core/ZCOMPS-bpred/bpred-blg.cpp \
	core/ZCOMPS-bpred/bpred-btfnt.cpp \
	core/ZCOMPS-bpred/bpred-nottaken.cpp \
	core/ZCOMPS-bpred/bpred-pathneural.cpp \
	core/ZCOMPS-bpred/bpred-perceptron.cpp \
	core/ZCOMPS-bpred/bpred-perfect.cpp \
	core/ZCOMPS-bpred/bpred-pwl.cpp \
	core/ZCOMPS-bpred/bpred-random.cpp \
	core/ZCOMPS-bpred/bpred-skewed.cpp \
	core/ZCOMPS-bpred/bpred-tage.cpp \
	core/ZCOMPS-bpred/bpred-taken.cpp \
	core/ZCOMPS-bpred/bpred-yags.cpp \
	core/ZCOMPS-btb.list \
	core/ZCOMPS-btb/btb-2levbtac.cpp \
	core/ZCOMPS-btb/btb-btac.cpp \
	core/ZCOMPS-btb/btb-perfect.cpp \
	core/ZCOMPS-dram.list \
	core/ZCOMPS-dram/dram-simplescalar.cpp \
	core/ZCOMPS-dram/dram-simplesdram.cpp \
	core/ZCOMPS-fusion.list \
	core/ZCOMPS-fusion/fusion-priority.cpp \
	core/ZCOMPS-fusion/fusion-multihybrid.cpp \
	core/ZCOMPS-fusion/fusion-singleton.cpp \
	core/ZCOMPS-fusion/fusion-colt.cpp \
	core/ZCOMPS-fusion/fusion-random.cpp \
	core/ZCOMPS-fusion/fusion-mcfarling.cpp \
	core/ZCOMPS-fusion/fusion-oracle.cpp \
	core/ZCOMPS-MC.list \
	core/ZCOMPS-MC/MC-simple.cpp \
	core/ZCOMPS-memdep.list \
	core/ZCOMPS-memdep/memdep-blind.cpp \
	core/ZCOMPS-memdep/memdep-lwt.cpp \
	core/ZCOMPS-memdep/memdep-none.cpp \
	core/ZCOMPS-memdep/memdep-oracle.cpp \
	\
	core/ZCOMPS-prefetch.list \
	core/ZCOMPS-prefetch/prefetch-context.cpp \
	core/ZCOMPS-prefetch/prefetch-IP.cpp \
	core/ZCOMPS-prefetch/prefetch-nextline.cpp \
	core/ZCOMPS-prefetch/prefetch-stream.cpp \
	core/ZCOMPS-ras.list \
	core/ZCOMPS-ras/ras-perfect.cpp \
	core/ZCOMPS-ras/ras-stack.cpp

libzesto_la_LDFLAGS = -module -avoid-version $(QSIM_LDFLAGS)
libzesto_la_LIBADD = $(QSIM_LIB)
libzesto_la_CPPFLAGS = $(QSIM_CPPFLAGS) $(BOOST_CPPFLAGS) $(MPI_CPPFLAGS)

endif
