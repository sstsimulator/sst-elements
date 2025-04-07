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
    $(srcdir)/arielapi.c \
    $(srcdir)/arielapi.h

libarielapi_la_LDFLAGS = -module -avoid-version -shared

all-local: copy-libarielapi

copy-libarielapi:
	@if test -f .libs/libarielapi.so; then \
		echo "Copying libarielapi.so to $(srcdir)"; \
		cp .libs/libarielapi.so $(srcdir)/libarielapi.so; \
	else \
		echo "libarielapi.so not found in .libs/; build may have failed or hasn't run yet."; \
	fi

clean-local:
	rm -f arielapi.o libarielapi.a libarielapi.so
	rm -f $(srcdir)/libarielapi.so
