AC_DEFUN([SST_CHECK_PINTOOL],
[
  sst_check_pintool_happy="yes"

  AC_ARG_WITH([pin],
    [AS_HELP_STRING([--with-pin@<:@=DIR@:>@],
      [Use the PIN binary instrumentation tool in DIR])])

  AS_IF([test "$with_pin" = "no"], [sst_check_pintool_happy="no"])

  CXXFLAGS_saved="$CXXFLAGS"
  CPPFLAGS_saved="$CPPFLAGS"
  LDFLAGS_saved="$LDFLAGS"
  PATH_saved="$PATH"

  AS_IF([test "$sst_check_pintool_happy" = "yes"], [
    AS_IF([test ! -z "$with_pin" -a "$with_pin" != "yes"],
      [PINTOOL_PATH="$with_pin:$PATH"
       PINTOOL_DIR="$with_pin"
       PINTOOL_CPPFLAGS="-I$with_pin/source/include -DHAVE_PIN=1"
       CPPFLAGS="$PINTOOL_CPPFLAGS $AM_CPPFLAGS $CPPFLAGS"
       CXXFLAGS="$AM_CXXFLAGS $CXXFLAGS"
       PINTOOL_LDFLAGS="-L$with_pin/intel64/runtime -L$with_pin/intel64/lib -L$with_pin/intel/lib-ext"
       LDFLAGS="$PINTOOL_LDFLAGS $PINTOOL_LDFLAGS $AM_LDFLAGS $LDFLAGS -lm"],
      [PINTOOL_CPPFLAGS=
       PINTOOL_LDFLAGS=
	PINTOOL_PATH="$PATH"
	PINTOOL_DIR=])])

dnl pin.sh is present in pin 2.14, but not in 3.0+
  AC_PATH_PROG([PINTOOL2_RUNTIME], [pin.sh], [""], [$PINTOOL_PATH])
  AC_PATH_PROG([PINTOOL3_RUNTIME], [pin], [""], [$PINTOOL_PATH])
  
  AS_IF([test "x$PINTOOL2_RUNTIME" = "x" -a "x$PINTOOL3_RUNTIME" = "x"], [sst_check_pintool_happy="no"])

  CXXFLAGS="$CXXFLAGS_saved"
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

  PIN_VERSION=$($PINTOOL_RUNTIME -version | head -1 | sed 's/.*pin-\(.*\)/\1/' | sed 's/-.*//')
  dnl AC_MSG_CHECKING([pintool version])
  dnl AC_MSG_RESULT([$PIN_VERSION])
  PIN_VERSION_MINOR=$(echo $PIN_VERSION | cut -d '.' -f 2)

dnl pin 3.24+ requires different compile line
  sst_check_pin_324_ge="yes"

  AS_VERSION_COMPARE([$PIN_VERSION], [3.24], [sst_check_pin_324_ge="no"])
  AC_MSG_CHECKING([for Pintool greater than or equal to 3.24])
  AC_MSG_RESULT([$sst_check_pin_324_ge])
  AM_CONDITIONAL([HAVE_PIN324], [test "$sst_check_pin_324_ge" = "yes"])

dnl pin 3.25+ needs libdwarf instead of lib3dwarf
  sst_check_pin_325_ge="yes"

  AS_VERSION_COMPARE([$PIN_VERSION], [3.25], [sst_check_pin_325_ge="no"])
  AC_MSG_CHECKING([for Pintool greater than or equal to 3.25])
  AC_MSG_RESULT([$sst_check_pin_325_ge])
  AM_CONDITIONAL([HAVE_PIN325], [test "$sst_check_pin_325_ge" = "yes"])

  AM_CONDITIONAL([HAVE_PIN3], [test "x$PINTOOL2_RUNTIME" = "x"])
  AM_CONDITIONAL([HAVE_PINTOOL], [test "$sst_check_pintool_happy" = "yes"])

  AS_IF([test "x$sst_check_pintool_happy" = "xyes" -a  "x$PINTOOL2_RUNTIME" = "x"],
         [AC_DEFINE([HAVE_PINCRT], [1], [Set to 1 if Pin is using PinCRT])])

  AC_MSG_CHECKING([PIN Tool])
  AC_MSG_RESULT([$sst_check_pintool_happy])

  AS_IF([test "$sst_check_pintool_happy" = "no" -a ! -z "$with_pin" -a "$with_pin" != "no"], [$3])
  AS_IF([test "$sst_check_pintool_happy" = "yes"], [$1], [$2])

  dnl Common flags
  PIN_CPPFLAGS="-g \
  -Wall \
  -Werror \
  -Wno-unknown-pragmas \
  -D__PIN__=1 -DPIN_CRT=1 \
  -fno-stack-protector \
  -fno-exceptions \
  -funwind-tables \
  -fasynchronous-unwind-tables \
  -fomit-frame-pointer \
  -fno-strict-aliasing \
  -fno-rtti \
  -faligned-new \
  -fpic \
  -DTARGET_IA32E \
  -DHOST_IA32E \
  -DTARGET_LINUX \
  -DPIN_VERSION_MINOR=$PIN_VERSION_MINOR"
  AC_SUBST([PIN_CPPFLAGS])

  AC_LANG_PUSH([C++])
  AX_COMPILER_VENDOR
  AC_LANG_POP([C++])
  AS_IF([test "$ax_cv_cxx_compiler_vendor" = "clang"],
        [PIN_CPPFLAGS_COMPILER="-D_LIBCPP_DISABLE_AVAILABILITY \
         -D_LIBCPP_NO_VCRUNTIME \
         -D__BIONIC__ \
         -Wno-non-c-typedef-for-linkage \
         -Wno-microsoft-include \
         -Wno-unicode"],
        [PIN_CPPFLAGS_COMPILER="-fno-exceptions \
         -fabi-version=2"])
  AC_SUBST([PIN_CPPFLAGS_COMPILER])
])
