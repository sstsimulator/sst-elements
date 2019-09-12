AC_DEFUN([SST_CHECK_CUDA],
[
  sst_check_cuda_happy="no"

  AC_ARG_WITH([cuda],
    [AS_HELP_STRING([--with-cuda@<:@=DIR@:>@],
      [Specify the root directory for CUDA])])

  CPPFLAGS_saved="$CPPFLAGS"
  LDFLAGS_saved="$LDFLAGS"
  LIBS_saved="$LIBS"

  AS_IF([test ! -z "$with_cuda" -a "$with_cuda" != "yes"],
         [CUDA_CPPFLAGS="-I$with_cuda/include -DHAVE_CUDA=1"
            CPPFLAGS="$CUDA_CPPFLAGS $CPPFLAGS"
            CUDA_LDFLAGS=""
            CUDA_LIBDIR="$with_cuda/lib64"
            CUDA_LIBS="-L$CUDA_LIBDIR"
            LDFLAGS="$CUDA_LDFLAGS $LDFLAGS"],
         [CUDA_CPPFLAGS=
            CUDA_LDFLAGS=
            CUDA_LIBDIR=
            CUDA_LIBS=
  ])

  WANT_CUDA_VERSION=8000

  AC_MSG_CHECKING([CUDA version is acceptable for SST])
  AC_LANG_PUSH(C++)
  AC_COMPILE_IFELSE(
         [AC_LANG_PROGRAM([[@%:@include "cuda.h"]],
               [[
                     #if CUDA_VERSION >= $WANT_CUDA_VERSION
                     // Version is fine
                     #else
                     #  error CUDA version is too old
                     #endif
               ]])
         ],
            [AC_MSG_RESULT([yes])
               sst_check_cuda_happy="yes"],
            [AC_MSG_RESULT([no])
               sst_check_cuda_happy="no"]
  )

  LIBS="$LIBS_saved"
  AC_LANG_POP(C++)

  CPPFLAGS="$CPPFLAGS_saved"
  LDFLAGS="$LDFLAGS_saved"
  LIBS="$LIBS_saved"

  AS_IF([test ! -z "$with_cuda" -a "$with_cuda" != "yes" -a "$sst_check_cuda_happy" = "no"],
         [AC_MSG_ERROR([CUDA provided but could not successfully compile or version is too old.])])

  AC_SUBST([CUDA_CPPFLAGS])
  AC_SUBST([CUDA_LDFLAGS])
  AC_SUBST([CUDA_LIBDIR])
  AC_SUBST([CUDA_LIBS])

  AS_IF([test "x$sst_check_cuda_happy" = "xyes"],
         [AC_DEFINE([HAVE_CUDA], [1], [Set to 1 CUDA was found])])
  AS_IF([test "$sst_check_cuda_happy" = "yes"],
         [AC_DEFINE_UNQUOTED([CUDA_LIBDIR], ["$CUDA_LIBDIR"], [Library directory where CUDA can be found.])])
  AM_CONDITIONAL([USE_CUDA], [test "x$sst_check_cuda_happy" = "xyes"])
  AS_IF([test "$sst_check_cuda_happy" = "yes"], [$1], [$2])

])
