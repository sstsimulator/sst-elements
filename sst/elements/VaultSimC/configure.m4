
AC_DEFUN([SST_VaultSimC_CONFIG], [
  vaultsim_happy="yes"

  # Use global PHX check
  SST_CHECK_LIBPHX([vaultsim_happy="yes"],[vaultsim_happy="no"],[AC_MSG_ERROR([PHX requested but could not be found])])

  AS_IF([test "$vaultsim_happy" = "yes"], [$1], [$2])
])
