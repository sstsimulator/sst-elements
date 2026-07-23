dnl -*- Autoconf -*-
dnl vim:ft=config
dnl

AC_DEFUN([SST_carcosa_CONFIG], [
	carcosa_happy="yes"

	dnl Optional balar ring bridge (components/balarRingBridge.{cc,h}).
	dnl The bridge #includes balar's CUDA-call packet HEADERS (header-only: packet
	dnl structs + encode/decode templates), so it is compiled into libcarcosa only
	dnl when GPGPU-Sim (hence balar / CUDA) is available -- mirroring balar's own
	dnl SST_CHECK_GPGPUSIM guard. It needs no GPGPU-Sim LIBS (it only exchanges
	dnl StandardMem packets with balar), just the CUDA include path (CUDA_CPPFLAGS,
	dnl AC_SUBST'd globally by balar's SST_CHECK_CUDA). When GPGPU-Sim is absent the
	dnl bridge compiles out and libcarcosa is unchanged -- the standalone invariant.
	dnl NOTE: do NOT call SST_CHECK_CUDA here; it defines AM_CONDITIONAL([USE_CUDA])
	dnl which balar already defines -- calling it twice is an automake error.
	SST_CHECK_GPGPUSIM([carcosa_have_balar=1],[carcosa_have_balar=0],[carcosa_have_balar=0])
	AS_IF([test "x$carcosa_have_balar" = "x1"],
		[AC_DEFINE([HAVE_BALAR_BRIDGE], [1],
			[Define if balar/GPGPU-Sim headers are available for the carcosa ring bridge])])
	AM_CONDITIONAL([SST_CARCOSA_HAVE_BALAR], [test "x$carcosa_have_balar" = "x1"])

	AS_IF([test "$carcosa_happy" = "yes"], [$1], [$2])
])
