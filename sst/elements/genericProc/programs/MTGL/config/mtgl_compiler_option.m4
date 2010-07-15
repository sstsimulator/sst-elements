dnl MTGL compiler option

AC_DEFUN([MTGL_COMPILER_OPTION],[
  dnl
  dnl Can be used to specify specific characteristics about the compiler.
  dnl 
AC_ARG_WITH(compiler,
AS_HELP_STRING([--with-compiler],[Use --with-compiler=mingw to build mingw executables and libraries]),
[WITH_COMPILER=$withval],
[WITH_COMPILER=notset]
)
if test X${WITH_COMPILER} = Xmingw ; then
  if test $build_os = cygwin ; then :; else
     AC_MSG_ERROR([mingw code can only be built on a cygwin system])
  fi
  CC=mingw
fi

if test X${WITH_COMPILER} = Xmingw ; then :; else
  if test X${WITH_COMPILER} = Xnotset ; then :; else
     AC_MSG_ERROR([Sorry, --with-compiler feature is only implemented for mingw right now])
  fi
fi


])
