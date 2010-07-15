dnl -*- Autoconf -*-
dnl

AC_DEFUN([MTGL_CHECK_QTHREADS],[
  AC_ARG_WITH([qthreads],
    [AC_HELP_STRING([--with-qthreads=@<:@DIR@:>@],
       [Enable qthreads threading support, optionally looking for qthreads in DIR])])
  AC_ARG_WITH([cprops],
    [AC_HELP_STRING([--with-cprops=DIR],
       [Look for cprops in DIR, if not automatically found.  Only necessary if qthreads support is enabled.])])

  AS_IF([test -n "$with_cprops" -a "x$with_cprops" != "xno"],
        [AS_IF([test "$with_cprops" != xyes],
		       [CPPFLAGS="$CPPFLAGS -I$with_cprops/include"
			    LDFLAGS="$LDFLAGS -L$with_cprops/lib"])
		 AC_SEARCH_LIBS([cp_hashtable_create],
		 		[cprops "cprops -ldl" "cprops -ldl -lssl" "cprops -lsocket -lnsl" "cprops -lsocket -lnsl -lssl" "cprops -lsocket -lnsl -ldl" "cprops -lsocket -lnsl -ldl -lssl"],
				[],
				[AC_MSG_ERROR([Cprops could not be found])])])
  AS_IF([test -n "$with_qthreads" -a "x$with_qthreads" != "xno"],
	    [AC_DEFINE([USING_QTHREADS], [1], [Define to 1 if qthreads support enabled])
         AS_IF([test "$with_qthreads" != "yes"],
               [CPPFLAGS="$CPPFLAGS -I$with_qthreads/include"
                LDFLAGS="$LDFLAGS -L$with_qthreads/lib"])
		 AC_CHECK_LIB([qthread], [qthread_init], [],
			          [AC_MSG_ERROR([Qthreads could not be found])])])
])
