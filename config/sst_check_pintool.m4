AC_DEFUN([SST_CHECK_PINTOOL],
[
  sst_check_pintool_happy="yes"

  AC_ARG_WITH([pin],
    [AS_HELP_STRING([--with-pin@<:@=DIR@:>@],
      [Use the PIN binary instrumentation tool in DIR])])

  AS_IF([test "$with_pin" = "no"], [sst_check_pintool_happy="no"])

  CPPFLAGS_saved="$CPPFLAGS"
  LDFLAGS_saved="$LDFLAGS"
  PATH_saved="$PATH"

  AS_IF([test "$sst_check_pintool_happy" = "yes"], [
    AS_IF([test ! -z "$with_pin" -a "$with_pin" != "yes"],
      [PINTOOL_PATH="$with_pin:$PATH"
       PINTOOL_DIR="$with_pin"
       PINTOOL_CPPFLAGS="-I$with_pin/source/include -DHAVE_PIN=1"
       CPPFLAGS="$PINTOOL_CPPFLAGS $CPPFLAGS"
       PINTOOL_LDFLAGS="-L$with_pin/intel64/runtime -L$with_pin/intel64/lib -L$with_pin/intel/lib-ext"
       LDFLAGS="$PINTOOL_LDFLAGS $PINTOOL_LDFLAGS $LDFLAGS -lm"],
      [PINTOOL_CPPFLAGS=
       PINTOOL_LDFLAGS=
	PINTOOL_PATH="$PATH"
	PINTOOL_DIR=])])

dnl pin.sh is present in pin 2.14, but not in 3.0+
  AC_PATH_PROG([PINTOOL2_RUNTIME], [pin.sh], [""], [$PINTOOL_PATH])
  AC_PATH_PROG([PINTOOL3_RUNTIME], [pin], [""], [$PINTOOL_PATH])
  
  AS_IF([test "x$PINTOOL2_RUNTIME" = "x" -a "x$PINTOOL3_RUNTIME" = "x"], [sst_check_pintool_happy="no"])

  CPPFLAGS="$CPPFLAGS_saved"
  LDFLAGS="$LDFLAGS_saved"
  PATH="$PATH_saved"
  
  AS_IF([test ! -z $PINTOOL2_RUNTIME], [PINTOOL_RUNTIME="$PINTOOL2_RUNTIME"], [PINTOOL_RUNTIME="$PINTOOL3_RUNTIME"])

  AS_IF([test "$sst_check_pintool_happy" = "yes"], [PINTOOL_CPPFLAGS="$PINTOOL_CPPFLAGS -DPINTOOL_EXECUTABLE=\\\"$PINTOOL_RUNTIME\\\""])

  AC_SUBST([PINTOOL_CPPFLAGS])
  AC_SUBST([PINTOOL_LDFLAGS])
  AC_SUBST([PINTOOL_RUNTIME])
  AC_SUBST([PINTOOL_DIR])
  AC_SUBST([PINTOOL_PATH])
  
  AC_DEFINE_UNQUOTED([PINTOOL_EXECUTABLE], ["$PINTOOL_RUNTIME"], [Defines the exectuable to run when we are performing integrated runs of the PIN engine])
  
  AM_CONDITIONAL([HAVE_PIN3], [test "x$PINTOOL2_RUNTIME" = "x"])
  AM_CONDITIONAL([HAVE_PINTOOL], [test "$sst_check_pintool_happy" = "yes"])
  
  AS_IF([test "x$sst_check_pintool_happy" = "xyes" -a  "x$PINTOOL2_RUNTIME" = "x"],
         [AC_DEFINE([HAVE_PINCRT], [1], [Set to 1 if Pin is using PinCRT])])

  AC_MSG_CHECKING([PIN Tool])
  AC_MSG_RESULT([$sst_check_pintool_happy])

  AS_IF([test "$sst_check_pintool_happy" = "no" -a ! -z "$with_pin" -a "$with_pin" != "no"], [$3])
  AS_IF([test "$sst_check_pintool_happy" = "yes"], [$1], [$2])
])
