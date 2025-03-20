AC_DEFUN([SST_CHECK_NUMPY], [
  sst_check_numpy_happy="yes"

  AC_ARG_WITH([numpy],
    [AS_HELP_STRING([--with-numpy<:@=DIR@:>@],
                    [NumPy must be installed])])

  # If with_numpy is blank or exactly "yes", then check for NumPy automatically.
  if test -z "$with_numpy" -o "x$with_numpy" = "xyes"; then
    SST_PYTHON=`$SST_CONFIG_TOOL --PYTHON`
    AC_MSG_CHECKING([for NumPy])
    if $SST_PYTHON -c 'import numpy' >/dev/null 2>&1; then
      NUMPY_INCLUDE_PATH=`$SST_PYTHON -c "import numpy; print(numpy.get_include())"`
      NUMPY_CPPFLAGS="-I$NUMPY_INCLUDE_PATH -DHAVE_NUMPY"
      AC_SUBST([NUMPY_CPPFLAGS])
      AC_MSG_RESULT([yes])
      sst_check_numpy_happy="yes"
    else
      AC_MSG_RESULT([no])
      sst_check_numpy_happy="no"
    fi
  else
    # User provided a specific path for --with-numpy
    NUMPY_INCLUDE_PATH="$with_numpy"
    NUMPY_CPPFLAGS="-I$NUMPY_INCLUDE_PATH -DHAVE_NUMPY"
    AC_SUBST([NUMPY_CPPFLAGS])
    AC_MSG_RESULT([yes])
    sst_check_numpy_happy="yes"
  fi

  AC_SUBST([sst_check_numpy_happy])
])
