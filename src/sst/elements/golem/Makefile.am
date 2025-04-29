# -*- Makefile -*-
#
#

if COMPILE_CLANG
	AM_CPPFLAGS += -ferror-limit=1
else
	AM_CPPFLAGS += -fmax-errors=1
endif

AM_CPPFLAGS += -I$(top_srcdir)/src \
	$(MPI_CPPFLAGS) \
	$(NUMPY_CPPFLAGS)

compdir = $(pkglibdir)
comp_LTLIBRARIES = libgolem.la
libgolem_la_SOURCES = \
	golem.cc \
	array/computeArray.h \
	array/mvmComputeArray.h \
	array/mvmFloatArray.h \
	array/mvmIntArray.h \
	rocc/roccAnalog.h \
	rocc/roccAnalogFloat.h \
	rocc/roccAnalogInt.h

EXTRA_DIST = \
	tests/small/mvm_float_array/multi_array/Makefile \
	tests/small/mvm_float_array/multi_array/multi_array.cpp \
	tests/small/mvm_float_array/multi_array/riscv64/multi_array \
	tests/small/mvm_float_array/multi_array/riscv64/sst.stdout.gold \
	tests/small/mvm_float_array/multi_array/riscv64/golem.stderr.gold \
	tests/small/mvm_float_array/multi_array/riscv64/golem.stdout.gold \
\
	tests/small/mvm_float_array/single_array/Makefile \
	tests/small/mvm_float_array/single_array/single_array.cpp \
	tests/small/mvm_float_array/single_array/riscv64/single_array \
	tests/small/mvm_float_array/single_array/riscv64/sst.stdout.gold \
	tests/small/mvm_float_array/single_array/riscv64/golem.stderr.gold \
	tests/small/mvm_float_array/single_array/riscv64/golem.stdout.gold \
\
	tests/small/mvm_int_array/multi_array/Makefile \
	tests/small/mvm_int_array/multi_array/multi_array.cpp \
	tests/small/mvm_int_array/multi_array/riscv64/multi_array \
	tests/small/mvm_int_array/multi_array/riscv64/sst.stdout.gold \
	tests/small/mvm_int_array/multi_array/riscv64/golem.stderr.gold \
	tests/small/mvm_int_array/multi_array/riscv64/golem.stdout.gold \
\
	tests/small/mvm_int_array/single_array/Makefile \
	tests/small/mvm_int_array/single_array/single_array.cpp \
	tests/small/mvm_int_array/single_array/riscv64/single_array \
	tests/small/mvm_int_array/single_array/riscv64/sst.stdout.gold \
	tests/small/mvm_int_array/single_array/riscv64/golem.stderr.gold \
	tests/small/mvm_int_array/single_array/riscv64/golem.stdout.gold \
\
	tests/basic_golem.py \
	tests/testsuite_default_golem.py


if HAVE_NUMPY
libgolem_la_SOURCES += \
	array/crossSimComputeArray.h \
	array/crossSimFloatArray.h \
	array/crossSimIntArray.h

EXTRA_DIST += \
	tests/small/crosssim_float_array/multi_array/Makefile \
	tests/small/crosssim_float_array/multi_array/multi_array.cpp \
	tests/small/crosssim_float_array/multi_array/riscv64/multi_array \
	tests/small/crosssim_float_array/multi_array/riscv64/sst.stdout.gold \
	tests/small/crosssim_float_array/multi_array/riscv64/golem.stderr.gold \
	tests/small/crosssim_float_array/multi_array/riscv64/golem.stdout.gold \
\
	tests/small/crosssim_float_array/single_array/Makefile \
	tests/small/crosssim_float_array/single_array/single_array.cpp \
	tests/small/crosssim_float_array/single_array/riscv64/single_array \
	tests/small/crosssim_float_array/single_array/riscv64/sst.stdout.gold \
	tests/small/crosssim_float_array/single_array/riscv64/golem.stderr.gold \
	tests/small/crosssim_float_array/single_array/riscv64/golem.stdout.gold \
\
	tests/small/crosssim_int_array/multi_array/Makefile \
	tests/small/crosssim_int_array/multi_array/multi_array.cpp \
	tests/small/crosssim_int_array/multi_array/riscv64/multi_array \
	tests/small/crosssim_int_array/multi_array/riscv64/sst.stdout.gold \
	tests/small/crosssim_int_array/multi_array/riscv64/golem.stderr.gold \
	tests/small/crosssim_int_array/multi_array/riscv64/golem.stdout.gold \
\
	tests/small/crosssim_int_array/single_array/Makefile \
	tests/small/crosssim_int_array/single_array/single_array.cpp \
	tests/small/crosssim_int_array/single_array/riscv64/single_array \
	tests/small/crosssim_int_array/single_array/riscv64/sst.stdout.gold \
	tests/small/crosssim_int_array/single_array/riscv64/golem.stderr.gold \
	tests/small/crosssim_int_array/single_array/riscv64/golem.stdout.gold
endif


libgolem_la_LDFLAGS = -module -avoid-version

install-exec-hook:
	$(SST_REGISTER_TOOL) SST_ELEMENT_SOURCE     golem=$(abs_srcdir)
	$(SST_REGISTER_TOOL) SST_ELEMENT_TESTS      golem=$(abs_srcdir)/tests

##########################################################################
##########################################################################
##########################################################################
