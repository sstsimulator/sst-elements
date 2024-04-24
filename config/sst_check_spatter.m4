AC_DEFUN([SST_CHECK_SPATTER],
[
  sst_check_spatter_happy="yes"

  AC_ARG_WITH([spatter],
    [AS_HELP_STRING([--with-spatter@<:@=DIR@:>@],
      [Specify the root directory for Spatter])])

  CXXFLAGS_saved="$CXXFLAGS"
  CPPFLAGS_saved="$CPPFLAGS"
  LDFLAGS_saved="$LDFLAGS"
  LIBS_saved="$LIBS"

  AS_IF([test "$sst_check_spatter_happy" = "yes"], [
    AS_IF([test ! -z "$with_spatter" -a "$with_spatter" != "yes"],
      [SPATTER_CPPFLAGS="-I$with_spatter/../src/ -I$with_spatter/../src/Spatter -I$with_spatter/_deps/nlohmann_json-src/include"
       CXXFLAGS="$AM_CXXFLAGS $CXXFLAGS"
       CPPFLAGS="$SPATTER_CPPFLAGS $AM_CPPFLAGS $CPPFLAGS"
       SPATTER_LDFLAGS="-L$with_spatter/src/Spatter -L$with_spatter/src/"
       LDFLAGS="$SPATTER_LDFLAGS $AM_LDFLAGS $LDFLAGS"
       SPATTER_LIB="-lSpatter"
       SPATTER_LIBDIR="$with_spatter/src/Spatter/"],
      [SPATTER_CXXFLAGS=
       SPATTER_CPPFLAGS=
       SPATTER_LDFLAGS=
       SPATTER_LIB=
       SPATTER_LIBDIR=])])
  
  AC_LANG_PUSH([C++])
  AC_CHECK_HEADERS([nlohmann/json.hpp \
                    Configuration.hh \
                    Input.hh \
                    JSONParser.hh \
                    PatternParser.hh \
                    SpatterTypes.hh \
                    AlignedAllocator.hh \
                    Timer.hh], [], [sst_check_spatter_happy="no"])
  AC_LANG_POP([C++])

  CXXFLAGS="$CXXFLAGS_saved"
  CPPFLAGS="$CPPFLAGS_saved"
  LDFLAGS="$LDFLAGS_saved"
  LIBS="$LIBS_saved"

  AC_SUBST([SPATTER_CPPFLAGS])
  AC_SUBST([SPATTER_LDFLAGS])
  AC_SUBST([SPATTER_LIB])
  AC_SUBST([SPATTER_LIBDIR])
  AS_IF([test "x$sst_check_spatter_happy" = "xyes"], [AC_DEFINE([HAVE_SPATTER],[1],[Defines whether we have Spatter])])
  AM_CONDITIONAL([USE_SPATTER], [test "x$sst_check_spatter_happy" = "xyes"])
  AC_DEFINE_UNQUOTED([SPATTER_LIBDIR], ["$SPATTER_LIBDIR"], [Path to Spatter library])

  AC_MSG_CHECKING([for Spatter])
  AC_MSG_RESULT([$sst_check_spatter_happy])
  AS_IF([test "$sst_check_spatter_happy" = "no" -a ! -z "$with_spatter"], [$3])
  AS_IF([test "$sst_check_spatter_happy" = "yes"], [$1], [$2])
])