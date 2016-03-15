
AC_DEFUN([SST_CHECK_NVDIMMSIM], [
  AC_ARG_WITH([nvdimmsim],
    [AS_HELP_STRING([--with-nvdimmsim@<:@=DIR@:>@],
      [Use NVDIMMSim package installed in optionally specified DIR])])

  sst_check_nvdimmsim_happy="yes"
  AS_IF([test "$with_nvdimmsim" = "no"], [sst_check_nvdimmsim_happy="no"])

  CPPFLAGS_saved="$CPPFLAGS"
  LDFLAGS_saved="$LDFLAGS"
  LIBS_saved="$LIBS"
  CXXFLAGS_saved="$CXXFLAGS"

  AS_IF([test ! -z "$with_nvdimmsim" -a "$with_nvdimmsim" != "yes"],
    [NVDIMMSIM_CPPFLAGS="-I$with_nvdimmsim -I$with_nvdimmsim/include"
     CPPFLAGS="$NVDIMMSIM_CPPFLAGS $CPPFLAGS"
     CXXFLAGS="$SST_CXX0X_FLAGS $CXXFLAGS"
     NVDIMMSIM_CXXFLAGS="$SST_CXX0X_FLAGS"
     NVDIMMSIM_LDFLAGS="-L$with_nvdimmsim -L$with_nvdimmsim/lib"
     NVDIMMSIM_LIBDIR="$with_nvdimmsim"
     LDFLAGS="$NVDIMMSIM_LDFLAGS $LDFLAGS"],
    [NVDIMMSIM_CPPFLAGS=
     NVDIMMSIM_LDFLAGS=
     NVDIMMSIM_LIBDIR=
     NVDIMMSIM_CXXFLAGS=])

  AC_LANG_PUSH(C++)
  AC_CHECK_HEADERS([NVDIMM.h], [], [sst_check_nvdimmsim_happy="no"])
  AC_CHECK_LIB([nvdsim], [main],
    [NVDIMMSIM_LIB="-lnvdsim"], [sst_check_nvdimmsim_happy="no"])
  AC_LANG_POP(C++)

  CPPFLAGS="$CPPFLAGS_saved"
  LDFLAGS="$LDFLAGS_saved"
  LIBS="$LIBS_saved"
  CXXFLAGS="$CXXFLAGS_saved"

  AC_SUBST([NVDIMMSIM_CPPFLAGS])
  AC_SUBST([NVDIMMSIM_LDFLAGS])
  AC_SUBST([NVDIMMSIM_CXXFLAGS])
  AC_SUBST([NVDIMMSIM_LIB])
  AC_SUBST([NVDIMMSIM_LIBDIR])
  AM_CONDITIONAL([HAVE_NVDIMMSIM], [test "$sst_check_nvdimmsim_happy" = "yes"])
  AS_IF([test "$sst_check_nvdimmsim_happy" = "yes"],
        [AC_DEFINE([HAVE_NVDIMMSIM], [1], [Set to 1 if NVDIMMSim was found])])
  AC_DEFINE_UNQUOTED([NVDIMMSIM_LIBDIR], ["$NVDIMMSIM_LIBDIR"], [Path to NVDIMMSim library])

  AS_IF([test "$sst_check_nvdimmsim_happy" = "yes"], [$1], [$2])
])
