AM_CPPFLAGS = -I$(top_srcdir)/include

lib_LTLIBRARIES = libcprops.la

SUBDIRS = man3 . svc example

libcprops_la_SOURCES = \
					   util.c \
					   log.c \
					   collection.c \
					   mempool.c \
					   vector.c \
					   str.c \
					   linked_list.c \
					   heap.c \
					   priority_list.c \
					   hashtable.c \
					   hashlist.c \
					   mtab.c \
					   trie.c \
					   avl.c \
					   rb.c \
					   splay.c \
					   sorted_hash.c \
					   nary.c \
					   multimap.c \
					   thread.c \
					   socket.c \
					   client.c \
					   http.c \
					   httpclient.c
libcprops_la_LIBADD =
libcprops_la_CFLAGS =
cpropsdir = $(includedir)/cprops
nodist_cprops_HEADERS = \
						http.h \
						config.h
dist_cprops_HEADERS = \
					  common.h \
					  collection.h \
					  mempool.h \
					  util.h \
					  log.h \
					  vector.h \
					  str.h \
					  heap.h \
					  linked_list.h \
					  priority_list.h \
					  hashtable.h \
					  hashlist.h \
					  trie.h \
					  avl.h \
					  rb.h \
					  mtab.h \
					  splay.h \
					  sorted_hash.h \
					  nary.h \
					  multimap.h \
					  thread.h \
					  socket.h \
					  client.h \
					  httpclient.h

if COMPILE_CP_DBMS
libcprops_la_SOURCES += db.c
dist_cprops_HEADERS += db.h
endif
if COMPILE_CP_DBMS_POSTGRES_STATIC
libcprops_la_SOURCES += db_postgres.c
dist_cprops_HEADERS += db_postgres.h
libcprops_la_LIBADD += @POSTGRESQL_LDFLAGS@
libcprops_la_CFLAGS += @POSTGRESQL_CFLAGS@
endif
if COMPILE_CP_DBMS_MYSQL_STATIC
libcprops_la_SOURCES += db_mysql.c
dist_cprops_HEADERS += db_mysql.h
libcprops_la_LIBADD += @MYSQL_LDFLAGS@
libcprops_la_CFLAGS += @MYSQL_CFLAGS@
endif

EXTRA_DIST = INSTALL.WIN32 \
			 README.WIN32 \
			 LIBVERSION_CP_DBMS_MYSQL \
			 LIBVERSION_CP_DBMS_POSTGRES \
			 Makefile.vc \
			 config.h.vc \
			 configure.bat \
			 test \
			 testhttpsrv.c

distclean-local:
	$(RM) *~ ".#"*
