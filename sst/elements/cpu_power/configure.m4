dnl -*- Autoconf -*-

AC_DEFUN([SST_cpu_power_CONFIG], [
  cpu_power_happy=1
  SST_CHECK_MCPAT([], [cpu_power_happy=0])
  SST_CHECK_HOTSPOT([], [cpu_power_happy=0])
  SST_CHECK_INTSIM([], [cpu_power_happy=0])
  SST_CHECK_ORION([], [cpu_power_happy=0])

  AS_IF([test $cpu_power_happy -eq 1], [$1], [$2])
])
