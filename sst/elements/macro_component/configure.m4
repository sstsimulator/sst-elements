dnl -*- Autoconf -*-

AC_DEFUN([SST_macro_component_CONFIG], [
  SST_CHECK_SSTMACRO([have_sstmacro=1],
              [have_sstmacro=0],
              [AC_MSG_ERROR([SST Macro requested but not found])])
  AS_IF([test "$have_sstmacro" = 1],
   [AC_DEFINE([HAVE_SSTMACRO], [1], [Define if you have the SST Macro library.])])

  AS_IF([test "$have_sstmacro" = "1"], [$1], [$2])
])
