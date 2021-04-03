# -*- Makefile -*-
#
#

AM_CPPFLAGS = \
	$(MPI_CPPFLAGS) \
	-DVANADIS_BUILD_DEBUG

compdir = $(pkglibdir)
comp_LTLIBRARIES = libvanadis.la

VANADIS_SRC_FILES = \
os/vnodeos.cc \
vanadis.h \
vfuncunit.h \
vinsbundle.h \
vinsloader.h \
datastruct/cqueue.h \
datastruct/vcache.h \
decoder/vauxvec.h \
decoder/vdecoder.h \
decoder/visaopts.h \
decoder/vmipsdecoder.h \
decoder/vmipsinsloader.h \
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
inst/vdecodefaultinst.h \
inst/vdelaytype.h \
inst/vdivmod.h \
inst/vfence.h \
inst/vfp2fp.h \
inst/vfp2gpr.h \
inst/vfpadd.h \
inst/vfpconv.h \
inst/vfpdiv.h \
inst/vfpmul.h \
inst/vfpscmp.h \
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
inst/vmul.h \
inst/vmuli.h \
inst/vmulsplit.h \
inst/vnop.h \
inst/vnor.h \
inst/vor.h \
inst/vori.h \
inst/vpartialload.h \
inst/vpartialstore.h \
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
os/vcpuos.h \
os/vmipscpuos.h \
os/vnodeos.h \
os/vnodeoshandler.h \
os/voscallev.h \
os/voscallfunc.h \
os/voscallresp.h \
util/vcmpop.h \
util/vdatacopy.h \
util/vfpreghandler.h \
util/vlinesplit.h \
util/vsignx.h \
vbranch/vbranchbasic.h \
vbranch/vbranchunit.h \
velf/velfinfo.h \
os/callev/voscallaccessev.h \
os/callev/voscallall.h \
os/callev/voscallbrk.h \
os/callev/voscallclose.h \
os/callev/voscallexitgrp.h \
os/callev/voscallfstat.h \
os/callev/voscallgettime64.h \
os/callev/voscallioctl.h \
os/callev/voscallmmap.h \
os/callev/voscallopenat.h \
os/callev/voscallopen.h \
os/callev/voscallread.h \
os/callev/voscallreadlink.h \
os/callev/voscallsta.h \
os/callev/voscalluname.h \
os/callev/voscallunmap.h \
os/callev/voscallwrite.h \
os/callev/voscallwritev.h \
os/callev/vosinitbrk.h \
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
os/node/vnodeosreadlink.h \
os/node/vnodeosstattype.h \
os/node/vnodeosunameh.h \
os/node/vnodeoswriteh.h \
os/node/vnodeoswritevh.h \
os/resp/voscallresp.h \
os/resp/vosexitresp.h

EXTRA_DIST = \
	tests/small/basic-io/Makefile \
	tests/small/basic-io/hello-world.c \
	tests/small/basic-io/hello-world.stderr.gold \
	tests/small/basic-io/hello-world.stdout.gold \
	tests/small/basic-math/Makefile \
	tests/small/basic-math/sqrt-double.c \
	tests/small/basic-math/sqrt-double.stderr.gold \
	tests/small/basic-math/sqrt-double.stdout.gold \
	tests/small/basic-math/sqrt-float.c \
	tests/small/basic-math/sqrt-float.stderr.gold \
	tests/small/basic-math/sqrt-float.stdout.gold \
	tests/small/basic-ops/Makefile \
	tests/small/basic-ops/test-branch.c \
	tests/small/basic-ops/test-branch.stderr.gold \
	tests/small/basic-ops/test-branch.stdout.gold \
	tests/small/basic-ops/test-shift.c \
	tests/small/basic-ops/test-shift.stderr.gold \
	tests/small/basic-ops/test-shift.stdout.gold \
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
