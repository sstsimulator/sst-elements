AC_DEFUN([SST_CHECK_LLVM_CONFIG],
[
   sst_check_llvm_happy="yes"

   AC_ARG_WITH([llvm],
               [AS_HELP_STRING([--with-llvm<:@=DIR@:>@],
                  [llvm-config must be in the PATH])],
   )

   LLVM_CFG_PATH="NOTFOUND"

   AS_IF([test -n "$with_llvm"],
         [AC_PATH_PROG([LLVM_CFG_PATH], [llvm-config], ["NOTFOUND"], [$with_llvm$PATH_SEPARATOR$with_llvm/bin])],
         [sst_check_llvm_happy="no"])

   AS_IF([test $LLVM_CFG_PATH = "NOTFOUND"], [sst_check_llvm_happy="no"])

   AS_IF([test "x$sst_check_llvm_happy" = "xyes"],
         [AC_MSG_CHECKING(for llvm-config)
            llvm_test=$($LLVM_CFG_PATH --version 2> /dev/null || echo no)
            AS_IF([test x"$llvm_test" != "xno"],
                  [AC_MSG_RESULT([yes])
                  LLVM_CFLAGS="`$LLVM_CFG_PATH --cflags` $CFLAGS"
                  LLVM_CPPFLAGS="`$LLVM_CFG_PATH --cxxflags` $CPPFLAGS"
                  LLVM_LDFLAGS="`$LLVM_CFG_PATH --ldflags --libs` $LDFLAGS"],
                  [sst_check_llvm_happy="no"
                  AC_MSG_RESULT([no])
                  LLVM_CFLAGS=
                  LLVM_CPPFLAGS=
                  LLVM_LDFLAGS=]
               )]
         )

   LLVM_LDFLAGS="`echo ${LLVM_LDFLAGS} | sed 's/\n//g'`"
   LLVM_LDFLAGS="`echo ${LLVM_LDFLAGS} | sed 's/-lgtest_main -lgtest//g'`"

   AC_SUBST(LLVM_CFLAGS)
   AC_SUBST(LLVM_CPPFLAGS)
   AC_SUBST(LLVM_LDFLAGS)

   AC_MSG_NOTICE([Value of sst_check_llvm_happy = $sst_check_llvm_happy])

   AS_IF([test "x$sst_check_llvm_happy" = "xyes"], [$1], [$2])
])

