SUBDIRS = api
if SST_USE_ARIEL_MPI
SUBDIRS += mpi
endif

AM_CPPFLAGS += \
	-I$(top_srcdir)/src

if HAVE_SET_PTRACER
AM_CPPFLAGS += "-DHAVE_SET_PTRACER=1"
endif # HAVE_SET_PTRACER

if HAVE_PIN3
AM_CPPFLAGS += $(PINTOOL_CPPFLAGS)
AM_CPPFLAGS += "-DHAVE_PINCRT=1 "
else
if HAVE_PINTOOL
AM_CPPFLAGS += $(PINTOOL_CPPFLAGS)
endif # HAVE_PINTOOL
endif # HAVE_PIN3

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
	arielfrontend.h \
	tb_header.h \
	arielrtlev.h

EXTRA_DIST = \
	api/arielapi.c \
	api/arielapi.h \
	mpi/mpilauncher.cc \
	mpi/fakepin.cc \
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
	frontend/simple/examples/stream/stream_malloc.c \
	frontend/simple/examples/stream/tests/refFiles/test_Ariel_ariel_ivb.out \
	frontend/simple/examples/stream/tests/refFiles/test_Ariel_ariel_snb.out \
	frontend/simple/examples/stream/tests/refFiles/test_Ariel_ariel_snb_mlm.out \
	frontend/simple/examples/stream/tests/refFiles/test_Ariel_memHstream.out \
	frontend/simple/examples/stream/tests/refFiles/test_Ariel_runstream.out \
	frontend/simple/examples/stream/tests/refFiles/test_Ariel_runstreamNB.out \
	frontend/simple/examples/stream/tests/refFiles/test_Ariel_runstreamSt.out \
	tests/testsuite_default_Ariel.py \
	tests/testsuite_testio_Ariel.py \
	tests/testsuite_mpi_Ariel.py \
	tests/testopenMP/ompmybarrier/ompmybarrier.c \
	tests/testopenMP/ompmybarrier/Makefile \
	tests/testMPI/Makefile

libariel_la_LDFLAGS = = \
	-module \
	-avoid-version \
	$(AM_LDFLAGS)

libariel_la_LIBADD = $(SHM_LIB)

if USE_LIBZ
libariel_la_LDFLAGS += $(LIBZ_LDFLAGS)
libariel_la_LIBADD += $(LIBZ_LIB)
AM_CPPFLAGS += $(LIBZ_CPPFLAGS)
libariel_la_SOURCES += arielgzbintracegen.h arielgzbintracegen.cc
endif # USE_LIBZ

if HAVE_PINTOOL
if HAVE_PIN3
libariel_la_SOURCES += frontend/pin3/pin3frontend.h \
		       frontend/pin3/pin3frontend.cc
endif # HAVE_PIN3
endif # HAVE_PINTOOL

libariel_la_CPPFLAGS = \
	-DARIEL_TRACE_LIB=$(libdir)/sst \
	-DARIEL_TOOL_DIR="$(libexecdir)" \
	-DPINTOOL_EXECUTABLE="$(PINTOOL_RUNTIME)" \
	-DMPILAUNCHER_EXECUTABLE="$(CURDIR)/mpi/mpilauncher" \
	$(AM_CPPFLAGS) \
	$(CPPFLAGS)

sstdir = $(includedir)/sst/elements/ariel
nobase_sst_HEADERS = \
	ariel_shmem.h \
	arieltracegen.h \
	arielmemmgr.h \
	api/arielapi.h

libexec_PROGRAMS =

if !SST_COMPILE_OSX

if HAVE_PINTOOL

TARGET = intel64
BIONIC_ARCH = x86_64
XED_ARCH = intel64

if HAVE_PIN3

all-local : fesimple.so

if HAVE_PIN325

fesimple.so : fesimple.o
	$(CXX) -shared -Wl,--hash-style=sysv \
	    $(PINTOOL_DIR)/$(TARGET)/runtime/pincrt/crtbeginS.o \
	    -Wl,-Bsymbolic -Wl,--version-script=$(PINTOOL_DIR)/source/include/pin/pintool.ver \
	    -o fesimple.so fesimple.o \
	    -L$(PINTOOL_DIR)/$(TARGET)/runtime/pincrt \
	    -L$(PINTOOL_DIR)/$(TARGET)/lib \
	    -L$(PINTOOL_DIR)/$(TARGET)/lib-ext \
	    -L$(PINTOOL_DIR)/extras/xed-$(XED_ARCH)/lib -lpin -lxed \
	    $(PINTOOL_DIR)/$(TARGET)/runtime/pincrt/crtendS.o \
	    -lpindwarf -ldl-dynamic -nostdlib -lc++ -lc++abi -lm-dynamic -lc-dynamic -lunwind-dynamic

fesimple.o : $(top_srcdir)/src/sst/elements/ariel/frontend/pin3/fesimple.cc
	$(CXX) \
	    $(PIN_CPPFLAGS) $(PIN_CPPFLAGS_COMPILER) $(AM_CPPFLAGS) $(CPPFLAGS) \
	    -I$(top_srcdir)/src/sst/elements/ariel \
	    -I$(PINTOOL_DIR)/source/include/pin \
	    -I$(PINTOOL_DIR)/source/include/pin/gen \
	    -isystem $(PINTOOL_DIR)/extras/cxx/include \
	    -isystem $(PINTOOL_DIR)/extras/crt/include \
	    -isystem $(PINTOOL_DIR)/extras/crt/include/arch-$(BIONIC_ARCH) \
	    -isystem $(PINTOOL_DIR)/extras/crt/include/kernel/uapi \
	    -isystem $(PINTOOL_DIR)/extras/crt/include/kernel/uapi/asm-x86 \
	    -I$(PINTOOL_DIR)/extras/components/include \
	    -I$(PINTOOL_DIR)/extras/xed-$(XED_ARCH)/include/xed \
	    -I$(PINTOOL_DIR)/source/tools/Utils \
	    -I$(PINTOOL_DIR)/source/tools/InstLib \
	    -O3 -c $(top_srcdir)/src/sst/elements/ariel/frontend/pin3/fesimple.cc

else

if HAVE_PIN324
fesimple.so : fesimple.o
	$(CXX) -shared -Wl,--hash-style=sysv \
	    $(PINTOOL_DIR)/$(TARGET)/runtime/pincrt/crtbeginS.o \
	    -Wl,-Bsymbolic -Wl,--version-script=$(PINTOOL_DIR)/source/include/pin/pintool.ver \
	    -o fesimple.so fesimple.o \
	    -L$(PINTOOL_DIR)/$(TARGET)/runtime/pincrt \
	    -L$(PINTOOL_DIR)/$(TARGET)/lib \
	    -L$(PINTOOL_DIR)/$(TARGET)/lib-ext \
	    -L$(PINTOOL_DIR)/extras/xed-$(XED_ARCH)/lib -lpin -lxed \
	    $(PINTOOL_DIR)/$(TARGET)/runtime/pincrt/crtendS.o \
	    -lpin3dwarf -ldl-dynamic -nostdlib -lc++ -lc++abi -lm-dynamic -lc-dynamic -lunwind-dynamic

fesimple.o : $(top_srcdir)/src/sst/elements/ariel/frontend/pin3/fesimple.cc
	$(CXX) \
	    $(PIN_CPPFLAGS) $(PIN_CPPFLAGS_COMPILER) $(AM_CPPFLAGS) $(CPPFLAGS) \
	    -I$(top_srcdir)/src/sst/elements/ariel \
	    -I$(PINTOOL_DIR)/source/include/pin \
	    -I$(PINTOOL_DIR)/source/include/pin/gen \
	    -isystem $(PINTOOL_DIR)/extras/cxx/include \
	    -isystem $(PINTOOL_DIR)/extras/crt/include \
	    -isystem $(PINTOOL_DIR)/extras/crt/include/arch-$(BIONIC_ARCH) \
	    -isystem $(PINTOOL_DIR)/extras/crt/include/kernel/uapi \
	    -isystem $(PINTOOL_DIR)/extras/crt/include/kernel/uapi/asm-x86 \
	    -I$(PINTOOL_DIR)/extras/components/include \
	    -I$(PINTOOL_DIR)/extras/xed-$(XED_ARCH)/include/xed \
	    -I$(PINTOOL_DIR)/source/tools/Utils \
	    -I$(PINTOOL_DIR)/source/tools/InstLib \
	    -O3 -c $(top_srcdir)/src/sst/elements/ariel/frontend/pin3/fesimple.cc
else

fesimple.so : fesimple.o
	$(CXX) -shared -Wl,--hash-style=sysv \
	    $(PINTOOL_DIR)/$(TARGET)/runtime/pincrt/crtbeginS.o \
	    -Wl,-Bsymbolic -Wl,--version-script=$(PINTOOL_DIR)/source/include/pin/pintool.ver \
	    -o fesimple.so fesimple.o \
	    -L$(PINTOOL_DIR)/$(TARGET)/runtime/pincrt \
	    -L$(PINTOOL_DIR)/$(TARGET)/lib \
	    -L$(PINTOOL_DIR)/$(TARGET)/lib-ext \
	    -L$(PINTOOL_DIR)/extras/xed-$(XED_ARCH)/lib -lpin -lxed \
	    $(PINTOOL_DIR)/$(TARGET)/runtime/pincrt/crtendS.o \
	    -lpin3dwarf -ldl-dynamic -nostdlib -lstlport-dynamic -lm-dynamic -lc-dynamic -lunwind-dynamic

fesimple.o : $(top_srcdir)/src/sst/elements/ariel/frontend/pin3/fesimple.cc
	$(CXX) \
	    $(PIN_CPPFLAGS) $(PIN_CPPFLAGS_COMPILER) $(AM_CPPFLAGS) $(CPPFLAGS) \
	    -I$(top_srcdir)/src/sst/elements/ariel \
	    -I$(PINTOOL_DIR)/source/include/pin \
	    -I$(PINTOOL_DIR)/source/include/pin/gen \
	    -isystem $(PINTOOL_DIR)/extras/stlport/include \
	    -isystem $(PINTOOL_DIR)/extras/libstdc++/include \
	    -isystem $(PINTOOL_DIR)/extras/crt/include \
	    -isystem $(PINTOOL_DIR)/extras/crt/include/arch-$(BIONIC_ARCH) \
	    -isystem $(PINTOOL_DIR)/extras/crt/include/kernel/uapi \
	    -isystem $(PINTOOL_DIR)/extras/crt/include/kernel/uapi/asm-x86 \
	    -I$(PINTOOL_DIR)/extras/components/include \
	    -I$(PINTOOL_DIR)/extras/xed-$(XED_ARCH)/include/xed \
	    -I$(PINTOOL_DIR)/source/tools/Utils \
	    -I$(PINTOOL_DIR)/source/tools/InstLib \
	    -O3 -c $(top_srcdir)/src/sst/elements/ariel/frontend/pin3/fesimple.cc
endif
endif

install-exec-hook:
	$(MKDIR_P) $(libexecdir)
	$(INSTALL) fesimple.so $(libexecdir)/fesimple.so
	$(MKDIR_P) $(pkglibdir)
	$(SST_REGISTER_TOOL) SST_ELEMENT_SOURCE     ariel=$(abs_srcdir)
	$(SST_REGISTER_TOOL) SST_ELEMENT_TESTS      ariel=$(abs_srcdir)/tests
	$(SST_REGISTER_TOOL) SST_ELEMENT_TESTSUITES ariel=$(abs_srcdir)/testsuites


else

all-local : fesimple.so

fesimple.so : fesimple.o
	$(CXX) -shared -Wl,--hash-style=sysv -Wl,-Bsymbolic \
	    -o fesimple.so fesimple.o \
	    -Wl,--version-script=$(PINTOOL_DIR)/source/include/pin/pintool.ver \
	    -L$(PINTOOL_DIR)/$(TARGET)/lib \
	    -L$(PINTOOL_DIR)/$(TARGET)/lib-ext \
	    -L$(PINTOOL_DIR)/$(TARGET)/runtime/glibc \
	    -L$(PINTOOL_DIR)/extras/xed-$(XED_ARCH)/lib \
	    -ldl -lpin -lxed -lpindwarf -ldl -lpthread -lrt $(LIBZ_LDFLAGS) $(LIBZ_LIB)

fesimple.o : $(top_srcdir)/src/sst/elements/ariel/frontend/simple/fesimple.cc
	$(CXX) $(AM_CXXFLAGS) $(CXXFLAGS) $(AM_CPPFLAGS) $(CPPFLAGS) \
	    $(LIBZ_CPPFLAGS) \
	    -DTARGET_IA32E -DHOST_IA32E -fPIC -DTARGET_LINUX \
	    -I./ \
	    -I$(PINTOOL_DIR)/source/include/pin \
	    -I$(PINTOOL_DIR)/ \
	    -I$(PINTOOL_DIR)/extras/components/include \
	    -I$(PINTOOL_DIR)/source/include/pin/gen/ \
	    -I$(PINTOOL_DIR)/extras/xed-$(XED_ARCH)/include \
	    -I$(top_srcdir)/src/sst \
	    -I$(top_srcdir)/src/sst/elements/ariel \
	    -O3 -fomit-frame-pointer -c $(top_srcdir)/src/sst/elements/ariel/frontend/simple/fesimple.cc

install-exec-hook:
	$(MKDIR_P) $(libexecdir)
	$(INSTALL) fesimple.so $(libexecdir)/fesimple.so
	$(SST_REGISTER_TOOL) SST_ELEMENT_SOURCE     ariel=$(abs_srcdir)
	$(SST_REGISTER_TOOL) SST_ELEMENT_TESTS      ariel=$(abs_srcdir)/tests
	$(SST_REGISTER_TOOL) SST_ELEMENT_TESTSUITES ariel=$(abs_srcdir)/testsuites

# HAVE_PIN3
endif

# HAVE_PINTOOL
endif

# HAVE_OSX....
endif
