
AM_CPPFLAGS = \
        $(BOOST_CPPFLAGS) \
        $(MPI_CPPFLAGS)

compdir = $(pkglibdir)
comp_LTLIBRARIES = libariel.la
libariel_la_SOURCES = \
	libariel.cc \
	arielcpu.cc \
	arielcpu.h \
	arielcore.cc \
	arielcore.h \
	arielmemmgr.cc \
	arielmemmgr.h \
	arielreadev.cc \
	arielreadev.h \
	arielexitev.h \
	arielexitev.cc \
	arielwriteev.cc \
	arielwriteev.h \
	arielevent.cc \
	arielevent.h \
	arielnoop.h \
	arielnoop.cc \
	arielallocev.h \
	arielallocev.cc \
	arielfreeev.h \
	arielfreeev.cc \
	ariel_inst_class.h \
	arielswitchpool.cc \
	arielswitchpool.h \
	ariel_shmem.h \
	arieltracegen.h \
	arieltexttracegen.h \
	arielgzbintracegen.h

EXTRA_DIST = \
	frontend/simple/examples/multicore.py \
	frontend/simple/examples/stream/Makefile \
	frontend/simple/examples/stream/Makefile \
	frontend/simple/examples/stream/ariel_ivb.py \
	frontend/simple/examples/stream/ariel_snb.py \
	frontend/simple/examples/stream/runstream.py \
	frontend/simple/examples/stream/runstreamSt.py \
	frontend/simple/examples/stream/runstreamNB.py \
	frontend/simple/examples/stream/memHstream.py \
	frontend/simple/examples/stream/stream.c

libariel_la_LDFLAGS = -module -avoid-version
libariel_la_CPPFLAGS = -DARIEL_TRACE_LIB=$(libdir)/sst \
	$(CPPFLAGS) $(AM_CPPFLAGS)

sstdir = $(includedir)/sst/elements/ariel
nobase_sst_HEADERS = \
	ariel_shmem.h \
	arieltracegen.h

libexec_PROGRAMS =

if SST_COMPILE_OSX

libexec_PROGRAMS += fesimple.dylib

fesimple_dylib_SOURCES = frontend/simple/fesimple.cc

fesimple.dylib: fesimple.o
	$(CXX) -shared \
	-Wl,-exported_symbols_list \
	-Wl,$(PINTOOL_DIR)/source/include/pin/pintool.exp \
	-stdlib=libstdc++ \
	-L$(PINTOOL_DIR)/intel64/lib \
	-L$(PINTOOL_DIR)/intel64/lib-ext \
	-L$(PINTOOL_DIR)/extras/xed-intel64/lib \
	-o fesimple.dylib fesimple.o  \
	-lpin -lxed -lpindwarf -lpthread

fesimple.o: frontend/simple/fesimple.cc
	$(CXX) -O3 \
	$(CPPFLAGS) \
	$(BOOST_CPPFLAGS) \
	-DBIGARRAY_MULTIPLIER=1 \
	-DTARGET_IA32E -DHOST_IA32E -fPIC -DTARGET_MAC \
	-I$(PINTOOL_DIR)/source/include/pin \
	-I$(PINTOOL_DIR)/ \
	-I$(PINTOOL_DIR)/extras/components/include \
	-I$(PINTOOL_DIR)/source/include/pin/gen/ \
	-I$(PINTOOL_DIR)/extras/xed-intel64/include \
	-fomit-frame-pointer -fno-stack-protector \
	-stdlib=libstdc++ \
	-o fesimple.o -c \
	frontend/simple/fesimple.cc

else

libexec_PROGRAMS += fesimple.so

fesimple_so_SOURCES = frontend/simple/fesimple.cc

fesimple.so: fesimple.o
	$(CXX) -shared -Wl,--hash-style=sysv -Wl,-Bsymbolic \
	-Wl,--version-script=$(PINTOOL_DIR)/source/include/pin/pintool.ver \
	-L$(PINTOOL_DIR)/intel64/lib \
	-L$(PINTOOL_DIR)/intel64/lib-ext \
	-L$(PINTOOL_DIR)/intel64/runtime/glibc \
	-L$(PINTOOL_DIR)/extras/xed-intel64/lib \
	-o fesimple.so fesimple.o  \
	-ldl -lpin -lxed -lpindwarf -ldl -lpthread -lrt

fesimple.o: frontend/simple/fesimple.cc
	$(CXX) -fPIC -O3 \
	-fomit-frame-pointer \
	$(CPPFLAGS) \
	$(BOOST_CPPFLAGS) \
	-DTARGET_IA32E -DHOST_IA32E -fPIC -DTARGET_LINUX \
	-I$(PINTOOL_DIR)/source/include/pin \
	-I$(PINTOOL_DIR)/ \
	-I$(PINTOOL_DIR)/extras/components/include \
	-I$(PINTOOL_DIR)/source/include/pin/gen/ \
	-I$(PINTOOL_DIR)/extras/xed-intel64/include \
	-o fesimple.o -c \
	frontend/simple/fesimple.cc

endif
