
AC_DEFUN([SST_CONFIG_ELEMENTS], [
	m4_ifdef([sst_element_list], [],
		[m4_fatal([Could not find a list of elements to configure])])
		
	m4_foreach(next_element, [sst_element_list],
		SST_CONFIGURE_ELEMENT(next_element))
])

AC_DEFUN([SST_CONFIGURE_ELEMENT], [
	AC_MSG_NOTICE([Configuration for element $1])
	
	m4_ifdef([SST_]$1[_CONFIG],
		[SST_]$1[_CONFIG]([should_build=1], [should_build=0])],
		[should_build=1])
		
	AC_MSG_CHECKING([if $1 can be built])
	
	AS_IF([test "$should_build" -eq 1],
		[AC_MSG_RESULT([yes]) $2],
		[AC_MSG_RESULT([no]), $3])
		
	AC_CONFIG_FILES([src/sst/elements/$1/Makefile])
])
