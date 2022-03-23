AC_DEFUN([SST_CHECK_OTF2],
[
  sst_check_otf2_happy="yes"

  AC_ARG_WITH([otf2],
    [AS_HELP_STRING([--with-otf2@<:@=DIR@:>@],
      [Use OTF-2 package installed in optionally specified DIR])])

  AS_IF([test "$with_otf2" = "no"], [sst_check_otf2_happy="no"])

  CXXFLAGS_saved="$CXXFLAGS"
  CPPFLAGS_saved="$CPPFLAGS"
  LDFLAGS_saved="$LDFLAGS"
  LIBS_saved="$LIBS"

  AS_IF([test "$sst_check_otf2_happy" = "yes"], [
    AS_IF([test ! -z "$with_otf2" -a "$with_otf2" != "yes"],
      [OTF2_PATH="$with_otf2/bin/:$PATH"
       OTF2_CPPFLAGS="-I$with_otf2/include"
       CPPFLAGS="$OTF2_CPPFLAGS $AM_CPPFLAGS $CPPFLAGS"
       CXXFLAGS="$AM_CXXFLAGS $CXXFLAGS"
       OTF2_LDFLAGS="-L$with_otf2/lib"
       LDFLAGS="$OTF2_LDFLAGS $AM_LDFLAGS $LDFLAGS"],
      [OTF2_CPPFLAGS=
       OTF2_LDFLAGS=])])
  
  AC_PATH_PROG([OTF2_CONFIG_TOOL],[otf2-config],[],[$OTF2_PATH])

  AS_IF([test "x$OTF2_CONFIG_TOOL" = "x"],
		[sst_check_otf2_happy="no"])

  CXXFLAGS="$CXXFLAGS_saved"
  CPPFLAGS="$CPPFLAGS_saved"
  LDFLAGS="$LDFLAGS_saved"
  LIBS="$LIBS_saved"

  AC_SUBST([OTF2_CPPFLAGS])
  AC_SUBST([OTF2_LDFLAGS])
  AC_SUBST([OTF2_CONFIG_TOOL])
  AC_SUBST([OTF2_PATH])

  AM_CONDITIONAL([USE_OTF2], [test "x$sst_check_otf2_happy" = "xyes"])
  AS_IF([test "$sst_check_otf2_happy" = "yes"], [AC_DEFINE([HAVE_OTF2],[1],[Defines whether we have the OFT library])])

  AC_MSG_CHECKING([for OTF-2 library])
  AC_MSG_RESULT([$sst_check_otf2_happy])
  AS_IF([test "$sst_check_otf2_happy" = "no" -a ! -z "$with_otf2" -a "$with_otf2" != "no"], [$3])
  AS_IF([test "$sst_check_otf2_happy" = "yes"], [$1], [$2])
])
