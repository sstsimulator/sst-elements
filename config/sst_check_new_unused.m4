dnl AC_DEFUN([SST_CHECK_NEW_UNUSED], [
dnl   AC_LANG_PUSH(C++)
dnl   AC_COMPILE_IFELSE([AC_LANG_SOURCE([[
dnl #include <sst_config.h>
dnl #if !SST_NEW_UNUSED
dnl #error
dnl #endif
dnl ]])], [new_unused=yes], [new_unused=no])
dnl   AC_LANG_POP(C++)
dnl ])
