if SST_USE_ARIEL_MPI
CC_LOCAL = $(ARIEL_MPICC)
CFLAGS_LOCAL = -fopenmp $(ARIEL_MPI_CFLAGS) -DENABLE_ARIEL_MPI=1
else
CC_LOCAL = $(CC)
CFLAGS_LOCAL =
endif

AM_CPPFLAGS += \
	$(MPI_CPPFLAGS) \
	-I$(top_srcdir)/src

lib_LTLIBRARIES = libarielapi.la

libarielapi_la_SOURCES = \
    arielapi.c \
    arielapi.h

libarielapi_la_LDFLAGS = -module -avoid-version -shared
