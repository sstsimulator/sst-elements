ACLOCAL_AMFLAGS = -I config

SUBDIRS = mtgl test

EXTRA_DIST = \
	AUTHORS \
	COPYRIGHT \
	FIRST_TUTORIAL \
	LICENSE \
	README \
	doc/Doxyfile

if ON_MTA
mostlyclean-local:
	/bin/rm -f *.ti
	/bin/rm -f *.pl
endif

