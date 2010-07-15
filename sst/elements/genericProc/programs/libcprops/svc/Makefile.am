
SUBDIRS = . cpsp

if ENABLE_CPSVC

bin_PROGRAMS = cpsvc

INCLUDES = -I$(top_builddir)

CPROPSLIB = $(top_builddir)/libcprops.la
cpsvcdir = $(bindir)
noinst_HEADERS = file.h cgi.h cpsp.h cpsp_invoker.h
cpsvc_SOURCES = file.c cgi.c cpsp.c cpsp_invoker.c cpsvc.c cpsp_worker.c
cpsvc_LDADD = $(CPROPSLIB)
cpsvc_DEPENDENCIES = $(CPROPSLIB)
cpsvc_DATA = mime.types

EXTRA_DIST = $(cpsvc_DATA) \
			 Makefile.vc

endif
