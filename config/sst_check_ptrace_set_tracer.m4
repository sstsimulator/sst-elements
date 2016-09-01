AC_DEFUN([SST_CHECK_PTRACE_SET_TRACER],
[
  sst_check_ptrace_set_tracer_happy="yes"

  CPPFLAGS_saved="$CPPFLAGS"
  LDFLAGS_saved="$LDFLAGS"

  AC_LANG_PUSH([C++])
  AC_COMPILE_IFELSE(
  	[AC_LANG_PROGRAM([[#include <sys/prctl.h>
  	#include <sys/types.h>
  	#include <unistd.h>]],
        [[
        	prctl(PR_SET_PTRACER, getppid(), 0, 0, 0);
        ]])],
        [],
        [sst_check_ptrace_set_tracer_happy="no"])
  
  AC_LANG_POP([C++])

  CPPFLAGS="$CPPFLAGS_saved"
  LDFLAGS="$LDFLAGS_saved"

  AS_IF([test "x$sst_check_ptrace_set_tracer_happy" = "xyes"],
  	[HAVE_SET_PTRACER=1], [HAVE_SET_PTRACER=0])

  AC_SUBST([HAVE_SET_PTRACER])
  AM_CONDITIONAL([HAVE_SET_PTRACER], [test "x$sst_check_ptrace_set_tracer_happy" = "xyes"])

  AC_MSG_CHECKING([for for PR_SET_PTRACER])
  AC_MSG_RESULT([$sst_check_ptrace_set_tracer_happy])
])
