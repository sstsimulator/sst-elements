
AC_DEFUN([SST_CORE_CHECK_INSTALL], [
	AC_ARG_WITH([sst-core],
	  [AS_HELP_STRING([--with-sst-core=@<:@=DIR@:>@],
	    [Use SST Discrete Event Core installed in DIR])])

  SST_CONFIG_TOOL=""
  SST_REGISTER_TOOL=""

  AS_IF( [test "x$with_sst_core" = "xyes"],
	 [AC_PATH_PROG([SST_CONFIG_TOOL], [sst-config], [], [$PATH])],
	 [AC_PATH_PROG([SST_CONFIG_TOOL], [sst-config], [], [$PATH$PATH_SEPARATOR$with_sst_core/bin])] )

  AC_MSG_CHECKING([for sst-config tool])
  AS_IF([test -x "$SST_CONFIG_TOOL"],
	[AC_MSG_RESULT([found $SST_CONFIG_TOOL])],
	[AC_MSG_ERROR([Unable to find sst-config in the PATH or in the sst-core directory], [1])])

  AS_IF( [test "x$with_sst_core" = "xyes"],
	 [AC_PATH_PROG([SST_REGISTER_TOOL], [sst-register], [], [$PATH])],
	 [AC_PATH_PROG([SST_REGISTER_TOOL], [sst-register], [], [$PATH$PATH_SEPARATOR$with_sst_core/bin])] )

  AC_MSG_CHECKING([for sst-register tool])
  AS_IF([test -x "$SST_REGISTER_TOOL"],
	[AC_MSG_RESULT([found $SST_REGISTER_TOOL])],
	[AC_MSG_ERROR([Unable to find sst-register in the PATH or in sst-core directory], [1])])

  SST_PREFIX=`$SST_CONFIG_TOOL --prefix`
  SST_CPPFLAGS=`$SST_CONFIG_TOOL --CPPFLAGS`
  SST_CFLAGS=`$SST_CONFIG_TOOL --CFLAGS`
  SST_CXXFLAGS=`$SST_CONFIG_TOOL --CXXFLAGS`
  SST_LDFLAGS=`$SST_CONFIG_TOOL --LDFLAGS`
  SST_LIBS=`$SST_CONFIG_TOOL --LIBS`
 
  BOOST_CPPFLAGS=`$SST_CONFIG_TOOL --BOOST_CPPFLAGS`
  BOOST_LDFLAGS=`$SST_CONFIG_TOOL --BOOST_LDFLAGS`
  BOOST_LIBS=`$SST_CONFIG_TOOL --BOOST_LIBS`
  AS_IF([test "${BOOST_LIBS##*-}" == "mt"],
    [BOOST_LIB_SUFFIX="-mt"],
    [BOOST_LIB_SUFFIX=""])

  PYTHON_CPPFLAGS=`$SST_CONFIG_TOOL --PYTHON_CPPFLAGS`
  PYTHON_LDFLAGS=`$SST_CONFIG_TOOL --PYTHON_LDFLAGS`

  CPPFLAGS="$CPPFLAGS $SST_CPPFLAGS $PYTHON_CPPFLAGS $BOOST_CPPFLAGS -I$SST_PREFIX/include/sst/core -I$SST_PREFIX/include"
  CFLAGS="$CFLAGS $SST_CFLAGS"
  CXXFLAGS="$CXXFLAGS $SST_CXXFLAGS"
  LDFLAGS="$LDFLAGS $SST_LDFLAGS $BOOST_LDFLAGS $PYTHON_LDFLAGS"
  LIBS="$SST_LIBS $BOOST_LIBS $LIBS"

  AC_SUBST([SST_CONFIG_TOOL])
  AC_SUBST([SST_REGISTER_TOOL])
  AC_SUBST([SST_PREFIX])
  AC_SUBST([BOOST_LIB_SUFFIX])
])
