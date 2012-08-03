#
# Definitions to be customized by 'configure'
#

include ../../Makefile.inc

# Make sure that output locations are set
ifeq ("$(strip $(OMNETPP_BIN_DIR))","")
$(error OMNETPP_BIN_DIR must be correctly set)
endif
ifeq ("$(strip $(OMNETPP_OUT_DIR))","")
$(error OMNETPP_OUT_DIR must be correctly set)
endif
ifeq ("$(strip $(OMNETPP_LIB_DIR))","")
$(error OMNETPP_LIB_DIR must be correctly set)
endif

#
# Local definitions
#
LIBNAME=liboppcmdenv$D
O=$(OMNETPP_OUT_DIR)/$(TOOLCHAIN_NAME)-$(MODE)/src/cmdenv
L=$(OMNETPP_LIB_DIR)/$(TOOLCHAIN_NAME)

COPTS=$(CFLAGS) -I$(OMNETPP_INCL_DIR) -I$(OMNETPP_INCL_DIR)/platdep -I../common -I../envir -DBUILDING_CMDENV

IMPLIBS= -L$L -loppsim$D -loppenvir$D -loppcommon$D

OBJS = $O/cmdenv.o

#
# Automatic rules
#
.SUFFIXES : .cc

$O/%.o: %.cc
	@mkdir -p $O
	$(CXX) -c $(COPTS) -o $@ $<

#
# Targets
#
all: $L/$(LIBNAME)$(LIB_SUFFIX)
	cd $(OMNETPP_LIB_DIR) && rm -f $(LIBNAME).* && $(LN) $(TOOLCHAIN_NAME)/$(LIBNAME).* .

$L/$(LIBNAME)$(A_LIB_SUFFIX) : $(OBJS)
	mkdir -p $L
	$(AR) $O/$(LIBNAME)$(A_LIB_SUFFIX) $(OBJS)
	$(RANLIB) $O/$(LIBNAME)$(A_LIB_SUFFIX)
	cd $O && $(OMNETPP_ROOT)/src/utils/install-lib $(LIBNAME)$(A_LIB_SUFFIX) $L

$L/$(LIBNAME)$(SO_LIB_SUFFIX) : $(OBJS)
	mkdir -p $L
	$(SHLIB_LD) $(LDFLAGS) -o $O/$(LIBNAME)$(SO_LIB_SUFFIX) $(OBJS) $(IMPLIBS)
	$(SHLIB_POSTPROCESS) $O/$(LIBNAME)$(SO_LIB_SUFFIX)
	cd $O && $(OMNETPP_ROOT)/src/utils/install-lib $(LIBNAME)$(SO_LIB_SUFFIX) $L
	rm -f $(OMNETPP_LIB_DIR)/$(LIBNAME)$(SO_LIB_SUFFIX) && $(LN) $L/$(LIBNAME)$(SO_LIB_SUFFIX) $(OMNETPP_LIB_DIR)
	$(SHLIB_POSTPROCESS) $(OMNETPP_LIB_DIR)/$(LIBNAME)$(SO_LIB_SUFFIX)

$L/$(LIBNAME)$(DLL_LIB_SUFFIX) : $(OBJS)
	mkdir -p $L
	$(SHLIB_LD) $(LDFLAGS) -o $O/$(LIBNAME)$(DLL_LIB_SUFFIX) $(OBJS) $(IMPLIBS) -Wl,--out-implib,$O/$(LIBNAME)$(A_LIB_SUFFIX)
	cd $O && $(OMNETPP_ROOT)/src/utils/install-lib $(LIBNAME)$(A_LIB_SUFFIX) $L
	cp $O/$(LIBNAME)$(DLL_LIB_SUFFIX) $(OMNETPP_BIN_DIR)

depend:
	$(MAKEDEPEND) -I. -I../envir -I../../include -P\$$O/ -- *.cc

clean:
	rm -rf $O
	rm -f $L/$(LIBNAME).* $(OMNETPP_LIB_DIR)/$(LIBNAME).*

# DO NOT DELETE THIS LINE -- make depend depends on it.
$O/cmdenv.o: cmdenv.cc \
  /home/gouri/devel/omnetpp-4.1/include/regmacros.h \
  ../envir/args.h \
  cmdenv.h \
  /home/gouri/devel/omnetpp-4.1/include/platdep/platdefs.h \
  ../envir/envirdefs.h \
  /home/gouri/devel/omnetpp-4.1/include/cmathfunction.h \
  ../envir/resultlistener.h \
  /home/gouri/devel/omnetpp-4.1/include/cnamedobject.h \
  /home/gouri/devel/omnetpp-4.1/include/cexpression.h \
  ../envir/eventlogfilemgr.h \
  cmddefs.h \
  /home/gouri/devel/omnetpp-4.1/include/cgate.h \
  /home/gouri/devel/omnetpp-4.1/include/simtime.h \
  ../envir/objectprinter.h \
  /home/gouri/devel/omnetpp-4.1/include/carray.h \
  ../envir/speedometer.h \
  /home/gouri/devel/omnetpp-4.1/include/simtime_t.h \
  /home/gouri/devel/omnetpp-4.1/include/cenvir.h \
  /home/gouri/devel/omnetpp-4.1/include/cmsgpar.h \
  /home/gouri/devel/omnetpp-4.1/include/ccomponent.h \
  /home/gouri/devel/omnetpp-4.1/include/cconfigoption.h \
  /home/gouri/devel/omnetpp-4.1/include/cproperties.h \
  /home/gouri/devel/omnetpp-4.1/include/cregistrationlist.h \
  /home/gouri/devel/omnetpp-4.1/include/cconfiguration.h \
  /home/gouri/devel/omnetpp-4.1/include/opp_string.h \
  ../envir/intervals.h \
  /home/gouri/devel/omnetpp-4.1/include/simkerneldefs.h \
  /home/gouri/devel/omnetpp-4.1/include/csimulation.h \
  /home/gouri/devel/omnetpp-4.1/include/errmsg.h \
  /home/gouri/devel/omnetpp-4.1/include/cmessage.h \
  /home/gouri/devel/omnetpp-4.1/include/cownedobject.h \
  /home/gouri/devel/omnetpp-4.1/include/onstartup.h \
  /home/gouri/devel/omnetpp-4.1/include/cobject.h \
  /home/gouri/devel/omnetpp-4.1/src/envir/envirbase.h \
  /home/gouri/devel/omnetpp-4.1/include/envirext.h \
  /home/gouri/devel/omnetpp-4.1/include/clistener.h \
  /home/gouri/devel/omnetpp-4.1/include/cstringpool.h \
  /home/gouri/devel/omnetpp-4.1/include/cpar.h \
  /home/gouri/devel/omnetpp-4.1/include/globals.h \
  ../envir/appreg.h \
  /home/gouri/devel/omnetpp-4.1/include/cdefaultlist.h \
  /home/gouri/devel/omnetpp-4.1/include/cvisitor.h \
  /home/gouri/devel/omnetpp-4.1/include/csimplemodule.h \
  /home/gouri/devel/omnetpp-4.1/include/cobjectfactory.h \
  /home/gouri/devel/omnetpp-4.1/include/cmodule.h \
  /home/gouri/devel/omnetpp-4.1//include/platdep/platmisc.h \
  /home/gouri/devel/omnetpp-4.1/include/cexception.h \
  /home/gouri/devel/omnetpp-4.1/include/cmessageheap.h \
  /home/gouri/devel/omnetpp-4.1/include/simutil.h \
  /home/gouri/devel/omnetpp-4.1/include/cproperty.h \
  /home/gouri/devel/omnetpp-4.1/include/platdep/intxtypes.h \
  /home/gouri/devel/omnetpp-4.1/include/ccomponenttype.h
