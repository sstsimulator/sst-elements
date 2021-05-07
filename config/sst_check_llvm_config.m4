AC_DEFUN([SST_CHECK_LLVM_CONFIG],
[
   sst_check_llvm_config_happy="yes"

   AC_ARG_WITH([llvm-config],
               [AS_HELP_STRING([--with-llvm-config],
                  [llvm-config must be in the PATH])],
   )

   AS_IF([test ! -z "$with_llvm_config" -a "$with_llvm_config" = "yes"],
         [],
         [sst_check_llvm_config_happy="no"]
         )

   AC_MSG_CHECKING(for llvm-config)
   llvm_config_test=$(llvm-config --version 2> /dev/null || echo no)
   if test ! -z "$llvm_config_test" -a "$llvm_config_test" != "no" -a "$with_llvm_config" = "yes"; then
      AC_MSG_RESULT([yes])
      LLVM_CFLAGS="`llvm-config --cflags`"
      LLVM_CPPFLAGS="`llvm-config --cxxflags`"
      LLVM_LDFLAGS="`llvm-config --ldflags --libs`"
   else
      sst_check_llvm_config_happy="no"
      AC_MSG_RESULT([no])
   fi

   LLVM_LDFLAGS="`echo ${LLVM_LDFLAGS} | sed 's/\n//g'`"
   LLVM_LDFLAGS="`echo ${LLVM_LDFLAGS} | sed 's/-lgtest_main -lgtest//g'`"

   AC_SUBST(LLVM_CFLAGS)
   AC_SUBST(LLVM_CPPFLAGS)
   AC_SUBST(LLVM_LDFLAGS)

   AS_IF([test "$sst_check_llvm_config_happy" = "yes"], [$1], [$2])
])

