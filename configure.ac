# -*- Autoconf -*-
#
#

# Set Version of SST Elements Library
AC_INIT([SST Elements Library], [-dev], [sst@sandia.gov])

AC_PREREQ([2.59])
AC_COPYRIGHT([Copyright Sandia National Laboratories, 2004-2018])

AC_CONFIG_AUX_DIR([config])
AC_CONFIG_MACRO_DIR([config])
AM_INIT_AUTOMAKE([1.9.6 foreign dist-bzip2 subdir-objects no-define tar-pax])

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

# Compile flags come from SST-Core, add or remove extra warnings 
# depending on the use-picky flag 
SST_CHECK_PICKY
AS_IF([test "x$use_picky" = "xyes"],
      [WARNFLAGS="-Wall -Wextra"],
      [WARNFLAGS=""])
CFLAGS="$CFLAGS $WARNFLAGS"
CXXFLAGS="$CXXFLAGS $WARNFLAGS"
      
AC_MSG_CHECKING([for SST-Elements Git Branch, Head SHA and Commit Count])
if test -d ".git" ; then
    SSTELEMENTS_GIT_BRANCH=`git rev-parse --abbrev-ref HEAD`
    AC_MSG_RESULT([${SSTELEMENTS_GIT_BRANCH}])
    SSTELEMENTS_GIT_HEADSHA=`git rev-parse HEAD`
    AC_MSG_RESULT([${SSTELEMENTS_GIT_HEADSHA}])
	SSTELEMENTS_GIT_COMMITCOUNT=`git rev-list HEAD | wc -l | sed -e 's/^ *//g'`
	AC_MSG_RESULT([${SSTELEMENTS_GIT_COMMITCOUNT}])
else
    SSTELEMENTS_GIT_BRANCH="N/A"
    AC_MSG_RESULT([${SSTELEMENTS_GIT_BRANCH}])
    SSTELEMENTS_GIT_HEADSHA="${PACKAGE_VERSION}"
    AC_MSG_RESULT([${SSTELEMENTS_GIT_HEADSHA}])
	SSTELEMENTS_GIT_COMMITCOUNT="0"
	AC_MSG_RESULT([${SSTELEMENTS_GIT_COMMITCOUNT}])
fi

AC_SUBST(SSTELEMENTS_GIT_BRANCH)
AC_DEFINE_UNQUOTED([SSTELEMENTS_GIT_BRANCH], ["$SSTELEMENTS_GIT_BRANCH"], [SST-Elements Git Branch])

AC_SUBST(SSTELEMENTS_GIT_HEADSHA)
AC_DEFINE_UNQUOTED([SSTELEMENTS_GIT_HEADSHA], ["$SSTELEMENTS_GIT_HEADSHA"], [SST-Elements Git Head SHA])

AC_SUBST(SSTELEMENTS_GIT_COMMITCOUNT)
AC_DEFINE_UNQUOTED([SSTELEMENTS_GIT_COMMITCOUNT], ["$SSTELEMENTS_GIT_COMMITCOUNT"], [SST-Elements Git Commit Count])

CC_VERSION=`$CC --version | sed -e ':a' -e 'N' -e '$!ba' -e 's/\n/%/g' | awk -F'%' '{print $1}'`
AC_SUBST(CC_VERSION)

SST_CHECK_PTRACE_SET_TRACER()

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
printf "%38s : %s\n" "SST-Elements Version" "$PACKAGE_VERSION"
if test "x$SSTELEMENTS_GIT_HEADSHA" != "x$PACKAGE_VERSION"; then
printf "%38s : %s\n" "Git Branch" "$SSTELEMENTS_GIT_BRANCH"
printf "%38s : %s\n" "Git HEAD SHA" "$SSTELEMENTS_GIT_HEADSHA"
printf "%38s : %s\n" "Branch Commit Count" "$SSTELEMENTS_GIT_COMMITCOUNT"
else 
printf "%38s : %s\n" "Version" "SST-Elements $PACKAGE_VERSION"
fi
printf "%38s : %s\n" "SST-Elements Prefix" "$prefix"
printf "%38s : %s\n" "SST-Core Prefix" "$SST_PREFIX"

printf "%38s : %s\n" "Preprocessor" "$CPP"
printf "%38s : %s\n" "C++ Compiler" "$CXX"
printf "%38s : %s\n" "C Compiler" "$CC"
printf "%38s : %s\n" "Compiler Version" "$CC_VERSION"
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
echo ""
echo "-------------------------------------------------------"
echo "Configuration Information (Found Dependencies):"
echo ""

prefix="sst_check_"
suffix=".m4"

list=`ls $srcdir/config/sst_check*.m4`

for depfilepath in $list; do

    # Get the basename of the file
    depfilename=`basename $depfilepath`
   
    # Strip off the .m4 from the filename 
    depfile=${depfilename%$suffix}

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

echo ""
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

echo ""
echo "-------------------------------------------------------"
echo ""
