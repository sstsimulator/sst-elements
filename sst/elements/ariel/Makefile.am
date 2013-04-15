
AM_CPPFLAGS = \
        $(BOOST_CPPFLAGS) \
        $(MPI_CPPFLAGS)

compdir = $(pkglibdir)
comp_LTLIBRARIES = libariel.la libarieltrace.la
libariel_la_SOURCES = \
        ariel.h \
        ariel.cc

libariel_la_LDFLAGS = -module -avoid-version
libariel_la_CPPFLAGS = -DARIEL_TRACE_LIB=$(libdir)/sst \
	$(CPPFLAGS) $(AM_CPPFLAGS)

# ---------------------------------------------------------------------------
# Build the tracing component which we need to call, this traps memory
# accesses etc using the PIN framework

libarieltrace_la_SOURCES = \
	arieltrace.cc

libarieltrace_la_LDFLAGS = \
	-Wl,--hash-style=sysv -shared -Wl,-Bsymbolic \
	-Wl,--version-script=$(PINTOOL_DIR)/source/include/pin/pintool.ver \
	-L$(PINTOOL_DIR)/Lib/ \
	-L$(PINTOOL_DIR)/ExtLib/ \
	-L$(PINTOOL_DIR)/extras/xed2-intel64/lib \
	-L$(PINTOOL_DIR)/intel64/lib \
	-L$(PINTOOL_DIR)/intel64/lib-ext

libarieltrace_la_LIBADD = \
	-L$(PINTOOL_DIR)/Lib/ \
	-L$(PINTOOL_DIR)/ExtLib/ \
	-L$(PINTOOL_DIR)/extras/xed2-intel64/lib \
	-L$(PINTOOL_DIR)/intel64/lib \
	-L$(PINTOOL_DIR)/intel64/lib-ext -lpin  -lxed -ldwarf \
	-lelf -ldl

libarieltrace_la_CPPFLAGS = \
	-I $(PINTOOL_DIR)/source/include/pin \
	-I $(PINTOOL_DIR)/source/include/pin/gen \
	-I $(PINTOOL_DIR)/extras/xed2-intel64/include \
	-I $(PINTOOL_DIR)/extras/components/include \
	-DBIGARRAY_MULTIPLIER=1 -DUSING_XED -DTARGET_IA32E \
	-DHOST_IA32E -fPIC -DTARGET_LINUX
	
libarieltrace_la_CXXFLAGS = \
	-fomit-frame-pointer
