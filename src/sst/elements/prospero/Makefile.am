
AM_CPPFLAGS = \
        $(MPI_CPPFLAGS) \
        -DPROSPERO_TOOL_DIR="$(libexecdir)"

compdir = $(pkglibdir)
comp_LTLIBRARIES = libprospero.la

libprospero_la_SOURCES = \
        proscpu.h \
        proscpu.cc \
	prosreader.h \
	prostextreader.h \
	prostextreader.cc \
	prosbinaryreader.h \
	prosbinaryreader.cc \
	prosmemmgr.h \
	prosmemmgr.cc

EXTRA_DIST = \
        tests/array/trace-binary.py \
        tests/array/trace-binary-withdramsim.py \
        tests/array/trace-compressed.py \
        tests/array/trace-compressed-withdramsim.py \
        tests/array/trace-text.py \
        tests/array/trace-text-withdramsim.py \
        tests/array/trace-common.py \
        tests/array/array.c \
        tests/array/Makefile \
        tests/testsuite_default_prospero.py \
        tracetool/Makefile \
        tracetool/Makefile.osx \
        tracetool/sstmemtrace.cc \
        tracetool/api/Makefile \
        tracetool/api/Makefile.osx \
        tracetool/api/prospero.c \
        tracetool/api/prospero.h

libprospero_la_LDFLAGS = -module -avoid-version
libprospero_la_LIBADD = $(SHM_LIB)

install-exec-local:
	$(SST_REGISTER_TOOL) SST_ELEMENT_SOURCE     prospero=$(abs_srcdir)
	$(SST_REGISTER_TOOL) SST_ELEMENT_TESTS      prospero=$(abs_srcdir)/tests

if USE_LIBZ
libprospero_la_LIBADD += -lz

libprospero_la_SOURCES += \
	prosbingzreader.h \
	prosbingzreader.cc
endif

if HAVE_PINTOOL

bin_PROGRAMS = sst-prospero-trace
sst_prospero_trace_SOURCES = runprosperotrace.cc
AM_CPPFLAGS +=  $(PINTOOL_CPPFLAGS)

if SST_COMPILE_OSX

all-local: tracetool/sstmemtrace.cc
	$(CXX) -O3 -shared \
	$(CPPFLAGS) \
	$(LIBZ_CPPFLAGS) \
	-DBIGARRAY_MULTIPLIER=1 \
	-DTARGET_IA32E -DHOST_IA32E -fPIC -DTARGET_MAC \
	-I$(PINTOOL_DIR)/source/include/pin \
	-I$(PINTOOL_DIR)/ \
	-I$(PINTOOL_DIR)/extras/components/include \
	-I$(PINTOOL_DIR)/source/include/pin/gen/ \
	-I$(PINTOOL_DIR)/extras/xed-intel64/include \
	-I$(top_srcdir)/sst \
	-fomit-frame-pointer -fno-stack-protector \
	-Wl,-exported_symbols_list \
	-Wl,$(PINTOOL_DIR)/source/include/pin/pintool.exp \
	-L$(PINTOOL_DIR)/intel64/lib \
	-L$(PINTOOL_DIR)/intel64/lib-ext \
	-L$(PINTOOL_DIR)/extras/xed-intel64/lib \
	-o prosperotrace.dylib $(top_srcdir)/src/sst/elements/prospero/tracetool/sstmemtrace.cc \
	-stdlib=libstdc++ \
	-lpin -lxed -lpindwarf -lpthread $(LIBZ_LDFLAGS) $(LIBZ_LIB)

install-exec-hook: prosperotrace.dylib
	cp prosperotrace.dylib $(libexecdir)/prosperotrace.dylib

else

all-local: prosperotrace.so

if HAVE_PIN3

prosperotrace.so : prosperotrace.o
	$(CXX) -shared -Wl,--hash-style=sysv \
	    $(PINTOOL_DIR)/intel64/runtime/pincrt/crtbeginS.o \
	    -Wl,-Bsymbolic -Wl,--version-script=$(PINTOOL_DIR)/source/include/pin/pintool.ver -fabi-version=2 \
	    -o prosperotrace.so prosperotrace.o \
	    -L$(PINTOOL_DIR)/intel64/runtime/pincrt \
	    -L$(PINTOOL_DIR)/intel64/lib \
	    -L$(PINTOOL_DIR)/intel64/lib-ext \
	    -L$(PINTOOL_DIR)/extras/xed-intel64/lib -lpin -lxed \
	    $(PINTOOL_DIR)/intel64/runtime/pincrt/crtendS.o \
	    -lpin3dwarf -ldl-dynamic -nostdlib -lstlport-dynamic -lm-dynamic -lc-dynamic -lunwind-dynamic

prosperotrace.o : $(top_srcdir)/src/sst/elements/prospero/tracetool/sstmemtrace.cc
	$(CXX) -Wall -Werror -Wno-unknown-pragmas -D__PIN__=1 -DPIN_CRT=1 -DHAVE_PIN3=1\
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
	    -I$(PINTOOL_DIR)/extras/components/include\
	    -I$(PINTOOL_DIR)/extras/xed-intel64/include/xed \
	    -I$(PINTOOL_DIR)/source/tools/Utils \
	    -I$(PINTOOL_DIR)/source/tools/InstLib \
	    -O3 -fomit-frame-pointer -fno-strict-aliasing \
	    -o prosperotrace.o -c $(top_srcdir)/src/sst/elements/prospero/tracetool/sstmemtrace.cc

else

prosperotrace.so : prosperotrace.o
	$(CXX) -shared -Wl,--hash-style=sysv -Wl,-Bsymbolic \
	    -o prosperotrace.so prosperotrace.o \
	    -Wl,--version-script=$(PINTOOL_DIR)/source/include/pin/pintool.ver \
	    -L$(PINTOOL_DIR)/intel64/lib \
	    -L$(PINTOOL_DIR)/intel64/lib-ext \
	    -L$(PINTOOL_DIR)/intel64/runtime/glibc \
	    -L$(PINTOOL_DIR)/extras/xed-intel64/lib \
	    -ldl -lpin -lxed -lpindwarf -lpthread -lrt $(LIBZ_LDFLAGS) $(LIBZ_LIB)

prosperotrace.o : $(top_srcdir)/src/sst/elements/prospero/tracetool/sstmemtrace.cc
	$(CXX) $(CPPFLAGS) \
	    $(LIBZ_CPPFLAGS) \
	    -DTARGET_IA32E -DHOST_IA32E -fPIC -DTARGET_LINUX \
	    -I$(PINTOOL_DIR)/ \
	    -I$(PINTOOL_DIR)/source/include/pin \
	    -I$(PINTOOL_DIR)/source/include/pin/gen \
	    -I$(PINTOOL_DIR)/extras/components/include \
	    -I$(PINTOOL_DIR)/extras/xed-intel64/include \
	    -I$(top_srcdir)/sst \
	    -O3 -fomit-frame-pointer -o prosperotrace.o \
	    -c $(top_srcdir)/src/sst/elements/prospero/tracetool/sstmemtrace.cc

endif

install-exec-hook: prosperotrace.so
	cp prosperotrace.so $(libexecdir)/prosperotrace.so

endif
endif
