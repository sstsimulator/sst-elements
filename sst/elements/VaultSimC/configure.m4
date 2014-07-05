
AC_DEFUN([SST_VaultSimC_CONFIG], [
  vaultsim_happy="yes"

  libphx_happy="yes"

  # Use global PHX check
  SST_CHECK_LIBPHX([libphx_happy="yes"],[libphx_happy="no"],[AC_MSG_ERROR([PHX library could not be found])])

  AS_IF([test "$libphx_happy" = "yes"],[AC_MSG_NOTICE([Building with PHX Library])],AC_MSG_NOTICE([No usable PHX Library building with internal timings]))

  AS_IF([test "$vaultsim_happy" = "yes"], [$1], [$2])
])
