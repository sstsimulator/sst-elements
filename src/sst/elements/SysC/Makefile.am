
AM_CPPFLAGS = -Wall \
	$(MPI_CPPFLAGS) \
	$(SST_SYSTEMC_CPPFLAGS) \
	$(SST_CXX0X_FLAGS)

AM_LDFLAGS = -module -avoid-version -lm 

compdir = $(pkglibdir)
comp_LTLIBRARIES = libSysC.la 

libSysC_la_SOURCES = SysC.cc \
	common/controller.cc \
	common/controller.h \
	common/memorywedge.h \
	common/memorywedge.cc \
	common/simplewedge.h \
	common/simplewedge.cc \
	common/memorymanager.h \
	common/memorymanager.cc \
	TLM/adapter.h \
	TLM/adapter.cc \
	TLM/TLMsimplemem.h \
	TLM/TLMsimplemem.cc 

libSysC_la_LDFLAGS = -module -avoid-version -lm \
	$(SST_SYSTEMC_LDFLAGS) \
        $(SST_SYSTEMC_LIBS)

libSysC_la_LIBADD = \
	$(SST_SYSTEMC_LIB)

#noinst_PROGRAMS = tests/SysCGeneratorAnouncerTest

#tests_SysCGeneratorAnouncerTest_SOURCES = \
#	common/TLMgenerator.h \
#	common/TLManouncer.h \
#	tests/TLMGeneratorAnouncerTest.cc

#tests_SysCGeneratorAnouncerTest_LDFLAGS = -module -avoid-version -lm \
#																					$(SST_SYSTEMC_LDFLAGS) \
#																					$(SST_SYSTEMC_LIBS) \
#																					$(SST_SYSTEMC_LIBDIR)
