# -*- Makefile -*-
#
#

compdir = $(pkglibdir)
comp_LTLIBRARIES = libm5C.la

libm5C_la_SOURCES = \
    barrier.h \
    baseCache.cpp \
    bounce.h \
    bridge.cpp \
    bus.cpp \
    busPlus.cpp \
    busPlus.h \
    debug.cpp \
    debug.h \
    trace.cpp\
    trace.h\
    dummySystem.h \
    dummyThreadContext.h \
    factory.h \
    factory.cpp \
    loadMemory.cpp \
    loadMemory.h \
    log.h \
    m5.cpp \
    m5.h \
    m5lib.cpp \
    memEvent.h \
    memLink.cpp \
    memLink.h \
    paramHelp.h \
    physical2.h \
    physicalPlus.cpp \
    physicalPlus.h \
    physicalMemory.cpp \
	portLink.h \
	portLink.cpp \
    power.cpp \
    process.h \
    rawEvent.h \
    simpleCpu.cpp \
    syscall.cpp \
    syscall.h \
    system.h \
    util.cpp \
    util.h

if USE_M5_O3
libm5C_la_SOURCES += O3Cpu.cpp
endif


if HAVE_DRAMSIM 
libm5C_la_SOURCES += DRAMSimWrap.cpp DRAMSimWrap.h \
            DRAMSimWrap2.cpp DRAMSimWrap2.h \
            DRAMSimWrapPlus.cpp DRAMSimWrapPlus.h \
            DRAMSimWrap2Plus.cpp DRAMSimWrap2Plus.h
endif

if HAVE_PHXSIM 
libm5C_la_SOURCES += PHXSimWrap.cpp PHXSimWrap.h
endif

libm5C_la_CPPFLAGS = $(BOOST_CPPFLAGS) $(MPI_CPPFLAGS) $(M5_CPPFLAGS) \
            $(DRAMSIM_CPPFLAGS) \
            $(PHXSIM_CPPFLAGS) \
	    $(PYTHON_CPPFLAGS) \
            -I$(top_srcdir)/sst/elements/include \
            -I$(top_srcdir)/sst/elements

libm5C_la_LDFLAGS = -module -avoid-version $(M5_LDFLAGS) $(DRAMSIM_LDFLAGS) $(PHXSIM_LDFLAGS)
libm5C_la_LIBADD = $(M5_LIB) $(DRAMSIM_LIB) $(PHXSIM_LIB)

