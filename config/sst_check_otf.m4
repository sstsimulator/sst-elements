AC_DEFUN([SST_CHECK_OTF],
[
  sst_check_otf_happy="yes"

  AC_ARG_WITH([otf],
    [AS_HELP_STRING([--with-otf@<:@=DIR@:>@],
      [Use OTF package installed in optionally specified DIR])])

  AS_IF([test "$with_otf" = "no"], [sst_check_otf_happy="no"])

  CPPFLAGS_saved="$CPPFLAGS"
  LDFLAGS_saved="$LDFLAGS"

  AS_IF([test "$sst_check_otf_happy" = "yes"], [
    AS_IF([test ! -z "$with_otf" -a "$with_otf" != "yes"],
      [OTF_PATH="$with_otf/bin/:$PATH"
       OTF_CPPFLAGS="-I$with_otf/include"
       CPPFLAGS="$OTF_CPPFLAGS $CPPFLAGS"
       OTF_LDFLAGS="-L$with_otf/lib"
       LDFLAGS="$OTF_LDFLAGS $LDFLAGS"],
      [OTF_CPPFLAGS=
       OTF_LDFLAGS=])])
  
  AC_PATH_PROG([OTF_CONFIG_TOOL],[otfconfig],[],[$OTF_PATH])

  AS_IF([test "x$OTF_CONFIG_TOOL" = "x"],
		[sst_check_otf_happy="no"])

  CPPFLAGS="$CPPFLAGS_saved"
  LDFLAGS="$LDFLAGS_saved"

  AC_SUBST([OTF_CPPFLAGS])
  AC_SUBST([OTF_LDFLAGS])
  AC_SUBST([OTF_CONFIG_TOOL])
  AC_SUBST([OTF_PATH])

  AM_CONDITIONAL([USE_OTF], [test "x$sst_check_otf_happy" = "xyes"])
  AS_IF([test "$sst_check_otf_happy" = "yes"], [AC_DEFINE([HAVE_OTF],[1],[Defines whether we have the OFT library])])

  AC_MSG_CHECKING([for OTF library])
  AC_MSG_RESULT([$sst_check_otf_happy])
  AS_IF([test "$sst_check_otf_happy" = "no" -a ! -z "$with_otf" -a "$with_otf" != "no"], [$3])
  AS_IF([test "$sst_check_otf_happy" = "yes"], [$1], [$2])
])
