AC_DEFUN([SST_routermodel_CONFIG], [
  AC_ARG_WITH([McPAT],
    [AS_HELP_STRING([--with-McPAT@<:@=DIR@:>@],
      [Use McPAT package installed in optionally specified DIR])])

  AS_IF([test "$with_McPAT" = "no"], [happy="no"])

  CPPFLAGS_saved="$CPPFLAGS"
  LDFLAGS_saved="$LDFLAGS"

  AS_IF([test ! -z "$with_McPAT" -a "$with_McPAT" != "yes"],
    [MCPAT_CPPFLAGS="-I$with_McPAT"
     CPPFLAGS="$MCPAT_CPPFLAGS $CPPFLAGS"
     MCPAT_LDFLAGS="-L$with_McPAT"
     LDFLAGS="$MCPAT_LDFLAGS $LDFLAGS"],
    [MCPAT_CPPFLAGS=
     MCPAT_LDFLAGS=])

  AC_LANG_PUSH(C++)  
  AC_CHECK_LIB([McPAT], [main], 
    [MCPAT_LIB="-lMcPAT"], [happy="no"])
  AC_LANG_POP(C++)

  CPPFLAGS="$CPPFLAGS_saved"
  LDFLAGS="$LDFLAGS_saved"

  AC_SUBST([MCPAT_CPPFLAGS])
  AC_SUBST([MCPAT_LDFLAGS])
  AC_SUBST([MCPAT_LIB])

  AS_IF([test "$happy" = "yes"], [$1], [$2])




  AC_ARG_WITH([hotspot],
    [AS_HELP_STRING([--with-hotspot@<:@=DIR@:>@],
      [Use hotspot package installed in optionally specified DIR])])

  AS_IF([test "$with_hotspot" = "no"], [happy="no"])

  CPPFLAGS_saved="$CPPFLAGS"
  LDFLAGS_saved="$LDFLAGS"

  AS_IF([test ! -z "$with_hotspot" -a "$with_hotspot" != "yes"],
    [HOTSPOT_CPPFLAGS="-I$with_hotspot"
     CPPFLAGS="$HOTSPOT_CPPFLAGS $CPPFLAGS"
     HOTSPOT_LDFLAGS="-L$with_hotspot"
     LDFLAGS="$HOTSPOT_LDFLAGS $LDFLAGS"],
    [HOTSPOT_CPPFLAGS=
     HOTSPOT_LDFLAGS=])

  AC_LANG_PUSH(C++)  
  AC_CHECK_LIB([hotspot], [main], 
    [HOTSPOT_LIB="-lhotspot"], [happy="no"])
  AC_LANG_POP(C++)

  CPPFLAGS="$CPPFLAGS_saved"
  LDFLAGS="$LDFLAGS_saved"

  AC_SUBST([HOTSPOT_CPPFLAGS])
  AC_SUBST([HOTSPOT_LDFLAGS])
  AC_SUBST([HOTSPOT_LIB])

  AS_IF([test "$happy" = "yes"], [$1], [$2])






  AC_ARG_WITH([orion],
    [AS_HELP_STRING([--with-orion@<:@=DIR@:>@],
      [Use orion package installed in optionally specified DIR])])

  AS_IF([test "$with_orion" = "no"], [happy="no"])

  CPPFLAGS_saved="$CPPFLAGS"
  LDFLAGS_saved="$LDFLAGS"

  AS_IF([test ! -z "$with_orion" -a "$with_orion" != "yes"],
    [ORION_CPPFLAGS="-I$with_orion"
     CPPFLAGS="$ORION_CPPFLAGS $CPPFLAGS"
     ORION_LDFLAGS="-L$with_orion"
     LDFLAGS="$ORION_LDFLAGS $LDFLAGS"],
    [ORION_CPPFLAGS=
     ORION_LDFLAGS=])

  AC_LANG_PUSH(C++)  
  AC_CHECK_LIB([orion], [main], 
    [ORION_LIB="-lorion"], [happy="no"])
  AC_LANG_POP(C++)

  CPPFLAGS="$CPPFLAGS_saved"
  LDFLAGS="$LDFLAGS_saved"

  AC_SUBST([ORION_CPPFLAGS])
  AC_SUBST([ORION_LDFLAGS])
  AC_SUBST([ORION_LIB])

  AS_IF([test "$happy" = "yes"], [$1], [$2])


])
