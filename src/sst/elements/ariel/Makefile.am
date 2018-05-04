
AM_CPPFLAGS = \
        $(MPI_CPPFLAGS) \
	-I$(top_srcdir)/src \
        $(PINTOOL_CPPFLAGS)

if HAVE_SET_PTRACER
AM_CPPFLAGS += "-DHAVE_SET_PTRACER=1"
endif

compdir = $(pkglibdir)
comp_LTLIBRARIES = libariel.la
libariel_la_SOURCES = \
	arielcpu.cc \
	arielcpu.h \
	arielcore.cc \
	arielcore.h \
	arielmemmgr.h \
	arielmemmgr_simple.cc \
	arielmemmgr_simple.h \
	arielmemmgr_malloc.cc \
	arielmemmgr_malloc.h \
	arielreadev.h \
	arielexitev.h \
	arielfenceev.h \
	arielflushev.h \
	arielwriteev.h \
	arielevent.cc \
	arielevent.h \
	arielnoop.h \
	arielallocev.h \
	arielalloctrackev.h \
	arielfreeev.h \
	ariel_inst_class.h \
	arielswitchpool.h \
	ariel_shmem.h \
	arieltracegen.h \
	arieltexttracegen.h \
	arieltexttracegen.cc

EXTRA_DIST = \
	frontend/simple/fesimple.cc \
	frontend/simple/examples/multicore.py \
	frontend/simple/examples/stream/Makefile \
	frontend/simple/examples/stream/ariel_ivb.py \
	frontend/simple/examples/stream/ariel_snb.py \
	frontend/simple/examples/stream/runstream.py \
	frontend/simple/examples/stream/runstreamSt.py \
	frontend/simple/examples/stream/runstreamNB.py \
	frontend/simple/examples/stream/memHstream.py \
	frontend/simple/examples/stream/ariel_snb_mlm.py \
	frontend/simple/examples/stream/malloc.txt \
	frontend/simple/examples/stream/stream.c \
	frontend/simple/examples/stream/stream_malloc.c


libariel_la_LDFLAGS = -module -avoid-version
libariel_la_LIBADD = $(SHM_LIB)

if USE_LIBZ
libariel_la_LDFLAGS += $(LIBZ_LDFLAGS)
libariel_la_LIBADD += $(LIBZ_LIB)
AM_CPPFLAGS += $(LIBZ_CPPFLAGS)
libariel_la_SOURCES += arielgzbintracegen.h arielgzbintracegen.cc
endif

libariel_la_CPPFLAGS = \
	-DARIEL_TRACE_LIB=$(libdir)/sst \
        -DARIEL_TOOL_DIR="$(libexecdir)" \
	-DPINTOOL_EXECUTABLE="$(PINTOOL_RUNTIME)" \
	$(CPPFLAGS) $(AM_CPPFLAGS)

sstdir = $(includedir)/sst/elements/ariel
nobase_sst_HEADERS = \
	ariel_shmem.h \
	arieltracegen.h

libexec_PROGRAMS =


if SST_COMPILE_OSX

all-local: frontend/simple/fesimple.cc
	$(CXX) -O3 -shared \
	$(CXXFLAGS) \
	$(CPPFLAGS) \
	$(LIBZ_CPPFLAGS) \
	-DBIGARRAY_MULTIPLIER=1 \
	-DTARGET_IA32E -DHOST_IA32E -fPIC -DTARGET_MAC \
	-I./ \
	-I$(PINTOOL_DIR)/source/include/pin \
	-I$(PINTOOL_DIR)/ \
	-I$(PINTOOL_DIR)/extras/components/include \
	-I$(PINTOOL_DIR)/source/include/pin/gen/ \
	-I$(PINTOOL_DIR)/extras/xed-intel64/include \
	-I$(top_srcdir)/src/sst \
	-I$(top_srcdir)/src/sst/elements/ariel \
	-fomit-frame-pointer -fno-stack-protector \
	-Wl,-exported_symbols_list \
	-Wl,$(PINTOOL_DIR)/source/include/pin/pintool.exp \
	-L$(PINTOOL_DIR)/intel64/lib \
	-L$(PINTOOL_DIR)/intel64/lib-ext \
	-L$(PINTOOL_DIR)/extras/xed-intel64/lib \
	-o fesimple.dylib $(top_srcdir)/src/sst/elements/ariel/frontend/simple/fesimple.cc \
	-stdlib=libstdc++ \
	-lpin -lxed -lpindwarf -lpthread $(LIBZ_LDFLAGS) $(LIBZ_LIB)

install-exec-hook:
	$(MKDIR_P) $(libexecdir)
	$(INSTALL) fesimple.dylib $(libexecdir)/fesimple.dylib

else

all-local:
	$(CXX) -shared -Wl,--hash-style=sysv -Wl,-Bsymbolic \
	-fPIC -O3 \
	-fomit-frame-pointer \
	$(CXXFLAGS) \
	$(CPPFLAGS) \
	$(LIBZ_CPPFLAGS) \
	-DTARGET_IA32E -DHOST_IA32E -fPIC -DTARGET_LINUX \
	-I./ \
	-I$(PINTOOL_DIR)/source/include/pin \
	-I$(PINTOOL_DIR)/ \
	-I$(PINTOOL_DIR)/extras/components/include \
	-I$(PINTOOL_DIR)/source/include/pin/gen/ \
	-I$(PINTOOL_DIR)/extras/xed-intel64/include \
	-I$(top_srcdir)/src/sst \
	-I$(top_srcdir)/src/sst/elements/ariel \
	-Wl,--version-script=$(PINTOOL_DIR)/source/include/pin/pintool.ver \
	-L$(PINTOOL_DIR)/intel64/lib \
	-L$(PINTOOL_DIR)/intel64/lib-ext \
	-L$(PINTOOL_DIR)/intel64/runtime/glibc \
	-L$(PINTOOL_DIR)/extras/xed-intel64/lib \
	-o fesimple.so $(top_srcdir)/src/sst/elements/ariel/frontend/simple/fesimple.cc \
	-ldl -lpin -lxed -lpindwarf -ldl -lpthread -lrt $(LIBZ_LDFLAGS) $(LIBZ_LIB)


install-exec-hook:
	$(MKDIR_P) $(libexecdir)
	$(INSTALL) fesimple.so $(libexecdir)/fesimple.so

endif
