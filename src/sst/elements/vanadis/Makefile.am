# -*- Makefile -*-
#
#

AM_CPPFLAGS += \
	$(MPI_CPPFLAGS) \
	-I$(top_srcdir)/src \
	-DVANADIS_BUILD_DEBUG

compdir = $(pkglibdir)
comp_LTLIBRARIES = libvanadis.la

VANADIS_SRC_FILES = \
datastruct/cqueue.h \
datastruct/vcache.h \
decoder/vauxvec.h \
decoder/vdecoder.h \
decoder/visaopts.h \
decoder/vmipsdecoder.h \
decoder/vriscv64decoder.h \
inst/fpregmode.h \
inst/isatable.h \
inst/regfile.h \
inst/regstack.h \
inst/vadd.h \
inst/vaddi.h \
inst/vaddiu.h \
inst/vand.h \
inst/vandi.h \
inst/vbcmp.h \
inst/vbcmpi.h \
inst/vbcmpil.h \
inst/vbfp.h \
inst/vcimov.h \
inst/vcmptype.h \
inst/vdecodealignfault.h \
inst/vdecodefaultinst.h \
inst/vdelaytype.h \
inst/vdiv.h \
inst/vdivmod.h \
inst/vfault.h \
inst/vfence.h \
inst/vfp2fp.h \
inst/vfp2gpr.h \
inst/vfpadd.h \
inst/vfpmadd.h \
inst/vfpmsub.h \
inst/vfpconv.h \
inst/vfpdiv.h \
inst/vfpflagsread.h \
inst/vfpflagsset.h \
inst/vfpflagssetimm.h \
inst/vfpinst.h \
inst/vfpmul.h \
inst/vfpmin.h \
inst/vfpsqrt.h \
inst/vfpscmp.h \
inst/vfpsignlogic.h \
inst/vfpsub.h \
inst/vgpr2fp.h \
inst/vinst.h \
inst/vinstall.h \
inst/vinsttype.h \
inst/vjl.h \
inst/vjlr.h \
inst/vjr.h \
inst/vjump.h \
inst/vload.h \
inst/vmemflagtype.h \
inst/vmipsfpscmp.h \
inst/vmod.h \
inst/vmovci.h \
inst/vmul.h \
inst/vmuli.h \
inst/vmulsplit.h \
inst/vmulhigh.h \
inst/vnop.h \
inst/vnor.h \
inst/vor.h \
inst/vori.h \
inst/vpartialload.h \
inst/vpartialstore.h \
inst/vpcaddi.h \
inst/vregfmt.h \
inst/vscmp.h \
inst/vscmpi.h \
inst/vsetreg.h \
inst/vsetregcallable.h \
inst/vsll.h \
inst/vslli.h \
inst/vspeculate.h \
inst/vsra.h \
inst/vsrai.h \
inst/vsrl.h \
inst/vsrli.h \
inst/vstore.h \
inst/vstorecond.h \
inst/vsub.h \
inst/vsyscall.h \
inst/vtrunc.h \
inst/vxor.h \
inst/vxori.h \
lsq/vbasiclsq.h \
lsq/vbasiclsqentry.h \
lsq/vlsq.h \
lsq/vmemwriterec.h \
util/vcmpop.h \
util/vdatacopy.h \
util/vfpreghandler.h \
util/vlinesplit.h \
util/vsignx.h \
util/vtypename.h \
vanadis.h \
vanadisDbgFlags.h \
vbranch/vbranchbasic.h \
vbranch/vbranchunit.h \
velf/velfinfo.h \
vfpflags.h \
vfuncunit.h \
vinsbundle.h \
vinsloader.h \
os/vappruntimememory.h \
os/vcpuos.h \
os/velfloader.cc \
os/velfloader.h \
os/vloadpage.h \
os/vloadpage.cc \
os/vmipscpuos.h \
os/vnodeos.cc \
os/vnodeos.h \
os/vnodeoshandler.cc \
os/vosbittype.h \
os/voscallev.h \
os/voscallfunc.h \
os/include/fdTable.h \
os/include/process.h \
os/include/processGrp.h \
os/include/virtMemMap.h \
os/vphysmemmanager.h \
os/vriscvcpuos.h \
os/vstartthreadreq.h \
os/callev/voscallaccessev.h \
os/callev/voscallall.h \
os/callev/voscallbrk.h \
os/callev/voscallclone.h \
os/callev/voscallclose.h \
os/callev/voscallexitgrp.h \
os/callev/voscallexit.h \
os/callev/voscallfstat.h \
os/callev/voscallfutex.h \
os/callev/voscallgettime64.h \
os/callev/voscallgetx.h \
os/callev/voscallioctl.h \
os/callev/voscalliovec.h \
os/callev/voscallmmap.h \
os/callev/voscallmmapx.h \
os/callev/voscallopenat.h \
os/callev/voscallopen.h \
os/callev/voscallread.h \
os/callev/voscallreadlink.h \
os/callev/voscallreadv.h \
os/callev/voscallsettidaddr.h \
os/callev/voscallsta.h \
os/callev/voscalluname.h \
os/callev/voscallunlinkat.h \
os/callev/voscallunlink.h \
os/callev/voscallunmap.h \
os/callev/voscallwrite.h \
os/callev/voscallwritev.h \
os/syscall/access.h \
os/syscall/brk.h \
os/syscall/clone.cc \
os/syscall/clone.h \
os/syscall/close.h \
os/syscall/exit.h \
os/syscall/exit.cc \
os/syscall/exitgroup.cc \
os/syscall/exitgroup.h \
os/syscall/fork.cc \
os/syscall/fork.h \
os/syscall/fstat.h \
os/syscall/futex.cc \
os/syscall/futex.h \
os/syscall/getpgid.h \
os/syscall/getpid.h \
os/syscall/getppid.h \
os/syscall/gettid.h \
os/syscall/gettime.h \
os/syscall/ioctl.h \
os/syscall/iovec.h \
os/syscall/mmap.h \
os/syscall/mprotect.cc \
os/syscall/mprotect.h \
os/syscall/openat.h \
os/syscall/open.h \
os/syscall/read.h \
os/syscall/readlink.h \
os/syscall/readv.h \
os/syscall/settidaddress.h \
os/syscall/syscall.h \
os/syscall/syscall.cc \
os/syscall/uname.h \
os/syscall/unlinkat.h \
os/syscall/unlink.h \
os/syscall/unmap.h \
os/syscall/write.h \
os/syscall/writev.h \
os/memmgr/vmemmgr.h \
os/resp/voscallresp.h \
os/resp/vosexitresp.h


EXTRA_DIST = \
	tests/small/basic-io/hello-world/Makefile \
	tests/small/basic-io/hello-world/hello-world.c \
	tests/small/basic-io/hello-world/mipsel/hello-world \
	tests/small/basic-io/hello-world/mipsel/sst.stdout.gold \
	tests/small/basic-io/hello-world/mipsel/vanadis.stderr.gold \
	tests/small/basic-io/hello-world/mipsel/vanadis.stdout.gold \
	tests/small/basic-io/hello-world/riscv64/hello-world \
	tests/small/basic-io/hello-world/riscv64/sst.stdout.gold \
	tests/small/basic-io/hello-world/riscv64/vanadis.stderr.gold \
	tests/small/basic-io/hello-world/riscv64/vanadis.stdout.gold \
\
	tests/small/basic-io/hello-world-cpp/Makefile \
	tests/small/basic-io/hello-world-cpp/hello-world-cpp.cc \
	tests/small/basic-io/hello-world-cpp/mipsel/hello-world-cpp \
	tests/small/basic-io/hello-world-cpp/mipsel/sst.stdout.gold \
	tests/small/basic-io/hello-world-cpp/mipsel/vanadis.stderr.gold \
	tests/small/basic-io/hello-world-cpp/mipsel/vanadis.stdout.gold \
	tests/small/basic-io/hello-world-cpp/riscv64/hello-world-cpp \
	tests/small/basic-io/hello-world-cpp/riscv64/sst.stdout.gold \
	tests/small/basic-io/hello-world-cpp/riscv64/vanadis.stderr.gold \
	tests/small/basic-io/hello-world-cpp/riscv64/vanadis.stdout.gold \
\
    tests/small/basic-io/printf-check/Makefile \
    tests/small/basic-io/printf-check/printf-check.c \
    tests/small/basic-io/printf-check/mipsel/printf-check \
	tests/small/basic-io/printf-check/mipsel/sst.stdout.gold \
	tests/small/basic-io/printf-check/mipsel/vanadis.stdout.gold \
    tests/small/basic-io/printf-check/mipsel/vanadis.stderr.gold \
    tests/small/basic-io/printf-check/riscv64/printf-check \
	tests/small/basic-io/printf-check/riscv64/sst.stdout.gold \
	tests/small/basic-io/printf-check/riscv64/vanadis.stdout.gold \
    tests/small/basic-io/printf-check/riscv64/vanadis.stderr.gold \
\
    tests/small/basic-io/unlinkat/Makefile \
    tests/small/basic-io/unlinkat/unlinkat.c \
    tests/small/basic-io/unlinkat/mipsel/unlinkat \
    tests/small/basic-io/unlinkat/mipsel/sst.stdout.gold \
    tests/small/basic-io/unlinkat/mipsel/vanadis.stderr.gold \
    tests/small/basic-io/unlinkat/mipsel/vanadis.stdout.gold \
    tests/small/basic-io/unlinkat/riscv64/unlinkat \
    tests/small/basic-io/unlinkat/riscv64/sst.stdout.gold \
    tests/small/basic-io/unlinkat/riscv64/vanadis.stderr.gold \
    tests/small/basic-io/unlinkat/riscv64/vanadis.stdout.gold \
\
    tests/small/basic-io/unlink/Makefile \
    tests/small/basic-io/unlink/unlink.c \
    tests/small/basic-io/unlink/mipsel/unlink \
    tests/small/basic-io/unlink/mipsel/sst.stdout.gold \
    tests/small/basic-io/unlink/mipsel/vanadis.stderr.gold \
    tests/small/basic-io/unlink/mipsel/vanadis.stdout.gold \
    tests/small/basic-io/unlink/riscv64/unlink \
    tests/small/basic-io/unlink/riscv64/sst.stdout.gold \
    tests/small/basic-io/unlink/riscv64/vanadis.stderr.gold \
    tests/small/basic-io/unlink/riscv64/vanadis.stdout.gold \
\
    tests/small/basic-io/openat/Makefile \
    tests/small/basic-io/openat/openat.c \
    tests/small/basic-io/openat/mipsel/openat \
    tests/small/basic-io/openat/mipsel/sst.stdout.gold \
    tests/small/basic-io/openat/mipsel/vanadis.stderr.gold \
    tests/small/basic-io/openat/mipsel/vanadis.stdout.gold \
    tests/small/basic-io/openat/riscv64/openat \
    tests/small/basic-io/openat/riscv64/sst.stdout.gold \
    tests/small/basic-io/openat/riscv64/vanadis.stderr.gold \
    tests/small/basic-io/openat/riscv64/vanadis.stdout.gold \
\
    tests/small/basic-io/read-write/Makefile \
    tests/small/basic-io/read-write/read-write.c \
    tests/small/basic-io/read-write/mipsel/read-write \
    tests/small/basic-io/read-write/mipsel/sst.stdout.gold \
    tests/small/basic-io/read-write/mipsel/vanadis.stderr.gold \
    tests/small/basic-io/read-write/mipsel/vanadis.stdout.gold \
    tests/small/basic-io/read-write/riscv64/read-write \
    tests/small/basic-io/read-write/riscv64/sst.stdout.gold \
    tests/small/basic-io/read-write/riscv64/vanadis.stderr.gold \
    tests/small/basic-io/read-write/riscv64/vanadis.stdout.gold \
\
	tests/small/basic-math/sqrt-double/Makefile \
	tests/small/basic-math/sqrt-double/sqrt-double.c \
	tests/small/basic-math/sqrt-double/mipsel/sqrt-double \
	tests/small/basic-math/sqrt-double/mipsel/sst.stdout.gold \
	tests/small/basic-math/sqrt-double/mipsel/vanadis.stderr.gold \
	tests/small/basic-math/sqrt-double/mipsel/vanadis.stdout.gold \
	tests/small/basic-math/sqrt-double/riscv64/sqrt-double \
	tests/small/basic-math/sqrt-double/riscv64/sst.stdout.gold \
	tests/small/basic-math/sqrt-double/riscv64/vanadis.stderr.gold \
	tests/small/basic-math/sqrt-double/riscv64/vanadis.stdout.gold \
\
	tests/small/basic-math/sqrt-float/Makefile \
	tests/small/basic-math/sqrt-float/sqrt-float.c \
	tests/small/basic-math/sqrt-float/mipsel/sqrt-float \
	tests/small/basic-math/sqrt-float/mipsel/sst.stdout.gold \
	tests/small/basic-math/sqrt-float/mipsel/vanadis.stderr.gold \
	tests/small/basic-math/sqrt-float/mipsel/vanadis.stdout.gold \
	tests/small/basic-math/sqrt-float/riscv64/sqrt-float \
	tests/small/basic-math/sqrt-float/riscv64/sst.stdout.gold \
	tests/small/basic-math/sqrt-float/riscv64/vanadis.stderr.gold \
	tests/small/basic-math/sqrt-float/riscv64/vanadis.stdout.gold \
\
	tests/small/basic-ops/test-branch/Makefile \
	tests/small/basic-ops/test-branch/test-branch.c \
	tests/small/basic-ops/test-branch/mipsel/test-branch \
	tests/small/basic-ops/test-branch/mipsel/sst.stdout.gold \
	tests/small/basic-ops/test-branch/mipsel/vanadis.stderr.gold \
	tests/small/basic-ops/test-branch/mipsel/vanadis.stdout.gold \
	tests/small/basic-ops/test-branch/riscv64/test-branch \
	tests/small/basic-ops/test-branch/riscv64/sst.stdout.gold \
	tests/small/basic-ops/test-branch/riscv64/vanadis.stderr.gold \
	tests/small/basic-ops/test-branch/riscv64/vanadis.stdout.gold \
\
	tests/small/basic-ops/test-shift/Makefile \
	tests/small/basic-ops/test-shift/test-shift.c \
	tests/small/basic-ops/test-shift/mipsel/test-shift \
	tests/small/basic-ops/test-shift/mipsel/sst.stdout.gold \
	tests/small/basic-ops/test-shift/mipsel/vanadis.stderr.gold \
	tests/small/basic-ops/test-shift/mipsel/vanadis.stdout.gold \
	tests/small/basic-ops/test-shift/riscv64/test-shift \
	tests/small/basic-ops/test-shift/riscv64/sst.stdout.gold \
	tests/small/basic-ops/test-shift/riscv64/vanadis.stderr.gold \
	tests/small/basic-ops/test-shift/riscv64/vanadis.stdout.gold \
\
    tests/small/misc/stream/Makefile \
    tests/small/misc/stream/stream.c \
    tests/small/misc/stream/mipsel/stream \
    tests/small/misc/stream/mipsel/sst.stdout.gold \
    tests/small/misc/stream/mipsel/vanadis.stderr.gold \
    tests/small/misc/stream/mipsel/vanadis.stdout.gold \
    tests/small/misc/stream/riscv64/stream \
    tests/small/misc/stream/riscv64/sst.stdout.gold \
    tests/small/misc/stream/riscv64/vanadis.stderr.gold \
    tests/small/misc/stream/riscv64/vanadis.stdout.gold \
\
	tests/small/misc/stream-fortran/Makefile \
	tests/small/misc/stream-fortran/stream.f \
	tests/small/misc/stream-fortran/mysecond.c \
	tests/small/misc/stream-fortran/riscv64/stream-fortran \
	tests/small/misc/stream-fortran/riscv64/sst.stdout.gold \
	tests/small/misc/stream-fortran/riscv64/vanadis.stderr.gold \
	tests/small/misc/stream-fortran/riscv64/vanadis.stdout.gold \
\
	tests/small/misc/gettime/Makefile \
	tests/small/misc/gettime/gettime.c \
	tests/small/misc/gettime/mipsel/gettime \
	tests/small/misc/gettime/mipsel/sst.stdout.gold \
	tests/small/misc/gettime/mipsel/vanadis.stderr.gold \
	tests/small/misc/gettime/mipsel/vanadis.stdout.gold \
	tests/small/misc/gettime/riscv64/gettime \
	tests/small/misc/gettime/riscv64/sst.stdout.gold \
	tests/small/misc/gettime/riscv64/vanadis.stderr.gold \
	tests/small/misc/gettime/riscv64/vanadis.stdout.gold \
\
	tests/small/misc/splitLoad/Makefile \
	tests/small/misc/splitLoad/splitLoad.c \
	tests/small/misc/splitLoad/mipsel/splitLoad \
	tests/small/misc/splitLoad/mipsel/sst.stdout.gold \
	tests/small/misc/splitLoad/mipsel/vanadis.stderr.gold \
	tests/small/misc/splitLoad/mipsel/vanadis.stdout.gold \
	tests/small/misc/splitLoad/riscv64/splitLoad \
	tests/small/misc/splitLoad/riscv64/sst.stdout.gold \
	tests/small/misc/splitLoad/riscv64/vanadis.stderr.gold \
	tests/small/misc/splitLoad/riscv64/vanadis.stdout.gold \
\
	tests/small/misc/mt-dgemm/Makefile \
	tests/small/misc/mt-dgemm/mt-dgemm.c \
	tests/small/misc/mt-dgemm/mipsel/mt-dgemm \
	tests/small/misc/mt-dgemm/mipsel/sst.stdout.gold \
	tests/small/misc/mt-dgemm/mipsel/vanadis.stderr.gold \
	tests/small/misc/mt-dgemm/mipsel/vanadis.stdout.gold \
	tests/small/misc/mt-dgemm/riscv64/mt-dgemm \
	tests/small/misc/mt-dgemm/riscv64/sst.stdout.gold \
	tests/small/misc/mt-dgemm/riscv64/vanadis.stderr.gold \
	tests/small/misc/mt-dgemm/riscv64/vanadis.stdout.gold \
\
	tests/small/misc/hpcg/hpcg.dat \
	tests/small/misc/hpcg/mipsel/hpcg \
	tests/small/misc/hpcg/riscv64/hpcg \
\
	tests/basic_vanadis.py \
	tests/no_rtr_vanadis.py \
	tests/testsuite_default_vanadis.py

libvanadis_la_SOURCES = \
	vanadis.cc \
	$(VANADIS_SRC_FILES)

#libvanadisdbg_la_SOURCES = \
#	vanadisdbg.cc \
#	$(VANADIS_SRC_FILES)

libvanadis_la_LDFLAGS = -module -avoid-version

#libvanadisdbg_la_LDFLAGS = -module -avoid-version

bin_PROGRAMS = sst-vanadis-tracediff

sst_vanadis_tracediff_SOURCES = tools/tracediff/tracediff.cc

#vanadisdbg.cc: vanadis.cc $(VANADIS_SRC_FILES)
#	$(CXXCPP) -DVANADIS_BUILD_DEBUG $(CXXFLAGS) $(CPPFLAGS) -I./ vanadis.cc > $@

#CLEANFILES = vanadisdbg.cc

install-exec-hook:
	$(SST_REGISTER_TOOL) SST_ELEMENT_SOURCE     vanadis=$(abs_srcdir)
	$(SST_REGISTER_TOOL) SST_ELEMENT_TESTS      vanadis=$(abs_srcdir)/tests
