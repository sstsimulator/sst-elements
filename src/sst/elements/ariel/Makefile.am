
AM_CPPFLAGS = \
        $(MPI_CPPFLAGS) \
	-I$(top_srcdir)/src \
        $(PINTOOL_CPPFLAGS)

if HAVE_SET_PTRACER
AM_CPPFLAGS += "-DHAVE_SET_PTRACER=1"
endif

if HAVE_PIN3
AM_CPPFLAGS += "-DHAVE_PINCRT=1"
endif

compdir = $(pkglibdir)
comp_LTLIBRARIES = libariel.la
libariel_la_SOURCES = \
	arielcpu.cc \
	arielcpu.h \
	arielcore.cc \
	arielcore.h \
	arielmemmgr.h \
	arielmemmgr_cache.h \
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
	arielfreeev.h \
	ariel_inst_class.h \
	arielswitchpool.h \
	ariel_shmem.h \
	arieltracegen.h \
	arieltexttracegen.h \
	arieltexttracegen.cc \
	arielfrontend.h

EXTRA_DIST = \
	frontend/pin3/fesimple.cc \
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

if HAVE_PIN3
libariel_la_SOURCES += frontend/pin3/pin3frontend.h \
		       frontend/pin3/pin3frontend.cc
else
libariel_la_SOURCES += frontend/simple/pin2frontend.h \
		       frontend/simple/pin2frontend.cc
endif

if USE_CUDA
AM_CPPFLAGS += $(CUDA_CPPFLAGS)
endif

libariel_la_CPPFLAGS = \
	-DARIEL_TRACE_LIB=$(libdir)/sst \
        -DARIEL_TOOL_DIR="$(libexecdir)" \
	-DPINTOOL_EXECUTABLE="$(PINTOOL_RUNTIME)" \
	$(CPPFLAGS) $(AM_CPPFLAGS)

sstdir = $(includedir)/sst/elements/ariel
nobase_sst_HEADERS = \
	ariel_shmem.h \
	arieltracegen.h \
	arielmemmgr.h

libexec_PROGRAMS =


if SST_COMPILE_OSX

all-local: frontend/simple/fesimple.cc
	$(CXX) -O3 -shared \
	$(CXXFLAGS) \
	$(CPPFLAGS) \
	$(LIBZ_CPPFLAGS) \
	-DBIGARRAY_MULTIPLIER=1 \
	-DTARGET_IA32E -DHOST_IA32E -fPIC -DTARGET_MAC \
	$(CUDA_CPPFLAGS) \
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
	$(CUDA_LIBS) \
	-L$(PINTOOL_DIR)/intel64/lib \
	-L$(PINTOOL_DIR)/intel64/lib-ext \
	-L$(PINTOOL_DIR)/extras/xed-intel64/lib \
	-o fesimple.dylib $(top_srcdir)/src/sst/elements/ariel/frontend/simple/fesimple.cc \
	-stdlib=libstdc++ \
	-lpin -lxed -lpindwarf -lpthread $(LIBZ_LDFLAGS) $(LIBZ_LIB) $(CUDA_LDFLAGS)

install-exec-hook:
	$(MKDIR_P) $(libexecdir)
	$(INSTALL) fesimple.dylib $(libexecdir)/fesimple.dylib
	$(SST_REGISTER_TOOL) SST_ELEMENT_SOURCE     ariel=$(abs_srcdir)
	$(SST_REGISTER_TOOL) SST_ELEMENT_TESTS      ariel=$(abs_srcdir)/tests

else

all-local : fesimple.so

if HAVE_PIN3

fesimple.so : fesimple.o
	$(CXX) -shared -Wl,--hash-style=sysv \
	    $(PINTOOL_DIR)/intel64/runtime/pincrt/crtbeginS.o \
	    -Wl,-Bsymbolic -Wl,--version-script=$(PINTOOL_DIR)/source/include/pin/pintool.ver -fabi-version=2 \
	    -o fesimple.so fesimple.o \
	    -L$(PINTOOL_DIR)/intel64/runtime/pincrt \
	    -L$(PINTOOL_DIR)/intel64/lib \
	    -L$(PINTOOL_DIR)/intel64/lib-ext \
	    -L$(PINTOOL_DIR)/extras/xed-intel64/lib -lpin -lxed \
	    $(PINTOOL_DIR)/intel64/runtime/pincrt/crtendS.o \
	    -lpin3dwarf -ldl-dynamic -nostdlib -lstlport-dynamic -lm-dynamic -lc-dynamic -lunwind-dynamic

fesimple.o : $(top_srcdir)/src/sst/elements/ariel/frontend/pin3/fesimple.cc
	$(CXX) -g -Wall -Werror -Wno-unknown-pragmas -D__PIN__=1 -DPIN_CRT=1 \
	    -fno-stack-protector -fno-exceptions -funwind-tables -fasynchronous-unwind-tables -fno-rtti \
	    -DTARGET_IA32E -DHOST_IA32E -fPIC -DTARGET_LINUX -fabi-version=2 \
	    $(CPPFLAGS) \
	    -I$(top_srcdir)/src/sst/elements/ariel \
	    -I$(PINTOOL_DIR)/source/include/pin \
	    -I$(PINTOOL_DIR)/source/include/pin/gen \
	    -isystem $(PINTOOL_DIR)/extras/stlport/include \
	    -isystem $(PINTOOL_DIR)/extras/libstdc++/include \
	    -isystem $(PINTOOL_DIR)/extras/crt/include \
	    -isystem $(PINTOOL_DIR)/extras/crt/include/arch-x86_64 \
	    -isystem $(PINTOOL_DIR)/extras/crt/include/kernel/uapi \
	    -isystem $(PINTOOL_DIR)/extras/crt/include/kernel/uapi/asm-x86 \
	    -I$(PINTOOL_DIR)/extras/components/include \
	    -I$(PINTOOL_DIR)/extras/xed-intel64/include/xed \
	    -I$(PINTOOL_DIR)/source/tools/Utils \
	    -I$(PINTOOL_DIR)/source/tools/InstLib \
	    -O3 -fomit-frame-pointer -fno-strict-aliasing -c $(top_srcdir)/src/sst/elements/ariel/frontend/pin3/fesimple.cc
	    

else

fesimple.so : fesimple.o
	$(CXX) -shared -Wl,--hash-style=sysv -Wl,-Bsymbolic \
	    -o fesimple.so fesimple.o \
	    -Wl,--version-script=$(PINTOOL_DIR)/source/include/pin/pintool.ver \
	    -L$(PINTOOL_DIR)/intel64/lib \
	    -L$(PINTOOL_DIR)/intel64/lib-ext \
	    -L$(PINTOOL_DIR)/intel64/runtime/glibc \
	    -L$(PINTOOL_DIR)/extras/xed-intel64/lib \
	    $(CUDA_LIBS) \
	    -ldl -lpin -lxed -lpindwarf -ldl -lpthread -lrt $(LIBZ_LDFLAGS) $(LIBZ_LIB) $(CUDA_LDFLAGS)

fesimple.o : $(top_srcdir)/src/sst/elements/ariel/frontend/simple/fesimple.cc
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) \
	    $(LIBZ_CPPFLAGS) \
	    $(CUDA_CPPFLAGS) \
	    -DTARGET_IA32E -DHOST_IA32E -fPIC -DTARGET_LINUX \
	    -I./ \
	    -I$(PINTOOL_DIR)/source/include/pin \
	    -I$(PINTOOL_DIR)/ \
	    -I$(PINTOOL_DIR)/extras/components/include \
	    -I$(PINTOOL_DIR)/source/include/pin/gen/ \
	    -I$(PINTOOL_DIR)/extras/xed-intel64/include \
	    -I$(top_srcdir)/src/sst \
	    -I$(top_srcdir)/src/sst/elements/ariel \
	    -O3 -fomit-frame-pointer -c $(top_srcdir)/src/sst/elements/ariel/frontend/simple/fesimple.cc

endif

install-exec-hook:
	$(MKDIR_P) $(libexecdir)
	$(INSTALL) fesimple.so $(libexecdir)/fesimple.so
	$(SST_REGISTER_TOOL) SST_ELEMENT_SOURCE     ariel=$(abs_srcdir)
	$(SST_REGISTER_TOOL) SST_ELEMENT_TESTS      ariel=$(abs_srcdir)/tests
	$(SST_REGISTER_TOOL) SST_ELEMENT_TESTSUITES ariel=$(abs_srcdir)/testsuites


endif
