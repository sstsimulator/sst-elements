# -*- Autoconf -*-
#
#

# Set Version of SST Elements Library
AC_INIT([SST Elements Library], [6.0.0], [sst@sandia.gov])

AC_PREREQ([2.59])
AC_COPYRIGHT([Copyright Sandia National Laboratories, 2004-2016])

AC_CONFIG_AUX_DIR([config])
AC_CONFIG_MACRO_DIR([config])
AM_INIT_AUTOMAKE([1.9.6 foreign dist-bzip2 subdir-objects no-define tar-ustar])

# If Automake supports silent rules, enable them.
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

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

echo ""
echo "-------------------------------------------------------"
echo "SST Elements Configuration completed."
echo "-------------------------------------------------------"
echo "Build Environment:"
echo ""
printf "%38s : %s\n" "SST-Elements Prefix" "$prefix"
printf "%38s : %s\n" "SST-Core Prefix" "$SST_PREFIX"

printf "%38s : %s\n" "Preprocessor" "$CPP"
printf "%38s : %s\n" "C++ Compiler" "$CXX"
printf "%38s : %s\n" "C Compiler" "$CC"
printf "%38s : %s\n" "MPI C Compiler" "$MPICC"
printf "%38s : %s\n" "MPI C++ Compiler" "$MPICXX"
printf "%38s : %s\n" "Linker" "$LD"
printf "%38s : %s\n" "Preprocessor Options" "$CPPFLAGS"
printf "%38s : %s\n" "C Compiler Options" "$CFLAGS"
printf "%38s : %s\n" "C++ Compiler Options" "$CXXFLAGS"

if test "x$found_cxx1y" = "xyes"; then
printf "%38s : %s\n" "C++ Standard Compliance" "C++1Y/14"
else
printf "%38s : %s\n" "C++ Standard Compliance" "C++11"
fi

printf "%38s : %s\n" "Linker Options" "$LDFLAGS"
printf "%38s : %s\n" "Boost Library Preprocessor" "$BOOST_CPPFLAGS"
printf "%38s : %s\n" "Boost Library Linker Options" "$BOOST_LDFLAGS"
echo "-------------------------------------------------------"
echo "Configuration Information (Found Dependencies):"
echo ""

prefix="sst_check_"

list=`ls ./config/sst_check*.m4`

for depfilepath in $list; do
    depfile=`basename -s .m4 $depfilepath`

    # Extract the subname from the depfile string name
    depsubname=$depfile
    depsubname=${depsubname#$prefix}
    
    happytest=${depfile}_happy
    
    if test "x${!happytest}" = "xyes" ; then
        printf "%38s : YES\n" $depsubname
    else
        printf "%38s : No\n" $depsubname
    fi
    
done

echo "-------------------------------------------------------" 
echo "Configuration Information (Make will build the following elements):"
echo ""

for e in $dist_element_libraries
do
    echo "$active_element_libraries" | grep "$e" >/dev/null
    if test $? -eq 0 ; then
        msg="YES"
    else
        msg="no"
    fi
    printf "%38s : $msg\n" "$e"
done

echo "-------------------------------------------------------"
echo ""
