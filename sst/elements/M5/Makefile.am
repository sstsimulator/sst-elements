# -*- Makefile -*-
#
#

EXTRA_DIST = m5.xml

compdir = $(pkglibdir)
comp_LTLIBRARIES = libm5C.la
libm5C_la_SOURCES = \
    baseCache.cpp \
    bounce.h \
    bus.cpp \
    busPlus.cpp \
    busPlus.h \
    debug.cpp \
    dummyPhysicalMemory.h \
    dummySystem.h \
    dummyThreadContext.h \
    exeBackingMem.h \
    factory.h \
    loadMemory.cpp \
    loadMemory.h \
    m5.cpp \
    m5.h \
    m5lib.cpp \
    memEvent.h \
    memLink.cpp \
    memLink.h \
    physicalPlus.cpp \
    physicalPlus.h \
    physicalMemory.cpp \
    port2link.cpp \
    port2link.h \
    process.h \
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
libm5C_la_SOURCES += DRAMSimWrap.cpp DRAMSimWrap.h
endif

libm5C_la_CPPFLAGS = $(BOOST_CPPFLAGS) $(MPI_CPPFLAGS) $(M5_CPPFLAGS) \
            $(DRAMSIM_CPPFLAGS) \
            -I$(top_srcdir)/sst/elements/include
libm5C_la_LDFLAGS = -module -avoid-version $(M5_LDFLAGS) $(DRAMSIM_LDFLAGS)
libm5C_la_LIBADD = $(M5_LIB) $(DRAMSIM_LIB)
