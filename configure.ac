
AC_INIT([SST Elements Library], [Devel], [sst@sandia.gov])
AC_PREREQ([2.59])
AC_COPYRIGHT([Copyright Sandia National Laboratories, 2004-2016])

AC_CONFIG_AUX_DIR([config])
AC_CONFIG_MACRO_DIR([config])
AM_INIT_AUTOMAKE([1.9.6 foreign dist-bzip2 subdir-objects no-define tar-ustar])

AC_CONFIG_HEADER([src/sst_element_config.h])

m4_include([config/sst_elements_include.m4])

AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_CC_C99
AC_C_INLINE
AC_PROG_MAKE_SET

AC_PROG_CXX
AC_CACHE_SAVE

AC_PROG_CPP
AC_CANONICAL_HOST

AC_CACHE_SAVE
AC_HEADER_TIME

LT_CONFIG_LTDL_DIR([src/libltdl])
LT_INIT([shared disable-static dlopen])
LTDL_INIT([recursive])

AC_HEADER_TIME
AC_CACHE_SAVE

SST_CORE_CHECK_INSTALL()

SST_CHECK_OSX()

SST_ELEMENT_CONFIG_OUTPUT()

AC_CONFIG_FILES([
  Makefile
  src/Makefile
  src/sst/Makefile
  src/sst/elements/Makefile
])

AC_OUTPUT
