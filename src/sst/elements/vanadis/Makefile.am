# -*- Makefile -*-
#
#

AM_CPPFLAGS += \
	$(MPI_CPPFLAGS) \
	-DVANADIS_BUILD_DEBUG

compdir = $(pkglibdir)
comp_LTLIBRARIES = libvanadis.la

VANADIS_SRC_FILES = \
vanadis.h \
vfpflags.h \
vfuncunit.h \
vinsbundle.h \
vinsloader.h \
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
inst/vfpconv.h \
inst/vfpdiv.h \
inst/vfpflagsread.h \
inst/vfpflagsset.h \
inst/vfpflagssetimm.h \
inst/vfpinst.h \
inst/vfpmul.h \
inst/vfpscmp.h \
inst/vfpsignlogic.h \
inst/vfpsub.h \
inst/vgpr2fp.h \
inst/vinstall.h \
inst/vinst.h \
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
inst/vsll.h \
inst/vslli.h \
inst/vspeculate.h \
inst/vsra.h \
inst/vsrai.h \
inst/vsrl.h \
inst/vsrli.h \
inst/vstorecond.h \
inst/vstore.h \
inst/vsub.h \
inst/vsyscall.h \
inst/vtrunc.h \
inst/vxor.h \
inst/vxori.h \
lsq/vlsq.h \
lsq/vlsqseq.h \
lsq/vlsqstd.h \
lsq/vmemwriterec.h \
util/vcmpop.h \
util/vdatacopy.h \
util/vfpreghandler.h \
util/vlinesplit.h \
util/vsignx.h \
util/vtypename.h \
vbranch/vbranchbasic.h \
vbranch/vbranchunit.h \
velf/velfinfo.h \
os/vappruntimememory.h \
os/vcpuos.h \
os/velfloader.cc \
os/velfloader.h \
os/vmipscpuos.h \
os/vnodeos.cc \
os/vnodeos.h \
os/vnodeoshandler.h \
os/vosbittype.h \
os/voscallev.h \
os/voscallfunc.h \
os/vriscvcpuos.h \
os/vstartthreadreq.h \
os/callev/voscallaccessev.h \
os/callev/voscallall.h \
os/callev/voscallbrk.h \
os/callev/voscallclose.h \
os/callev/voscallexitgrp.h \
os/callev/voscallexit.h \
os/callev/voscallfstat.h \
os/callev/voscallgettime64.h \
os/callev/voscallioctl.h \
os/callev/voscallmmap.h \
os/callev/voscallmmapx.h \
os/callev/voscallopenat.h \
os/callev/voscallopen.h \
os/callev/voscallread.h \
os/callev/voscallreadlink.h \
os/callev/voscallreadv.h \
os/callev/voscallsta.h \
os/callev/voscalluname.h \
os/callev/voscallunlinkat.h \
os/callev/voscallunlink.h \
os/callev/voscallunmap.h \
os/callev/voscallwrite.h \
os/callev/voscallwritev.h \
os/memmgr/vmemmgr.h \
os/node/vnodemmaph.h \
os/node/vnodenoactionh.h \
os/node/vnodeosaccessh.h \
os/node/vnodeosbrk.h \
os/node/vnodeosfd.h \
os/node/vnodeosfstath.h \
os/node/vnodeoshstate.h \
os/node/vnodeosopenath.h \
os/node/vnodeosopenh.h \
os/node/vnodeosreadh.h \
os/node/vnodeosreadlink.h \
os/node/vnodeosreadvh.h \
os/node/vnodeosstattype.h \
os/node/vnodeosunameh.h \
os/node/vnodeosunlinkath.h \
os/node/vnodeosunlinkh.h \
os/node/vnodeoswriteh.h \
os/node/vnodeoswritevh.h \
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
	tests/small/basic-io/hello-world-cpp/mipsel/vanadis.stderr.gold \
	tests/small/basic-io/hello-world-cpp/mipsel/vanadis.stdout.gold \
	tests/small/basic-io/hello-world-cpp/riscv64/hello-world-cpp \
\
    tests/small/basic-io/printf-check/Makefile \
    tests/small/basic-io/printf-check/printf-check.c \
    tests/small/basic-io/printf-check/mipsel/printf-check \
	tests/small/basic-io/printf-check/mipsel/sst.stdout.gold \
	tests/small/basic-io/printf-check/mipsel/vanadis.stdout.gold \
    tests/small/basic-io/printf-check/mipsel/vanadis.stderr.gold \
    tests/small/basic-io/printf-check/riscv64/printf-check \
\
    tests/small/basic-io/unlinkat/Makefile \
    tests/small/basic-io/unlinkat/unlinkat.c \
    tests/small/basic-io/unlinkat/mipsel/unlinkat \
    tests/small/basic-io/unlinkat/mipsel/sst.stdout.gold \
    tests/small/basic-io/unlinkat/mipsel/vanadis.stderr.gold \
    tests/small/basic-io/unlinkat/mipsel/vanadis.stdout.gold \
    tests/small/basic-io/unlinkat/riscv64/unlinkat \
\
    tests/small/basic-io/unlink/Makefile \
    tests/small/basic-io/unlink/unlink.c \
    tests/small/basic-io/unlink/mipsel/unlink \
    tests/small/basic-io/unlink/mipsel/sst.stdout.gold \
    tests/small/basic-io/unlink/mipsel/vanadis.stderr.gold \
    tests/small/basic-io/unlink/mipsel/vanadis.stdout.gold \
    tests/small/basic-io/unlink/riscv64/unlink \
\
    tests/small/basic-io/openat/Makefile \
    tests/small/basic-io/openat/openat.c \
    tests/small/basic-io/openat/mipsel/openat \
    tests/small/basic-io/openat/mipsel/sst.stdout.gold \
    tests/small/basic-io/openat/mipsel/vanadis.stderr.gold \
    tests/small/basic-io/openat/mipsel/vanadis.stdout.gold \
    tests/small/basic-io/openat/riscv64/openat \
\
    tests/small/basic-io/read-write/Makefile \
    tests/small/basic-io/read-write/read-write.c \
    tests/small/basic-io/read-write/mipsel/read-write \
    tests/small/basic-io/read-write/riscv64/read-write \
\
	tests/small/basic-math/sqrt-double/Makefile \
	tests/small/basic-math/sqrt-double/sqrt-double.c \
	tests/small/basic-math/sqrt-double/mipsel/sqrt-double \
	tests/small/basic-math/sqrt-double/mipsel/sst.stdout.gold \
	tests/small/basic-math/sqrt-double/mipsel/vanadis.stderr.gold \
	tests/small/basic-math/sqrt-double/mipsel/vanadis.stdout.gold \
	tests/small/basic-math/sqrt-double/riscv64/sqrt-double \
\
	tests/small/basic-math/sqrt-float/Makefile \
	tests/small/basic-math/sqrt-float/sqrt-float.c \
	tests/small/basic-math/sqrt-float/mipsel/sqrt-float \
	tests/small/basic-math/sqrt-float/mipsel/sst.stdout.gold \
	tests/small/basic-math/sqrt-float/mipsel/vanadis.stderr.gold \
	tests/small/basic-math/sqrt-float/mipsel/vanadis.stdout.gold \
	tests/small/basic-math/sqrt-float/riscv64/sqrt-float \
\
	tests/small/basic-ops/test-branch/Makefile \
	tests/small/basic-ops/test-branch/test-branch.c \
	tests/small/basic-ops/test-branch/mipsel/test-branch \
	tests/small/basic-ops/test-branch/mipsel/sst.stdout.gold \
	tests/small/basic-ops/test-branch/mipsel/vanadis.stderr.gold \
	tests/small/basic-ops/test-branch/mipsel/vanadis.stdout.gold \
	tests/small/basic-ops/test-branch/riscv64/test-branch \
\
	tests/small/basic-ops/test-shift/Makefile \
	tests/small/basic-ops/test-shift/test-shift.c \
	tests/small/basic-ops/test-shift/mipsel/test-shift \
	tests/small/basic-ops/test-shift/mipsel/sst.stdout.gold \
	tests/small/basic-ops/test-shift/mipsel/vanadis.stderr.gold \
	tests/small/basic-ops/test-shift/mipsel/vanadis.stdout.gold \
	tests/small/basic-ops/test-shift/riscv64/test-shift \
\
	tests/basic_vanadis.py \
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
