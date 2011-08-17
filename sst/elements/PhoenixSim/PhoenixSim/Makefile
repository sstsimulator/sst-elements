#
# OMNeT++/OMNEST Makefile for PhoenixSim
#
# This file was generated with the command:
#  opp_makemake -f --deep -O out
#

# Name of target to be created (-o option)
TARGET = PhoenixSim$(EXE_SUFFIX)

# User interface (uncomment one) (-u option)
USERIF_LIBS = $(ALL_ENV_LIBS) # that is, $(TKENV_LIBS) $(CMDENV_LIBS)
#USERIF_LIBS = $(CMDENV_LIBS)
#USERIF_LIBS = $(TKENV_LIBS)

# C++ include paths (with -I)
INCLUDE_PATH = \
    -I. \
    -Iaddon \
    -Iaddon/HotSpot-5.0 \
    -IchipComponents \
    -Idata \
    -IelectronicComponents \
    -IelectronicComponents/Arbiters \
    -IelectronicComponents/orion \
    -IelectronicComponents/orion/ORION_1_Params \
    -IelectronicComponents/orion/ORION_2_Params \
    -Iimages \
    -Iimages/components \
    -Iimages/components/old \
    -Iimages/mitucb \
    -Iimages/planes \
    -IioComponents \
    -IioComponents/DRAM_LRL \
    -IioComponents/DRAMsim \
    -IioComponents/DRAMsim/mem_system_def \
    -Iparameters \
    -Iparameters/Hendry-Thesis \
    -Iparameters/default \
    -Iphotonic \
    -Iphotonic/PhotoCAD \
    -Iphotonic/devices \
    -Iphotonic/devices/machzehnder \
    -Iphotonic/devices/multiring \
    -Iphotonic/switches \
    -Iphotonic/switches/machzehnder \
    -Iphotonic/switches/multiring \
    -IprocessingPlane \
    -IprocessingPlane/DRAM_Cfg \
    -IprocessingPlane/addressTranslation \
    -IprocessingPlane/apps \
    -IprocessingPlane/apps/DRAM \
    -IprocessingPlane/apps/MITUCB \
    -IprocessingPlane/apps/SizeDistribution \
    -IprocessingPlane/apps/model \
    -IprocessingPlane/apps/synthetics \
    -IprocessingPlane/apps/test \
    -IprocessingPlane/apps/trace \
    -IprocessingPlane/interfaces \
    -Iresults \
    -IsimCore \
    -Istatistics \
    -Istatistics/statGroups \
    -Istatistics/statObjects \
    -Istatistics/statObjects/energy \
    -Itopologies \
    -Itopologies/CMesh \
    -Itopologies/blockingTorus \
    -Itopologies/electronicMesh \
    -Itopologies/electronicMesh_Circuit \
    -Itopologies/electronicTorus \
    -Itopologies/flattenedButterfly \
    -Itopologies/nonblockingSiliconNitride \
    -Itopologies/nonblockingTorus \
    -Itopologies/photonicMesh \
    -Itopologies/photonicTDM \
    -Itopologies/photonicTDM/ETDM \
    -Itopologies/photonicTDM/TDM \
    -Itopologies/squareRoot \
    -Itopologies/switches \
    -Itopologies/switches/crossbar \
    -Itopologies/switches/machzehndercrossbar \
    -Itopologies/torusNX

# Additional object and library files to link with
EXTRA_OBJS =

# Additional libraries (-L, -l options)
LIBS =

# Output directory
PROJECT_OUTPUT_DIR = out
PROJECTRELATIVE_PATH =
O = $(PROJECT_OUTPUT_DIR)/$(CONFIGNAME)/$(PROJECTRELATIVE_PATH)

# Object files for local .cc and .msg files
OBJS = \
    $O/addon/thermalmodel.o \
    $O/chipComponents/InoutConverter.o \
    $O/electronicComponents/RouterInport.o \
    $O/electronicComponents/mux.o \
    $O/electronicComponents/VirtualChannelControl.o \
    $O/electronicComponents/RouterCrossbar.o \
    $O/electronicComponents/serializer.o \
    $O/electronicComponents/electronicIOpad.o \
    $O/electronicComponents/RouterArbiter.o \
    $O/electronicComponents/RouterStat.o \
    $O/electronicComponents/electronic_channel.o \
    $O/electronicComponents/VC_Control.o \
    $O/electronicComponents/Arbiters/Arbiter_FB.o \
    $O/electronicComponents/Arbiters/Arbiter_BT_EJ.o \
    $O/electronicComponents/Arbiters/Arbiter_EHS.o \
    $O/electronicComponents/Arbiters/Arbiter_SR.o \
    $O/electronicComponents/Arbiters/Arbiter_PR.o \
    $O/electronicComponents/Arbiters/Arbiter_FT_Root.o \
    $O/electronicComponents/Arbiters/Arbiter_MITUCB_MeshNode.o \
    $O/electronicComponents/Arbiters/ArbiterCanRoute.o \
    $O/electronicComponents/Arbiters/Arbiter_BT_INJ.o \
    $O/electronicComponents/Arbiters/Arbiter_TNX_Node.o \
    $O/electronicComponents/Arbiters/ElectronicArbiter.o \
    $O/electronicComponents/Arbiters/Arbiter_XB_Node.o \
    $O/electronicComponents/Arbiters/Arbiter_NBT_Node_SP.o \
    $O/electronicComponents/Arbiters/Arbiter_NBSiN.o \
    $O/electronicComponents/Arbiters/PhotonicNoUturnArbiter.o \
    $O/electronicComponents/Arbiters/Arbiter_OM4x4.o \
    $O/electronicComponents/Arbiters/Photonic4x4Arbiter.o \
    $O/electronicComponents/Arbiters/Arbiter_FT_GWC.o \
    $O/electronicComponents/Arbiters/Arbiter_PM_Node.o \
    $O/electronicComponents/Arbiters/Arbiter_SR_Quad.o \
    $O/electronicComponents/Arbiters/Arbiter_TNX_Gateway.o \
    $O/electronicComponents/Arbiters/Arbiter_FT.o \
    $O/electronicComponents/Arbiters/Arbiter_SR_Middle.o \
    $O/electronicComponents/Arbiters/Arbiter_SR16x16.o \
    $O/electronicComponents/Arbiters/Arbiter_BT_GWS.o \
    $O/electronicComponents/Arbiters/PhotonicArbiter.o \
    $O/electronicComponents/Arbiters/Arbiter_MITUCB_MemoryNode.o \
    $O/electronicComponents/Arbiters/Arbiter_Crossbar.o \
    $O/electronicComponents/Arbiters/Arbiter_EM.o \
    $O/electronicComponents/Arbiters/Arbiter_PC.o \
    $O/electronicComponents/Arbiters/Arbiter.o \
    $O/electronicComponents/Arbiters/Arbiter_ET.o \
    $O/electronicComponents/Arbiters/Arbiter_SR4x4.o \
    $O/electronicComponents/Arbiters/Arbiter_BT_4x4.o \
    $O/electronicComponents/Arbiters/Arbiter_EM_circuit.o \
    $O/electronicComponents/Arbiters/Arbiter_CM.o \
    $O/electronicComponents/Arbiters/Arbiter_GWC.o \
    $O/electronicComponents/Arbiters/Arbiter_SR8x8.o \
    $O/electronicComponents/Arbiters/Arbiter_XB_Gateway.o \
    $O/electronicComponents/Arbiters/Arbiter_NBT_Gateway.o \
    $O/electronicComponents/Arbiters/Arbiter_SR_Top.o \
    $O/electronicComponents/Arbiters/Arbiter_NBT_Node.o \
    $O/electronicComponents/orion/ORION_Array_Info.o \
    $O/electronicComponents/orion/ORION_Config.o \
    $O/electronicComponents/orion/ORION_Arbiter.o \
    $O/electronicComponents/orion/ORION_Util.o \
    $O/electronicComponents/orion/ORION_Array_Internal.o \
    $O/electronicComponents/orion/ORION_Link.o \
    $O/electronicComponents/orion/ORION_Crossbar.o \
    $O/electronicComponents/orion/ORION_Array.o \
    $O/ioComponents/cDramModule.o \
    $O/ioComponents/DRAM_LRL/OCM_Control.o \
    $O/ioComponents/DRAM_LRL/NIF_MemGW.o \
    $O/ioComponents/DRAM_LRL/ProcessorDataStripper.o \
    $O/ioComponents/DRAM_LRL/DRAM_Bank.o \
    $O/ioComponents/DRAM_LRL/MemoryControl.o \
    $O/ioComponents/DRAMsim/Global_TQ_info.o \
    $O/ioComponents/DRAMsim/utilities.o \
    $O/ioComponents/DRAMsim/DRAMaddress.o \
    $O/ioComponents/DRAMsim/PrefetchCache.o \
    $O/ioComponents/DRAMsim/Command.o \
    $O/ioComponents/DRAMsim/Transaction.o \
    $O/ioComponents/DRAMsim/PowerConfig.o \
    $O/ioComponents/DRAMsim/PendingQueue.o \
    $O/ioComponents/DRAMsim/DIMM.o \
    $O/ioComponents/DRAMsim/RefreshQueue.o \
    $O/ioComponents/DRAMsim/DRAM_config.o \
    $O/ioComponents/DRAMsim/RBRR.o \
    $O/ioComponents/DRAMsim/Aux_Stat.o \
    $O/ioComponents/DRAMsim/BIU.o \
    $O/ioComponents/DRAMsim/TransactionQueue.o \
    $O/ioComponents/DRAMsim/DRAM_Sim.o \
    $O/ioComponents/DRAMsim/Memory_Controller.o \
    $O/ioComponents/DRAMsim/DRAM.o \
    $O/photonic/activeringelement.o \
    $O/photonic/modulatorelement.o \
    $O/photonic/phylayer.o \
    $O/photonic/resonanceprofile.o \
    $O/photonic/tworingelement.o \
    $O/photonic/activetworingelement.o \
    $O/photonic/machzehnderelement.o \
    $O/photonic/ringelement.o \
    $O/photonic/activemultiringelement.o \
    $O/photonic/activeelement.o \
    $O/photonic/detectorelement.o \
    $O/photonic/domainchanger.o \
    $O/photonic/basicelement.o \
    $O/photonic/multiringelement.o \
    $O/photonic/devices/StarCoupler.o \
    $O/photonic/devices/detector.o \
    $O/photonic/devices/join.o \
    $O/photonic/devices/bend.o \
    $O/photonic/devices/end.o \
    $O/photonic/devices/cross.o \
    $O/photonic/devices/line.o \
    $O/photonic/devices/laser.o \
    $O/photonic/devices/filter1x2.o \
    $O/photonic/devices/SwitchingFilter.o \
    $O/photonic/devices/pse2x2.o \
    $O/photonic/devices/modulator.o \
    $O/photonic/devices/pse1x2.o \
    $O/photonic/devices/activedirectionalcoupler.o \
    $O/photonic/devices/pse1x2nx.o \
    $O/photonic/devices/facet.o \
    $O/photonic/devices/attenuator.o \
    $O/photonic/devices/machzehnder/mzswitch2x2.o \
    $O/photonic/devices/machzehnder/mzswitch1x2.o \
    $O/photonic/devices/multiring/pse1x2r2.o \
    $O/photonic/devices/multiring/pse2x2r2.o \
    $O/photonic/devices/multiring/pse1x2nxr3.o \
    $O/photonic/devices/multiring/pse2x2r3.o \
    $O/photonic/devices/multiring/pse1x2nxr2.o \
    $O/photonic/devices/multiring/pse1x2multiring.o \
    $O/photonic/devices/multiring/pse2x2multiring.o \
    $O/photonic/devices/multiring/pse1x2r3.o \
    $O/photonic/devices/multiring/pse1x2nxmultiring.o \
    $O/processingPlane/ProcessorRouterArbiter.o \
    $O/processingPlane/Processor.o \
    $O/processingPlane/GatewaySignalControl.o \
    $O/processingPlane/DRAM_Cfg/DRAM_Cfg_AllNodes.o \
    $O/processingPlane/DRAM_Cfg/DRAM_Cfg_Periphery.o \
    $O/processingPlane/DRAM_Cfg/DRAM_Cfg_Periphery_SR16x16.o \
    $O/processingPlane/DRAM_Cfg/DRAM_Cfg_Periphery_SR8x8.o \
    $O/processingPlane/DRAM_Cfg/DRAM_Cfg.o \
    $O/processingPlane/DRAM_Cfg/DRAM_Cfg_Periphery_SR4x4.o \
    $O/processingPlane/addressTranslation/AddressTranslator_SquareRoot.o \
    $O/processingPlane/addressTranslation/AddressTranslator.o \
    $O/processingPlane/addressTranslation/AddressTranslator_FatTree.o \
    $O/processingPlane/addressTranslation/AddressTranslator_Standard.o \
    $O/processingPlane/apps/Application.o \
    $O/processingPlane/apps/DRAM/AppDRAMOne2One.o \
    $O/processingPlane/apps/DRAM/AppDRAMOne2All.o \
    $O/processingPlane/apps/DRAM/AppDRAMRandom.o \
    $O/processingPlane/apps/DRAM/AppDRAMAll2One.o \
    $O/processingPlane/apps/MITUCB/App_MITUCB_Random.o \
    $O/processingPlane/apps/MITUCB/App_MITUCB_All2All.o \
    $O/processingPlane/apps/SizeDistribution/SizeDistribution_Control.o \
    $O/processingPlane/apps/SizeDistribution/SizeDistribution.o \
    $O/processingPlane/apps/SizeDistribution/SizeDistribution_InvGamma.o \
    $O/processingPlane/apps/SizeDistribution/SizeDistribution_Constant.o \
    $O/processingPlane/apps/SizeDistribution/SizeDistribution_Gamma.o \
    $O/processingPlane/apps/SizeDistribution/SizeDistribution_Normal.o \
    $O/processingPlane/apps/model/AppFFTstream.o \
    $O/processingPlane/apps/model/AppFFT.o \
    $O/processingPlane/apps/model/AppDataFlow.o \
    $O/processingPlane/apps/synthetics/AppBitreverse.o \
    $O/processingPlane/apps/synthetics/AppHotspot.o \
    $O/processingPlane/apps/synthetics/AppRandom_FiniteQueue.o \
    $O/processingPlane/apps/synthetics/AppNeighbor.o \
    $O/processingPlane/apps/synthetics/AppTornado.o \
    $O/processingPlane/apps/synthetics/AppRandom.o \
    $O/processingPlane/apps/test/AppAll2All.o \
    $O/processingPlane/apps/test/AppOne2One.o \
    $O/processingPlane/apps/test/AppBcastTest.o \
    $O/processingPlane/apps/trace/App_LL_DepGraph.o \
    $O/processingPlane/apps/trace/AppTrace_LBL.o \
    $O/processingPlane/interfaces/NIF.o \
    $O/processingPlane/interfaces/NIF_Packet_Credit.o \
    $O/processingPlane/interfaces/NIF_Circuit.o \
    $O/processingPlane/interfaces/NIF_Electronic.o \
    $O/processingPlane/interfaces/NIF_Photonic.o \
    $O/processingPlane/interfaces/NIF_ElectronicCC.o \
    $O/simCore/NetworkAddress.o \
    $O/simCore/convert.o \
    $O/simCore/packetstat.o \
    $O/simCore/sim_includes.o \
    $O/statistics/statistics.o \
    $O/statistics/StatObject.o \
    $O/statistics/StatGroup.o \
    $O/statistics/LogFile.o \
    $O/statistics/statGroups/StatGroup_Sum.o \
    $O/statistics/statGroups/StatGroup_ListAll.o \
    $O/statistics/statObjects/StatObject_TimeAvg.o \
    $O/statistics/statObjects/StatObject_MMA.o \
    $O/statistics/statObjects/StatObject_Count.o \
    $O/statistics/statObjects/StatObject_Total.o \
    $O/statistics/statObjects/energy/StatObject_EnergyEvent.o \
    $O/statistics/statObjects/energy/StatObject_EnergyOn.o \
    $O/statistics/statObjects/energy/StatObject_EnergyStatic.o \
    $O/topologies/photonicTDM/ScheduleItem.o \
    $O/topologies/photonicTDM/ETDM/ETDM_Switch_Controller.o \
    $O/topologies/photonicTDM/ETDM/NIF_PhotonicETDM.o \
    $O/topologies/photonicTDM/TDM/NIF_PhotonicTDM.o \
    $O/topologies/photonicTDM/TDM/TDM_Switch_Controller.o \
    $O/simCore/messages_m.o

# Message files
MSGFILES = \
    simCore/messages.msg

#------------------------------------------------------------------------------

# Pull in OMNeT++ configuration (Makefile.inc or configuser.vc)

ifneq ("$(OMNETPP_CONFIGFILE)","")
CONFIGFILE = $(OMNETPP_CONFIGFILE)
else
ifneq ("$(OMNETPP_ROOT)","")
CONFIGFILE = $(OMNETPP_ROOT)/Makefile.inc
else
CONFIGFILE = $(shell opp_configfilepath)
endif
endif

ifeq ("$(wildcard $(CONFIGFILE))","")
$(error Config file '$(CONFIGFILE)' does not exist -- add the OMNeT++ bin directory to the path so that opp_configfilepath can be found, or set the OMNETPP_CONFIGFILE variable to point to Makefile.inc)
endif

include $(CONFIGFILE)

# Simulation kernel and user interface libraries
OMNETPP_LIB_SUBDIR = $(OMNETPP_LIB_DIR)/$(TOOLCHAIN_NAME)
OMNETPP_LIBS = -L"$(OMNETPP_LIB_SUBDIR)" -L"$(OMNETPP_LIB_DIR)" $(USERIF_LIBS) $(KERNEL_LIBS) $(SYS_LIBS)

COPTS = $(CFLAGS)  $(INCLUDE_PATH) -I$(OMNETPP_INCL_DIR)
MSGCOPTS = $(INCLUDE_PATH)

#------------------------------------------------------------------------------
# User-supplied makefile fragment(s)
# >>>
# <<<
#------------------------------------------------------------------------------

# Main target
all: $(TARGET)

$(TARGET) : $O/$(TARGET)
	$(LN) $O/$(TARGET) .

$O/$(TARGET): $(OBJS)  $(wildcard $(EXTRA_OBJS)) Makefile
	@$(MKPATH) $O
	$(CXX) $(LDFLAGS) -o $O/$(TARGET)  $(OBJS) $(EXTRA_OBJS) $(WHOLE_ARCHIVE_ON) $(LIBS) $(WHOLE_ARCHIVE_OFF) $(OMNETPP_LIBS)

.PHONY:

.SUFFIXES: .cc

$O/%.o: %.cc
	@$(MKPATH) $(dir $@)
	$(CXX) -c $(COPTS) -o $@ $<

%_m.cc %_m.h: %.msg
	$(MSGC) -s _m.cc $(MSGCOPTS) $?

msgheaders: $(MSGFILES:.msg=_m.h)

clean:
	-rm -rf $O
	-rm -f PhoenixSim PhoenixSim.exe libPhoenixSim.so libPhoenixSim.a libPhoenixSim.dll libPhoenixSim.dylib
	-rm -f ./*_m.cc ./*_m.h
	-rm -f addon/*_m.cc addon/*_m.h
	-rm -f addon/HotSpot-5.0/*_m.cc addon/HotSpot-5.0/*_m.h
	-rm -f chipComponents/*_m.cc chipComponents/*_m.h
	-rm -f data/*_m.cc data/*_m.h
	-rm -f electronicComponents/*_m.cc electronicComponents/*_m.h
	-rm -f electronicComponents/Arbiters/*_m.cc electronicComponents/Arbiters/*_m.h
	-rm -f electronicComponents/orion/*_m.cc electronicComponents/orion/*_m.h
	-rm -f electronicComponents/orion/ORION_1_Params/*_m.cc electronicComponents/orion/ORION_1_Params/*_m.h
	-rm -f electronicComponents/orion/ORION_2_Params/*_m.cc electronicComponents/orion/ORION_2_Params/*_m.h
	-rm -f images/*_m.cc images/*_m.h
	-rm -f images/components/*_m.cc images/components/*_m.h
	-rm -f images/components/old/*_m.cc images/components/old/*_m.h
	-rm -f images/mitucb/*_m.cc images/mitucb/*_m.h
	-rm -f images/planes/*_m.cc images/planes/*_m.h
	-rm -f ioComponents/*_m.cc ioComponents/*_m.h
	-rm -f ioComponents/DRAM_LRL/*_m.cc ioComponents/DRAM_LRL/*_m.h
	-rm -f ioComponents/DRAMsim/*_m.cc ioComponents/DRAMsim/*_m.h
	-rm -f ioComponents/DRAMsim/mem_system_def/*_m.cc ioComponents/DRAMsim/mem_system_def/*_m.h
	-rm -f parameters/*_m.cc parameters/*_m.h
	-rm -f parameters/Hendry-Thesis/*_m.cc parameters/Hendry-Thesis/*_m.h
	-rm -f parameters/default/*_m.cc parameters/default/*_m.h
	-rm -f photonic/*_m.cc photonic/*_m.h
	-rm -f photonic/PhotoCAD/*_m.cc photonic/PhotoCAD/*_m.h
	-rm -f photonic/devices/*_m.cc photonic/devices/*_m.h
	-rm -f photonic/devices/machzehnder/*_m.cc photonic/devices/machzehnder/*_m.h
	-rm -f photonic/devices/multiring/*_m.cc photonic/devices/multiring/*_m.h
	-rm -f photonic/switches/*_m.cc photonic/switches/*_m.h
	-rm -f photonic/switches/machzehnder/*_m.cc photonic/switches/machzehnder/*_m.h
	-rm -f photonic/switches/multiring/*_m.cc photonic/switches/multiring/*_m.h
	-rm -f processingPlane/*_m.cc processingPlane/*_m.h
	-rm -f processingPlane/DRAM_Cfg/*_m.cc processingPlane/DRAM_Cfg/*_m.h
	-rm -f processingPlane/addressTranslation/*_m.cc processingPlane/addressTranslation/*_m.h
	-rm -f processingPlane/apps/*_m.cc processingPlane/apps/*_m.h
	-rm -f processingPlane/apps/DRAM/*_m.cc processingPlane/apps/DRAM/*_m.h
	-rm -f processingPlane/apps/MITUCB/*_m.cc processingPlane/apps/MITUCB/*_m.h
	-rm -f processingPlane/apps/SizeDistribution/*_m.cc processingPlane/apps/SizeDistribution/*_m.h
	-rm -f processingPlane/apps/model/*_m.cc processingPlane/apps/model/*_m.h
	-rm -f processingPlane/apps/synthetics/*_m.cc processingPlane/apps/synthetics/*_m.h
	-rm -f processingPlane/apps/test/*_m.cc processingPlane/apps/test/*_m.h
	-rm -f processingPlane/apps/trace/*_m.cc processingPlane/apps/trace/*_m.h
	-rm -f processingPlane/interfaces/*_m.cc processingPlane/interfaces/*_m.h
	-rm -f results/*_m.cc results/*_m.h
	-rm -f simCore/*_m.cc simCore/*_m.h
	-rm -f statistics/*_m.cc statistics/*_m.h
	-rm -f statistics/statGroups/*_m.cc statistics/statGroups/*_m.h
	-rm -f statistics/statObjects/*_m.cc statistics/statObjects/*_m.h
	-rm -f statistics/statObjects/energy/*_m.cc statistics/statObjects/energy/*_m.h
	-rm -f topologies/*_m.cc topologies/*_m.h
	-rm -f topologies/CMesh/*_m.cc topologies/CMesh/*_m.h
	-rm -f topologies/blockingTorus/*_m.cc topologies/blockingTorus/*_m.h
	-rm -f topologies/electronicMesh/*_m.cc topologies/electronicMesh/*_m.h
	-rm -f topologies/electronicMesh_Circuit/*_m.cc topologies/electronicMesh_Circuit/*_m.h
	-rm -f topologies/electronicTorus/*_m.cc topologies/electronicTorus/*_m.h
	-rm -f topologies/flattenedButterfly/*_m.cc topologies/flattenedButterfly/*_m.h
	-rm -f topologies/nonblockingSiliconNitride/*_m.cc topologies/nonblockingSiliconNitride/*_m.h
	-rm -f topologies/nonblockingTorus/*_m.cc topologies/nonblockingTorus/*_m.h
	-rm -f topologies/photonicMesh/*_m.cc topologies/photonicMesh/*_m.h
	-rm -f topologies/photonicTDM/*_m.cc topologies/photonicTDM/*_m.h
	-rm -f topologies/photonicTDM/ETDM/*_m.cc topologies/photonicTDM/ETDM/*_m.h
	-rm -f topologies/photonicTDM/TDM/*_m.cc topologies/photonicTDM/TDM/*_m.h
	-rm -f topologies/squareRoot/*_m.cc topologies/squareRoot/*_m.h
	-rm -f topologies/switches/*_m.cc topologies/switches/*_m.h
	-rm -f topologies/switches/crossbar/*_m.cc topologies/switches/crossbar/*_m.h
	-rm -f topologies/switches/machzehndercrossbar/*_m.cc topologies/switches/machzehndercrossbar/*_m.h
	-rm -f topologies/torusNX/*_m.cc topologies/torusNX/*_m.h

cleanall: clean
	-rm -rf $(PROJECT_OUTPUT_DIR)

depend:
	$(MAKEDEPEND) $(INCLUDE_PATH) -f Makefile -P\$$O/ -- $(MSG_CC_FILES)  ./*.cc addon/*.cc addon/HotSpot-5.0/*.cc chipComponents/*.cc data/*.cc electronicComponents/*.cc electronicComponents/Arbiters/*.cc electronicComponents/orion/*.cc electronicComponents/orion/ORION_1_Params/*.cc electronicComponents/orion/ORION_2_Params/*.cc images/*.cc images/components/*.cc images/components/old/*.cc images/mitucb/*.cc images/planes/*.cc ioComponents/*.cc ioComponents/DRAM_LRL/*.cc ioComponents/DRAMsim/*.cc ioComponents/DRAMsim/mem_system_def/*.cc parameters/*.cc parameters/Hendry-Thesis/*.cc parameters/default/*.cc photonic/*.cc photonic/PhotoCAD/*.cc photonic/devices/*.cc photonic/devices/machzehnder/*.cc photonic/devices/multiring/*.cc photonic/switches/*.cc photonic/switches/machzehnder/*.cc photonic/switches/multiring/*.cc processingPlane/*.cc processingPlane/DRAM_Cfg/*.cc processingPlane/addressTranslation/*.cc processingPlane/apps/*.cc processingPlane/apps/DRAM/*.cc processingPlane/apps/MITUCB/*.cc processingPlane/apps/SizeDistribution/*.cc processingPlane/apps/model/*.cc processingPlane/apps/synthetics/*.cc processingPlane/apps/test/*.cc processingPlane/apps/trace/*.cc processingPlane/interfaces/*.cc results/*.cc simCore/*.cc statistics/*.cc statistics/statGroups/*.cc statistics/statObjects/*.cc statistics/statObjects/energy/*.cc topologies/*.cc topologies/CMesh/*.cc topologies/blockingTorus/*.cc topologies/electronicMesh/*.cc topologies/electronicMesh_Circuit/*.cc topologies/electronicTorus/*.cc topologies/flattenedButterfly/*.cc topologies/nonblockingSiliconNitride/*.cc topologies/nonblockingTorus/*.cc topologies/photonicMesh/*.cc topologies/photonicTDM/*.cc topologies/photonicTDM/ETDM/*.cc topologies/photonicTDM/TDM/*.cc topologies/squareRoot/*.cc topologies/switches/*.cc topologies/switches/crossbar/*.cc topologies/switches/machzehndercrossbar/*.cc topologies/torusNX/*.cc

# DO NOT DELETE THIS LINE -- make depend depends on it.
$O/addon/thermalmodel.o: addon/thermalmodel.cc \
	addon/HotSpot-5.0/temperature.h \
	statistics/StatObject.h \
	simCore/sim_includes.h \
	addon/HotSpot-5.0/flp.h \
	addon/HotSpot-5.0/temperature_grid.h \
	addon/HotSpot-5.0/util.h \
	addon/thermalmodel.h
$O/chipComponents/InoutConverter.o: chipComponents/InoutConverter.cc \
	chipComponents/InoutConverter.h
$O/electronicComponents/serializer.o: electronicComponents/serializer.cc \
	electronicComponents/orion/ORION_Array_Internal.h \
	statistics/statistics.h \
	electronicComponents/orion/ORION_Util.h \
	statistics/StatGroup.h \
	electronicComponents/orion/ORION_1_Params/SIM_power.h \
	simCore/messages_m.h \
	electronicComponents/orion/ORION_Array_Info.h \
	electronicComponents/orion/ORION_1_Params/SIM_power_test.h \
	statistics/LogFile.h \
	statistics/StatObject.h \
	simCore/sim_includes.h \
	electronicComponents/orion/ORION_Config.h \
	simCore/packetstat.h \
	electronicComponents/orion/ORION_1_Params/SIM_port.h \
	electronicComponents/orion/ORION_Array.h \
	photonic/phylayer.h \
	electronicComponents/serializer.h
$O/electronicComponents/VirtualChannelControl.o: electronicComponents/VirtualChannelControl.cc \
	simCore/messages_m.h \
	electronicComponents/VirtualChannelControl.h \
	electronicComponents/VC_Control.h
$O/electronicComponents/RouterInport.o: electronicComponents/RouterInport.cc \
	statistics/statistics.h \
	electronicComponents/orion/ORION_Array_Internal.h \
	electronicComponents/orion/ORION_Util.h \
	statistics/StatGroup.h \
	electronicComponents/orion/ORION_1_Params/SIM_power.h \
	simCore/messages_m.h \
	electronicComponents/VirtualChannelControl.h \
	electronicComponents/orion/ORION_Array_Info.h \
	electronicComponents/orion/ORION_1_Params/SIM_power_test.h \
	statistics/LogFile.h \
	electronicComponents/RouterInport.h \
	statistics/StatObject.h \
	electronicComponents/orion/ORION_Config.h \
	simCore/sim_includes.h \
	simCore/NetworkAddress.h \
	electronicComponents/orion/ORION_1_Params/SIM_port.h \
	electronicComponents/orion/ORION_Array.h \
	photonic/phylayer.h \
	electronicComponents/VC_Control.h
$O/electronicComponents/RouterArbiter.o: electronicComponents/RouterArbiter.cc \
	electronicComponents/Arbiters/Arbiter_EM_circuit.h \
	electronicComponents/Arbiters/Arbiter_FT.h \
	electronicComponents/Arbiters/Arbiter_MITUCB_MemoryNode.h \
	electronicComponents/Arbiters/Arbiter_SR_Top.h \
	electronicComponents/Arbiters/Arbiter_BT_INJ.h \
	electronicComponents/Arbiters/Arbiter_NBT_Gateway.h \
	processingPlane/apps/SizeDistribution/SizeDistribution.h \
	electronicComponents/orion/ORION_Config.h \
	electronicComponents/Arbiters/Arbiter.h \
	processingPlane/addressTranslation/AddressTranslator_FatTree.h \
	electronicComponents/Arbiters/Arbiter_ET.h \
	photonic/phylayer.h \
	electronicComponents/Arbiters/Arbiter_BT_GWS.h \
	addon/thermalmodel.h \
	electronicComponents/Arbiters/Arbiter_PC.h \
	electronicComponents/orion/ORION_Util.h \
	electronicComponents/orion/ORION_Arbiter.h \
	electronicComponents/Arbiters/Arbiter_XB_Gateway.h \
	electronicComponents/Arbiters/Arbiter_XB_Node.h \
	electronicComponents/orion/ORION_1_Params/SIM_power.h \
	electronicComponents/orion/ORION_Array_Info.h \
	electronicComponents/Arbiters/Arbiter_EHS.h \
	addon/HotSpot-5.0/temperature.h \
	electronicComponents/Arbiters/Arbiter_GWC.h \
	electronicComponents/Arbiters/ArbiterCanRoute.h \
	processingPlane/addressTranslation/AddressTranslator_Standard.h \
	electronicComponents/Arbiters/Arbiter_BT_EJ.h \
	electronicComponents/Arbiters/Arbiter_TNX_Gateway.h \
	electronicComponents/orion/ORION_Array_Internal.h \
	statistics/statistics.h \
	simCore/messages_m.h \
	electronicComponents/VirtualChannelControl.h \
	addon/HotSpot-5.0/util.h \
	electronicComponents/RouterArbiter.h \
	electronicComponents/Arbiters/Arbiter_EM.h \
	electronicComponents/orion/ORION_1_Params/SIM_power_test.h \
	statistics/LogFile.h \
	processingPlane/DRAM_Cfg/DRAM_Cfg.h \
	simCore/NetworkAddress.h \
	electronicComponents/orion/ORION_1_Params/SIM_port.h \
	electronicComponents/Arbiters/Arbiter_FT_GWC.h \
	addon/HotSpot-5.0/flp.h \
	electronicComponents/Arbiters/Arbiter_SR_Quad.h \
	electronicComponents/Arbiters/Arbiter_NBSiN.h \
	electronicComponents/Arbiters/Arbiter_SR.h \
	statistics/StatGroup.h \
	electronicComponents/Arbiters/Arbiter_PR.h \
	electronicComponents/Arbiters/Arbiter_PM_Node.h \
	electronicComponents/Arbiters/Arbiter_FT_Root.h \
	electronicComponents/Arbiters/Arbiter_MITUCB_MeshNode.h \
	electronicComponents/Arbiters/Arbiter_TNX_Node.h \
	electronicComponents/Arbiters/Arbiter_CM.h \
	electronicComponents/Arbiters/PhotonicArbiter.h \
	electronicComponents/Arbiters/PhotonicNoUturnArbiter.h \
	processingPlane/addressTranslation/AddressTranslator_SquareRoot.h \
	statistics/StatObject.h \
	simCore/sim_includes.h \
	electronicComponents/Arbiters/Arbiter_BT_4x4.h \
	electronicComponents/Arbiters/ElectronicArbiter.h \
	processingPlane/addressTranslation/AddressTranslator.h \
	electronicComponents/Arbiters/Arbiter_Crossbar.h \
	processingPlane/apps/Application.h \
	electronicComponents/Arbiters/Arbiter_SR_Middle.h \
	electronicComponents/Arbiters/Arbiter_NBT_Node.h \
	electronicComponents/Arbiters/Photonic4x4Arbiter.h \
	electronicComponents/orion/ORION_Array.h \
	processingPlane/Processor.h \
	addon/HotSpot-5.0/temperature_grid.h \
	electronicComponents/VC_Control.h \
	electronicComponents/Arbiters/Arbiter_FB.h
$O/electronicComponents/RouterStat.o: electronicComponents/RouterStat.cc \
	electronicComponents/orion/ORION_Array_Internal.h \
	statistics/statistics.h \
	electronicComponents/orion/ORION_Arbiter.h \
	electronicComponents/orion/ORION_Util.h \
	statistics/StatGroup.h \
	electronicComponents/orion/ORION_1_Params/SIM_power.h \
	simCore/messages_m.h \
	electronicComponents/RouterStat.h \
	electronicComponents/orion/ORION_Array_Info.h \
	electronicComponents/orion/ORION_Crossbar.h \
	electronicComponents/orion/ORION_1_Params/SIM_power_test.h \
	statistics/LogFile.h \
	statistics/StatObject.h \
	simCore/sim_includes.h \
	electronicComponents/orion/ORION_Config.h \
	electronicComponents/orion/ORION_1_Params/SIM_port.h \
	electronicComponents/orion/ORION_Link.h \
	electronicComponents/orion/ORION_Array.h \
	photonic/phylayer.h
$O/electronicComponents/RouterCrossbar.o: electronicComponents/RouterCrossbar.cc \
	statistics/statistics.h \
	electronicComponents/RouterCrossbar.h \
	electronicComponents/orion/ORION_Util.h \
	statistics/StatGroup.h \
	electronicComponents/orion/ORION_1_Params/SIM_power.h \
	simCore/messages_m.h \
	electronicComponents/orion/ORION_1_Params/SIM_power_test.h \
	statistics/LogFile.h \
	electronicComponents/orion/ORION_Crossbar.h \
	statistics/StatObject.h \
	electronicComponents/orion/ORION_Config.h \
	simCore/sim_includes.h \
	electronicComponents/orion/ORION_1_Params/SIM_port.h \
	photonic/phylayer.h
$O/electronicComponents/electronic_channel.o: electronicComponents/electronic_channel.cc \
	electronicComponents/orion/ORION_Array_Internal.h \
	statistics/statistics.h \
	electronicComponents/orion/ORION_Util.h \
	electronicComponents/orion/ORION_Arbiter.h \
	statistics/StatGroup.h \
	electronicComponents/orion/ORION_1_Params/SIM_power.h \
	simCore/messages_m.h \
	electronicComponents/RouterStat.h \
	electronicComponents/orion/ORION_Array_Info.h \
	electronicComponents/orion/ORION_Crossbar.h \
	electronicComponents/orion/ORION_1_Params/SIM_power_test.h \
	statistics/LogFile.h \
	electronicComponents/electronic_channel.h \
	statistics/StatObject.h \
	electronicComponents/orion/ORION_Config.h \
	simCore/sim_includes.h \
	electronicComponents/orion/ORION_1_Params/SIM_port.h \
	electronicComponents/orion/ORION_Link.h \
	electronicComponents/orion/ORION_Array.h \
	photonic/phylayer.h
$O/electronicComponents/electronicIOpad.o: electronicComponents/electronicIOpad.cc \
	statistics/LogFile.h \
	electronicComponents/orion/ORION_1_Params/SIM_power_test.h \
	statistics/statistics.h \
	statistics/StatObject.h \
	simCore/sim_includes.h \
	electronicComponents/orion/ORION_Config.h \
	statistics/StatGroup.h \
	electronicComponents/orion/ORION_1_Params/SIM_port.h \
	electronicComponents/orion/ORION_1_Params/SIM_power.h \
	electronicComponents/electronicIOpad.h \
	simCore/messages_m.h \
	photonic/phylayer.h
$O/electronicComponents/mux.o: electronicComponents/mux.cc \
	simCore/packetstat.h \
	simCore/messages_m.h \
	electronicComponents/mux.h
$O/electronicComponents/VC_Control.o: electronicComponents/VC_Control.cc \
	simCore/messages_m.h \
	electronicComponents/VC_Control.h
$O/electronicComponents/Arbiters/Arbiter_SR_Top.o: electronicComponents/Arbiters/Arbiter_SR_Top.cc \
	statistics/statistics.h \
	electronicComponents/orion/ORION_Array_Internal.h \
	electronicComponents/Arbiters/Arbiter_SR_Top.h \
	simCore/messages_m.h \
	electronicComponents/VirtualChannelControl.h \
	addon/HotSpot-5.0/util.h \
	statistics/LogFile.h \
	electronicComponents/orion/ORION_1_Params/SIM_power_test.h \
	processingPlane/apps/SizeDistribution/SizeDistribution.h \
	electronicComponents/orion/ORION_Config.h \
	electronicComponents/Arbiters/Arbiter.h \
	processingPlane/DRAM_Cfg/DRAM_Cfg.h \
	simCore/NetworkAddress.h \
	electronicComponents/orion/ORION_1_Params/SIM_port.h \
	addon/HotSpot-5.0/flp.h \
	electronicComponents/Arbiters/Arbiter_SR.h \
	photonic/phylayer.h \
	addon/thermalmodel.h \
	electronicComponents/orion/ORION_Util.h \
	electronicComponents/orion/ORION_Arbiter.h \
	statistics/StatGroup.h \
	electronicComponents/orion/ORION_1_Params/SIM_power.h \
	electronicComponents/orion/ORION_Array_Info.h \
	electronicComponents/Arbiters/PhotonicArbiter.h \
	electronicComponents/Arbiters/PhotonicNoUturnArbiter.h \
	statistics/StatObject.h \
	addon/HotSpot-5.0/temperature.h \
	simCore/sim_includes.h \
	electronicComponents/Arbiters/ArbiterCanRoute.h \
	electronicComponents/Arbiters/ElectronicArbiter.h \
	processingPlane/addressTranslation/AddressTranslator.h \
	processingPlane/apps/Application.h \
	electronicComponents/Arbiters/Photonic4x4Arbiter.h \
	electronicComponents/orion/ORION_Array.h \
	processingPlane/Processor.h \
	electronicComponents/VC_Control.h \
	addon/HotSpot-5.0/temperature_grid.h
$O/electronicComponents/Arbiters/Arbiter_PR.o: electronicComponents/Arbiters/Arbiter_PR.cc \
	electronicComponents/orion/ORION_Array_Internal.h \
	electronicComponents/orion/ORION_Arbiter.h \
	electronicComponents/orion/ORION_Util.h \
	electronicComponents/Arbiters/Arbiter_PR.h \
	electronicComponents/orion/ORION_1_Params/SIM_power.h \
	simCore/messages_m.h \
	electronicComponents/VirtualChannelControl.h \
	electronicComponents/orion/ORION_Array_Info.h \
	electronicComponents/orion/ORION_1_Params/SIM_power_test.h \
	statistics/StatObject.h \
	simCore/sim_includes.h \
	electronicComponents/orion/ORION_Config.h \
	electronicComponents/Arbiters/Arbiter.h \
	electronicComponents/Arbiters/ArbiterCanRoute.h \
	electronicComponents/Arbiters/ElectronicArbiter.h \
	electronicComponents/orion/ORION_1_Params/SIM_port.h \
	simCore/NetworkAddress.h \
	processingPlane/addressTranslation/AddressTranslator.h \
	electronicComponents/orion/ORION_Array.h \
	electronicComponents/VC_Control.h
$O/electronicComponents/Arbiters/Arbiter_PC.o: electronicComponents/Arbiters/Arbiter_PC.cc \
	electronicComponents/orion/ORION_Array_Internal.h \
	electronicComponents/orion/ORION_Arbiter.h \
	electronicComponents/orion/ORION_Util.h \
	electronicComponents/orion/ORION_1_Params/SIM_power.h \
	simCore/messages_m.h \
	electronicComponents/VirtualChannelControl.h \
	electronicComponents/orion/ORION_Array_Info.h \
	electronicComponents/orion/ORION_1_Params/SIM_power_test.h \
	statistics/StatObject.h \
	simCore/sim_includes.h \
	electronicComponents/Arbiters/Arbiter.h \
	electronicComponents/orion/ORION_Config.h \
	electronicComponents/Arbiters/ArbiterCanRoute.h \
	electronicComponents/Arbiters/ElectronicArbiter.h \
	electronicComponents/orion/ORION_1_Params/SIM_port.h \
	processingPlane/addressTranslation/AddressTranslator.h \
	simCore/NetworkAddress.h \
	electronicComponents/orion/ORION_Array.h \
	electronicComponents/VC_Control.h \
	electronicComponents/Arbiters/Arbiter_PC.h
$O/electronicComponents/Arbiters/Arbiter_SR_Quad.o: electronicComponents/Arbiters/Arbiter_SR_Quad.cc \
	electronicComponents/orion/ORION_Array_Internal.h \
	statistics/statistics.h \
	simCore/messages_m.h \
	electronicComponents/VirtualChannelControl.h \
	addon/HotSpot-5.0/util.h \
	electronicComponents/orion/ORION_1_Params/SIM_power_test.h \
	statistics/LogFile.h \
	processingPlane/apps/SizeDistribution/SizeDistribution.h \
	electronicComponents/Arbiters/Arbiter.h \
	electronicComponents/orion/ORION_Config.h \
	processingPlane/DRAM_Cfg/DRAM_Cfg.h \
	simCore/NetworkAddress.h \
	electronicComponents/orion/ORION_1_Params/SIM_port.h \
	electronicComponents/Arbiters/Arbiter_SR_Quad.h \
	addon/HotSpot-5.0/flp.h \
	electronicComponents/Arbiters/Arbiter_SR.h \
	photonic/phylayer.h \
	addon/thermalmodel.h \
	electronicComponents/orion/ORION_Util.h \
	electronicComponents/orion/ORION_Arbiter.h \
	statistics/StatGroup.h \
	electronicComponents/orion/ORION_1_Params/SIM_power.h \
	electronicComponents/orion/ORION_Array_Info.h \
	electronicComponents/Arbiters/PhotonicArbiter.h \
	electronicComponents/Arbiters/PhotonicNoUturnArbiter.h \
	addon/HotSpot-5.0/temperature.h \
	statistics/StatObject.h \
	simCore/sim_includes.h \
	electronicComponents/Arbiters/ArbiterCanRoute.h \
	electronicComponents/Arbiters/ElectronicArbiter.h \
	processingPlane/addressTranslation/AddressTranslator.h \
	processingPlane/apps/Application.h \
	electronicComponents/Arbiters/Photonic4x4Arbiter.h \
	processingPlane/Processor.h \
	electronicComponents/orion/ORION_Array.h \
	addon/HotSpot-5.0/temperature_grid.h \
	electronicComponents/VC_Control.h
$O/electronicComponents/Arbiters/Arbiter_TNX_Gateway.o: electronicComponents/Arbiters/Arbiter_TNX_Gateway.cc \
	electronicComponents/orion/ORION_Array_Internal.h \
	electronicComponents/orion/ORION_Arbiter.h \
	electronicComponents/orion/ORION_Util.h \
	electronicComponents/orion/ORION_1_Params/SIM_power.h \
	simCore/messages_m.h \
	electronicComponents/VirtualChannelControl.h \
	electronicComponents/orion/ORION_Array_Info.h \
	electronicComponents/Arbiters/PhotonicArbiter.h \
	electronicComponents/orion/ORION_1_Params/SIM_power_test.h \
	statistics/StatObject.h \
	simCore/sim_includes.h \
	electronicComponents/orion/ORION_Config.h \
	electronicComponents/Arbiters/Arbiter.h \
	electronicComponents/Arbiters/ArbiterCanRoute.h \
	electronicComponents/Arbiters/ElectronicArbiter.h \
	electronicComponents/orion/ORION_1_Params/SIM_port.h \
	simCore/NetworkAddress.h \
	processingPlane/addressTranslation/AddressTranslator.h \
	electronicComponents/Arbiters/Arbiter_TNX_Gateway.h \
	electronicComponents/orion/ORION_Array.h \
	electronicComponents/VC_Control.h
$O/electronicComponents/Arbiters/Arbiter_SR8x8.o: electronicComponents/Arbiters/Arbiter_SR8x8.cc \
	electronicComponents/orion/ORION_Array_Internal.h \
	statistics/statistics.h \
	simCore/messages_m.h \
	electronicComponents/VirtualChannelControl.h \
	addon/HotSpot-5.0/util.h \
	electronicComponents/orion/ORION_1_Params/SIM_power_test.h \
	statistics/LogFile.h \
	processingPlane/apps/SizeDistribution/SizeDistribution.h \
	electronicComponents/orion/ORION_Config.h \
	electronicComponents/Arbiters/Arbiter.h \
	processingPlane/DRAM_Cfg/DRAM_Cfg.h \
	simCore/NetworkAddress.h \
	electronicComponents/orion/ORION_1_Params/SIM_port.h \
	electronicComponents/Arbiters/Arbiter_SR8x8.h \
	addon/HotSpot-5.0/flp.h \
	photonic/phylayer.h \
	addon/thermalmodel.h \
	electronicComponents/orion/ORION_Util.h \
	electronicComponents/orion/ORION_Arbiter.h \
	statistics/StatGroup.h \
	electronicComponents/orion/ORION_1_Params/SIM_power.h \
	electronicComponents/orion/ORION_Array_Info.h \
	electronicComponents/Arbiters/PhotonicArbiter.h \
	electronicComponents/Arbiters/PhotonicNoUturnArbiter.h \
	addon/HotSpot-5.0/temperature.h \
	statistics/StatObject.h \
	simCore/sim_includes.h \
	electronicComponents/Arbiters/ArbiterCanRoute.h \
	electronicComponents/Arbiters/ElectronicArbiter.h \
	processingPlane/addressTranslation/AddressTranslator.h \
	processingPlane/apps/Application.h \
	electronicComponents/Arbiters/Photonic4x4Arbiter.h \
	electronicComponents/orion/ORION_Array.h \
	processingPlane/Processor.h \
	addon/HotSpot-5.0/temperature_grid.h \
	electronicComponents/VC_Control.h
$O/electronicComponents/Arbiters/Arbiter_GWC.o: electronicComponents/Arbiters/Arbiter_GWC.cc \
	statistics/statistics.h \
	electronicComponents/orion/ORION_Array_Internal.h \
	simCore/messages_m.h \
	electronicComponents/VirtualChannelControl.h \
	addon/HotSpot-5.0/util.h \
	statistics/LogFile.h \
	electronicComponents/orion/ORION_1_Params/SIM_power_test.h \
	processingPlane/apps/SizeDistribution/SizeDistribution.h \
	electronicComponents/Arbiters/Arbiter.h \
	electronicComponents/orion/ORION_Config.h \
	processingPlane/DRAM_Cfg/DRAM_Cfg.h \
	simCore/NetworkAddress.h \
	electronicComponents/orion/ORION_1_Params/SIM_port.h \
	addon/HotSpot-5.0/flp.h \
	photonic/phylayer.h \
	addon/thermalmodel.h \
	electronicComponents/orion/ORION_Arbiter.h \
	electronicComponents/orion/ORION_Util.h \
	statistics/StatGroup.h \
	electronicComponents/orion/ORION_1_Params/SIM_power.h \
	electronicComponents/orion/ORION_Array_Info.h \
	electronicComponents/Arbiters/PhotonicArbiter.h \
	electronicComponents/Arbiters/PhotonicNoUturnArbiter.h \
	addon/HotSpot-5.0/temperature.h \
	statistics/StatObject.h \
	simCore/sim_includes.h \
	electronicComponents/Arbiters/Arbiter_GWC.h \
	electronicComponents/Arbiters/ArbiterCanRoute.h \
	electronicComponents/Arbiters/ElectronicArbiter.h \
	processingPlane/addressTranslation/AddressTranslator.h \
	processingPlane/apps/Application.h \
	processingPlane/Processor.h \
	electronicComponents/orion/ORION_Array.h \
	addon/HotSpot-5.0/temperature_grid.h \
	electronicComponents/VC_Control.h
$O/electronicComponents/Arbiters/Arbiter_SR.o: electronicComponents/Arbiters/Arbiter_SR.cc \
	electronicComponents/orion/ORION_Array_Internal.h \
	statistics/statistics.h \
	simCore/messages_m.h \
	electronicComponents/VirtualChannelControl.h \
	addon/HotSpot-5.0/util.h \
	electronicComponents/orion/ORION_1_Params/SIM_power_test.h \
	statistics/LogFile.h \
	processingPlane/apps/SizeDistribution/SizeDistribution.h \
	electronicComponents/orion/ORION_Config.h \
	electronicComponents/Arbiters/Arbiter.h \
	processingPlane/DRAM_Cfg/DRAM_Cfg.h \
	simCore/NetworkAddress.h \
	electronicComponents/orion/ORION_1_Params/SIM_port.h \
	addon/HotSpot-5.0/flp.h \
	electronicComponents/Arbiters/Arbiter_SR.h \
	photonic/phylayer.h \
	addon/thermalmodel.h \
	electronicComponents/orion/ORION_Util.h \
	electronicComponents/orion/ORION_Arbiter.h \
	statistics/StatGroup.h \
	electronicComponents/orion/ORION_1_Params/SIM_power.h \
	electronicComponents/orion/ORION_Array_Info.h \
	electronicComponents/Arbiters/PhotonicArbiter.h \
	electronicComponents/Arbiters/PhotonicNoUturnArbiter.h \
	addon/HotSpot-5.0/temperature.h \
	statistics/StatObject.h \
	simCore/sim_includes.h \
	electronicComponents/Arbiters/ArbiterCanRoute.h \
	electronicComponents/Arbiters/ElectronicArbiter.h \
	processingPlane/addressTranslation/AddressTranslator.h \
	processingPlane/apps/Application.h \
	electronicComponents/Arbiters/Photonic4x4Arbiter.h \
	electronicComponents/orion/ORION_Array.h \
	processingPlane/Processor.h \
	addon/HotSpot-5.0/temperature_grid.h \
	electronicComponents/VC_Control.h
$O/electronicComponents/Arbiters/PhotonicNoUturnArbiter.o: electronicComponents/Arbiters/PhotonicNoUturnArbiter.cc \
	statistics/statistics.h \
	electronicComponents/orion/ORION_Array_Internal.h \
	simCore/messages_m.h \
	electronicComponents/VirtualChannelControl.h \
	addon/HotSpot-5.0/util.h \
	statistics/LogFile.h \
	electronicComponents/orion/ORION_1_Params/SIM_power_test.h \
	processingPlane/apps/SizeDistribution/SizeDistribution.h \
	electronicComponents/Arbiters/Arbiter.h \
	electronicComponents/orion/ORION_Config.h \
	processingPlane/DRAM_Cfg/DRAM_Cfg.h \
	simCore/NetworkAddress.h \
	electronicComponents/orion/ORION_1_Params/SIM_port.h \
	addon/HotSpot-5.0/flp.h \
	photonic/phylayer.h \
	addon/thermalmodel.h \
	electronicComponents/orion/ORION_Arbiter.h \
	electronicComponents/orion/ORION_Util.h \
	statistics/StatGroup.h \
	electronicComponents/orion/ORION_1_Params/SIM_power.h \
	electronicComponents/orion/ORION_Array_Info.h \
	electronicComponents/Arbiters/PhotonicArbiter.h \
	electronicComponents/Arbiters/PhotonicNoUturnArbiter.h \
	addon/HotSpot-5.0/temperature.h \
	statistics/StatObject.h \
	simCore/sim_includes.h \
	electronicComponents/Arbiters/ArbiterCanRoute.h \
	electronicComponents/Arbiters/ElectronicArbiter.h \
	processingPlane/addressTranslation/AddressTranslator.h \
	processingPlane/apps/Application.h \
	processingPlane/Processor.h \
	electronicComponents/orion/ORION_Array.h \
	addon/HotSpot-5.0/temperature_grid.h \
	electronicComponents/VC_Control.h
$O/electronicComponents/Arbiters/Arbiter_CM.o: electronicComponents/Arbiters/Arbiter_CM.cc \
	electronicComponents/orion/ORION_Array_Internal.h \
	electronicComponents/orion/ORION_Arbiter.h \
	electronicComponents/orion/ORION_Util.h \
	electronicComponents/orion/ORION_1_Params/SIM_power.h \
	simCore/messages_m.h \
	electronicComponents/VirtualChannelControl.h \
	electronicComponents/Arbiters/Arbiter_CM.h \
	electronicComponents/orion/ORION_Array_Info.h \
	electronicComponents/orion/ORION_1_Params/SIM_power_test.h \
	statistics/StatObject.h \
	simCore/sim_includes.h \
	electronicComponents/orion/ORION_Config.h \
	electronicComponents/Arbiters/Arbiter.h \
	electronicComponents/Arbiters/ArbiterCanRoute.h \
	electronicComponents/Arbiters/ElectronicArbiter.h \
	electronicComponents/orion/ORION_1_Params/SIM_port.h \
	simCore/NetworkAddress.h \
	processingPlane/addressTranslation/AddressTranslator.h \
	electronicComponents/orion/ORION_Array.h \
	electronicComponents/VC_Control.h
$O/electronicComponents/Arbiters/Arbiter_NBT_Node.o: electronicComponents/Arbiters/Arbiter_NBT_Node.cc \
	electronicComponents/orion/ORION_Array_Internal.h \
	electronicComponents/orion/ORION_Arbiter.h \
	electronicComponents/orion/ORION_Util.h \
	electronicComponents/orion/ORION_1_Params/SIM_power.h \
	simCore/messages_m.h \
	electronicComponents/VirtualChannelControl.h \
	electronicComponents/orion/ORION_Array_Info.h \
	electronicComponents/Arbiters/PhotonicArbiter.h \
	electronicComponents/orion/ORION_1_Params/SIM_power_test.h \
	statistics/StatObject.h \
	simCore/sim_includes.h \
	electronicComponents/orion/ORION_Config.h \
	electronicComponents/Arbiters/Arbiter.h \
	electronicComponents/Arbiters/ArbiterCanRoute.h \
	electronicComponents/Arbiters/ElectronicArbiter.h \
	electronicComponents/orion/ORION_1_Params/SIM_port.h \
	processingPlane/addressTranslation/AddressTranslator.h \
	simCore/NetworkAddress.h \
	electronicComponents/Arbiters/Arbiter_NBT_Node.h \
	electronicComponents/orion/ORION_Array.h \
	electronicComponents/VC_Control.h
$O/electronicComponents/Arbiters/Arbiter_BT_EJ.o: electronicComponents/Arbiters/Arbiter_BT_EJ.cc \
	electronicComponents/orion/ORION_Array_Internal.h \
	statistics/statistics.h \
	simCore/messages_m.h \
	electronicComponents/VirtualChannelControl.h \
	addon/HotSpot-5.0/util.h \
	electronicComponents/orion/ORION_1_Params/SIM_power_test.h \
	statistics/LogFile.h \
	processingPlane/apps/SizeDistribution/SizeDistribution.h \
	electronicComponents/orion/ORION_Config.h \
	electronicComponents/Arbiters/Arbiter.h \
	processingPlane/DRAM_Cfg/DRAM_Cfg.h \
	simCore/NetworkAddress.h \
	electronicComponents/orion/ORION_1_Params/SIM_port.h \
	addon/HotSpot-5.0/flp.h \
	photonic/phylayer.h \
	addon/thermalmodel.h \
	electronicComponents/orion/ORION_Util.h \
	electronicComponents/orion/ORION_Arbiter.h \
	statistics/StatGroup.h \
	electronicComponents/orion/ORION_1_Params/SIM_power.h \
	electronicComponents/orion/ORION_Array_Info.h \
	electronicComponents/Arbiters/PhotonicArbiter.h \
	electronicComponents/Arbiters/PhotonicNoUturnArbiter.h \
	addon/HotSpot-5.0/temperature.h \
	statistics/StatObject.h \
	simCore/sim_includes.h \
	electronicComponents/Arbiters/ArbiterCanRoute.h \
	electronicComponents/Arbiters/ElectronicArbiter.h \
	processingPlane/addressTranslation/AddressTranslator.h \
	electronicComponents/Arbiters/Arbiter_BT_EJ.h \
	processingPlane/apps/Application.h \
	electronicComponents/orion/ORION_Array.h \
	processingPlane/Processor.h \
	addon/HotSpot-5.0/temperature_grid.h \
	electronicComponents/VC_Control.h
$O/electronicComponents/Arbiters/Arbiter_FB.o: electronicComponents/Arbiters/Arbiter_FB.cc \
	electronicComponents/orion/ORION_Array_Internal.h \
	electronicComponents/orion/ORION_Arbiter.h \
	electronicComponents/orion/ORION_Util.h \
	electronicComponents/orion/ORION_1_Params/SIM_power.h \
	simCore/messages_m.h \
	electronicComponents/VirtualChannelControl.h \
	electronicComponents/orion/ORION_Array_Info.h \
	electronicComponents/orion/ORION_1_Params/SIM_power_test.h \
	statistics/StatObject.h \
	simCore/sim_includes.h \
	electronicComponents/Arbiters/Arbiter.h \
	electronicComponents/orion/ORION_Config.h \
	electronicComponents/Arbiters/ArbiterCanRoute.h \
	electronicComponents/Arbiters/ElectronicArbiter.h \
	electronicComponents/orion/ORION_1_Params/SIM_port.h \
	processingPlane/addressTranslation/AddressTranslator.h \
	simCore/NetworkAddress.h \
	electronicComponents/orion/ORION_Array.h \
	electronicComponents/VC_Control.h \
	electronicComponents/Arbiters/Arbiter_FB.h
$O/electronicComponents/Arbiters/Arbiter_EM_circuit.o: electronicComponents/Arbiters/Arbiter_EM_circuit.cc \
	statistics/statistics.h \
	electronicComponents/orion/ORION_Array_Internal.h \
	electronicComponents/Arbiters/Arbiter_EM_circuit.h \
	simCore/messages_m.h \
	electronicComponents/VirtualChannelControl.h \
	addon/HotSpot-5.0/util.h \
	statistics/LogFile.h \
	electronicComponents/orion/ORION_1_Params/SIM_power_test.h \
	processingPlane/apps/SizeDistribution/SizeDistribution.h \
	electronicComponents/Arbiters/Arbiter.h \
	electronicComponents/orion/ORION_Config.h \
	processingPlane/DRAM_Cfg/DRAM_Cfg.h \
	simCore/NetworkAddress.h \
	electronicComponents/orion/ORION_1_Params/SIM_port.h \
	addon/HotSpot-5.0/flp.h \
	photonic/phylayer.h \
	addon/thermalmodel.h \
	electronicComponents/orion/ORION_Arbiter.h \
	electronicComponents/orion/ORION_Util.h \
	statistics/StatGroup.h \
	electronicComponents/orion/ORION_1_Params/SIM_power.h \
	electronicComponents/orion/ORION_Array_Info.h \
	electronicComponents/Arbiters/PhotonicArbiter.h \
	electronicComponents/Arbiters/PhotonicNoUturnArbiter.h \
	addon/HotSpot-5.0/temperature.h \
	statistics/StatObject.h \
	simCore/sim_includes.h \
	electronicComponents/Arbiters/ArbiterCanRoute.h \
	electronicComponents/Arbiters/ElectronicArbiter.h \
	processingPlane/addressTranslation/AddressTranslator.h \
	processingPlane/apps/Application.h \
	electronicComponents/orion/ORION_Array.h \
	processingPlane/Processor.h \
	addon/HotSpot-5.0/temperature_grid.h \
	electronicComponents/VC_Control.h
$O/electronicComponents/Arbiters/Arbiter_MITUCB_MemoryNode.o: electronicComponents/Arbiters/Arbiter_MITUCB_MemoryNode.cc \
	electronicComponents/orion/ORION_Array_Internal.h \
	electronicComponents/Arbiters/Arbiter_MITUCB_MemoryNode.h \
	electronicComponents/orion/ORION_Arbiter.h \
	electronicComponents/orion/ORION_Util.h \
	electronicComponents/orion/ORION_1_Params/SIM_power.h \
	simCore/messages_m.h \
	electronicComponents/VirtualChannelControl.h \
	electronicComponents/orion/ORION_Array_Info.h \
	electronicComponents/orion/ORION_1_Params/SIM_power_test.h \
	statistics/StatObject.h \
	simCore/sim_includes.h \
	electronicComponents/Arbiters/Arbiter.h \
	electronicComponents/orion/ORION_Config.h \
	electronicComponents/Arbiters/ArbiterCanRoute.h \
	electronicComponents/Arbiters/ElectronicArbiter.h \
	electronicComponents/orion/ORION_1_Params/SIM_port.h \
	processingPlane/addressTranslation/AddressTranslator.h \
	simCore/NetworkAddress.h \
	electronicComponents/orion/ORION_Array.h \
	electronicComponents/VC_Control.h
$O/electronicComponents/Arbiters/Arbiter_ET.o: electronicComponents/Arbiters/Arbiter_ET.cc \
	electronicComponents/orion/ORION_Array_Internal.h \
	electronicComponents/orion/ORION_Arbiter.h \
	electronicComponents/orion/ORION_Util.h \
	electronicComponents/orion/ORION_1_Params/SIM_power.h \
	simCore/messages_m.h \
	electronicComponents/VirtualChannelControl.h \
	electronicComponents/orion/ORION_Array_Info.h \
	electronicComponents/orion/ORION_1_Params/SIM_power_test.h \
	statistics/StatObject.h \
	electronicComponents/Arbiters/Arbiter.h \
	simCore/sim_includes.h \
	electronicComponents/orion/ORION_Config.h \
	electronicComponents/Arbiters/ArbiterCanRoute.h \
	electronicComponents/Arbiters/ElectronicArbiter.h \
	simCore/NetworkAddress.h \
	processingPlane/addressTranslation/AddressTranslator.h \
	electronicComponents/orion/ORION_1_Params/SIM_port.h \
	electronicComponents/orion/ORION_Array.h \
	electronicComponents/Arbiters/Arbiter_ET.h \
	electronicComponents/VC_Control.h
$O/electronicComponents/Arbiters/Arbiter_SR16x16.o: electronicComponents/Arbiters/Arbiter_SR16x16.cc \
	statistics/statistics.h \
	electronicComponents/orion/ORION_Array_Internal.h \
	simCore/messages_m.h \
	electronicComponents/VirtualChannelControl.h \
	addon/HotSpot-5.0/util.h \
	statistics/LogFile.h \
	electronicComponents/orion/ORION_1_Params/SIM_power_test.h \
	processingPlane/apps/SizeDistribution/SizeDistribution.h \
	electronicComponents/orion/ORION_Config.h \
	electronicComponents/Arbiters/Arbiter.h \
	processingPlane/DRAM_Cfg/DRAM_Cfg.h \
	simCore/NetworkAddress.h \
	electronicComponents/orion/ORION_1_Params/SIM_port.h \
	addon/HotSpot-5.0/flp.h \
	photonic/phylayer.h \
	addon/thermalmodel.h \
	electronicComponents/orion/ORION_Util.h \
	electronicComponents/orion/ORION_Arbiter.h \
	statistics/StatGroup.h \
	electronicComponents/orion/ORION_1_Params/SIM_power.h \
	electronicComponents/orion/ORION_Array_Info.h \
	electronicComponents/Arbiters/PhotonicArbiter.h \
	electronicComponents/Arbiters/PhotonicNoUturnArbiter.h \
	statistics/StatObject.h \
	addon/HotSpot-5.0/temperature.h \
	electronicComponents/Arbiters/Arbiter_SR16x16.h \
	simCore/sim_includes.h \
	electronicComponents/Arbiters/ArbiterCanRoute.h \
	electronicComponents/Arbiters/ElectronicArbiter.h \
	processingPlane/addressTranslation/AddressTranslator.h \
	processingPlane/apps/Application.h \
	electronicComponents/Arbiters/Photonic4x4Arbiter.h \
	electronicComponents/orion/ORION_Array.h \
	processingPlane/Processor.h \
	electronicComponents/VC_Control.h \
	addon/HotSpot-5.0/temperature_grid.h
$O/electronicComponents/Arbiters/PhotonicArbiter.o: electronicComponents/Arbiters/PhotonicArbiter.cc \
	electronicComponents/orion/ORION_Array_Internal.h \
	electronicComponents/orion/ORION_Arbiter.h \
	electronicComponents/orion/ORION_Util.h \
	electronicComponents/orion/ORION_1_Params/SIM_power.h \
	simCore/messages_m.h \
	electronicComponents/VirtualChannelControl.h \
	electronicComponents/orion/ORION_Array_Info.h \
	electronicComponents/Arbiters/PhotonicArbiter.h \
	electronicComponents/orion/ORION_1_Params/SIM_power_test.h \
	statistics/StatObject.h \
	simCore/sim_includes.h \
	electronicComponents/Arbiters/Arbiter.h \
	electronicComponents/orion/ORION_Config.h \
	electronicComponents/Arbiters/ArbiterCanRoute.h \
	electronicComponents/Arbiters/ElectronicArbiter.h \
	electronicComponents/orion/ORION_1_Params/SIM_port.h \
	processingPlane/addressTranslation/AddressTranslator.h \
	simCore/NetworkAddress.h \
	electronicComponents/orion/ORION_Array.h \
	electronicComponents/VC_Control.h
$O/electronicComponents/Arbiters/Arbiter_OM4x4.o: electronicComponents/Arbiters/Arbiter_OM4x4.cc \
	electronicComponents/orion/ORION_Array_Internal.h \
	electronicComponents/orion/ORION_Arbiter.h \
	electronicComponents/orion/ORION_Util.h \
	electronicComponents/orion/ORION_1_Params/SIM_power.h \
	simCore/messages_m.h \
	electronicComponents/VirtualChannelControl.h \
	electronicComponents/orion/ORION_Array_Info.h \
	electronicComponents/Arbiters/PhotonicArbiter.h \
	electronicComponents/orion/ORION_1_Params/SIM_power_test.h \
	statistics/StatObject.h \
	simCore/sim_includes.h \
	electronicComponents/orion/ORION_Config.h \
	electronicComponents/Arbiters/Arbiter.h \
	electronicComponents/Arbiters/ArbiterCanRoute.h \
	electronicComponents/Arbiters/ElectronicArbiter.h \
	electronicComponents/orion/ORION_1_Params/SIM_port.h \
	processingPlane/addressTranslation/AddressTranslator.h \
	simCore/NetworkAddress.h \
	electronicComponents/orion/ORION_Array.h \
	electronicComponents/VC_Control.h \
	electronicComponents/Arbiters/Arbiter_OM4x4.h
$O/electronicComponents/Arbiters/Arbiter_SR4x4.o: electronicComponents/Arbiters/Arbiter_SR4x4.cc \
	electronicComponents/orion/ORION_Array_Internal.h \
	statistics/statistics.h \
	simCore/messages_m.h \
	electronicComponents/Arbiters/Arbiter_SR4x4.h \
	electronicComponents/VirtualChannelControl.h \
	addon/HotSpot-5.0/util.h \
	electronicComponents/orion/ORION_1_Params/SIM_power_test.h \
	statistics/LogFile.h \
	processingPlane/apps/SizeDistribution/SizeDistribution.h \
	electronicComponents/Arbiters/Arbiter.h \
	electronicComponents/orion/ORION_Config.h \
	processingPlane/DRAM_Cfg/DRAM_Cfg.h \
	simCore/NetworkAddress.h \
	electronicComponents/orion/ORION_1_Params/SIM_port.h \
	addon/HotSpot-5.0/flp.h \
	photonic/phylayer.h \
	addon/thermalmodel.h \
	electronicComponents/orion/ORION_Util.h \
	electronicComponents/orion/ORION_Arbiter.h \
	statistics/StatGroup.h \
	electronicComponents/orion/ORION_1_Params/SIM_power.h \
	electronicComponents/orion/ORION_Array_Info.h \
	electronicComponents/Arbiters/PhotonicArbiter.h \
	electronicComponents/Arbiters/PhotonicNoUturnArbiter.h \
	addon/HotSpot-5.0/temperature.h \
	statistics/StatObject.h \
	simCore/sim_includes.h \
	electronicComponents/Arbiters/ArbiterCanRoute.h \
	electronicComponents/Arbiters/ElectronicArbiter.h \
	processingPlane/addressTranslation/AddressTranslator.h \
	processingPlane/apps/Application.h \
	electronicComponents/Arbiters/Photonic4x4Arbiter.h \
	processingPlane/Processor.h \
	electronicComponents/orion/ORION_Array.h \
	addon/HotSpot-5.0/temperature_grid.h \
	electronicComponents/VC_Control.h
$O/electronicComponents/Arbiters/Arbiter_TNX_Node.o: electronicComponents/Arbiters/Arbiter_TNX_Node.cc \
	electronicComponents/orion/ORION_Array_Internal.h \
	statistics/statistics.h \
	simCore/messages_m.h \
	electronicComponents/VirtualChannelControl.h \
	addon/HotSpot-5.0/util.h \
	electronicComponents/orion/ORION_1_Params/SIM_power_test.h \
	statistics/LogFile.h \
	processingPlane/apps/SizeDistribution/SizeDistribution.h \
	electronicComponents/Arbiters/Arbiter.h \
	electronicComponents/orion/ORION_Config.h \
	processingPlane/DRAM_Cfg/DRAM_Cfg.h \
	simCore/NetworkAddress.h \
	electronicComponents/orion/ORION_1_Params/SIM_port.h \
	addon/HotSpot-5.0/flp.h \
	photonic/phylayer.h \
	addon/thermalmodel.h \
	electronicComponents/orion/ORION_Util.h \
	electronicComponents/orion/ORION_Arbiter.h \
	statistics/StatGroup.h \
	electronicComponents/orion/ORION_1_Params/SIM_power.h \
	electronicComponents/Arbiters/Arbiter_TNX_Node.h \
	electronicComponents/orion/ORION_Array_Info.h \
	electronicComponents/Arbiters/PhotonicArbiter.h \
	electronicComponents/Arbiters/PhotonicNoUturnArbiter.h \
	addon/HotSpot-5.0/temperature.h \
	statistics/StatObject.h \
	simCore/sim_includes.h \
	electronicComponents/Arbiters/ArbiterCanRoute.h \
	electronicComponents/Arbiters/ElectronicArbiter.h \
	processingPlane/addressTranslation/AddressTranslator.h \
	processingPlane/apps/Application.h \
	electronicComponents/Arbiters/Photonic4x4Arbiter.h \
	processingPlane/Processor.h \
	electronicComponents/orion/ORION_Array.h \
	addon/HotSpot-5.0/temperature_grid.h \
	electronicComponents/VC_Control.h
$O/electronicComponents/Arbiters/Arbiter_EM.o: electronicComponents/Arbiters/Arbiter_EM.cc \
	electronicComponents/orion/ORION_Array_Internal.h \
	electronicComponents/orion/ORION_Arbiter.h \
	electronicComponents/orion/ORION_Util.h \
	electronicComponents/orion/ORION_1_Params/SIM_power.h \
	simCore/messages_m.h \
	electronicComponents/VirtualChannelControl.h \
	electronicComponents/orion/ORION_Array_Info.h \
	electronicComponents/Arbiters/Arbiter_EM.h \
	electronicComponents/orion/ORION_1_Params/SIM_power_test.h \
	statistics/StatObject.h \
	simCore/sim_includes.h \
	electronicComponents/orion/ORION_Config.h \
	electronicComponents/Arbiters/Arbiter.h \
	electronicComponents/Arbiters/ArbiterCanRoute.h \
	electronicComponents/Arbiters/ElectronicArbiter.h \
	electronicComponents/orion/ORION_1_Params/SIM_port.h \
	simCore/NetworkAddress.h \
	processingPlane/addressTranslation/AddressTranslator.h \
	electronicComponents/orion/ORION_Array.h \
	electronicComponents/VC_Control.h
$O/electronicComponents/Arbiters/Arbiter_NBT_Node_SP.o: electronicComponents/Arbiters/Arbiter_NBT_Node_SP.cc \
	electronicComponents/orion/ORION_Array_Internal.h \
	electronicComponents/orion/ORION_Arbiter.h \
	electronicComponents/orion/ORION_Util.h \
	electronicComponents/orion/ORION_1_Params/SIM_power.h \
	electronicComponents/Arbiters/Arbiter_NBT_Node_SP.h \
	simCore/messages_m.h \
	electronicComponents/VirtualChannelControl.h \
	electronicComponents/orion/ORION_Array_Info.h \
	electronicComponents/Arbiters/PhotonicArbiter.h \
	electronicComponents/orion/ORION_1_Params/SIM_power_test.h \
	statistics/StatObject.h \
	simCore/sim_includes.h \
	electronicComponents/orion/ORION_Config.h \
	electronicComponents/Arbiters/Arbiter.h \
	electronicComponents/Arbiters/ArbiterCanRoute.h \
	electronicComponents/Arbiters/ElectronicArbiter.h \
	electronicComponents/orion/ORION_1_Params/SIM_port.h \
	processingPlane/addressTranslation/AddressTranslator.h \
	simCore/NetworkAddress.h \
	electronicComponents/Arbiters/Arbiter_NBT_Node.h \
	electronicComponents/orion/ORION_Array.h \
	electronicComponents/VC_Control.h
$O/electronicComponents/Arbiters/Arbiter.o: electronicComponents/Arbiters/Arbiter.cc \
	electronicComponents/orion/ORION_Array_Internal.h \
	statistics/statistics.h \
	electronicComponents/orion/ORION_Util.h \
	electronicComponents/orion/ORION_Arbiter.h \
	statistics/StatGroup.h \
	electronicComponents/orion/ORION_1_Params/SIM_power.h \
	simCore/messages_m.h \
	electronicComponents/VirtualChannelControl.h \
	electronicComponents/orion/ORION_Array_Info.h \
	electronicComponents/orion/ORION_1_Params/SIM_power_test.h \
	statistics/LogFile.h \
	statistics/StatObject.h \
	electronicComponents/Arbiters/Arbiter.h \
	electronicComponents/orion/ORION_Config.h \
	simCore/sim_includes.h \
	electronicComponents/Arbiters/ArbiterCanRoute.h \
	simCore/NetworkAddress.h \
	processingPlane/addressTranslation/AddressTranslator.h \
	electronicComponents/orion/ORION_1_Params/SIM_port.h \
	electronicComponents/orion/ORION_Array.h \
	photonic/phylayer.h \
	electronicComponents/VC_Control.h
$O/electronicComponents/Arbiters/Arbiter_SR_Middle.o: electronicComponents/Arbiters/Arbiter_SR_Middle.cc \
	statistics/statistics.h \
	electronicComponents/orion/ORION_Array_Internal.h \
	simCore/messages_m.h \
	electronicComponents/VirtualChannelControl.h \
	addon/HotSpot-5.0/util.h \
	statistics/LogFile.h \
	electronicComponents/orion/ORION_1_Params/SIM_power_test.h \
	processingPlane/apps/SizeDistribution/SizeDistribution.h \
	electronicComponents/orion/ORION_Config.h \
	electronicComponents/Arbiters/Arbiter.h \
	processingPlane/DRAM_Cfg/DRAM_Cfg.h \
	simCore/NetworkAddress.h \
	electronicComponents/orion/ORION_1_Params/SIM_port.h \
	addon/HotSpot-5.0/flp.h \
	electronicComponents/Arbiters/Arbiter_SR.h \
	photonic/phylayer.h \
	addon/thermalmodel.h \
	electronicComponents/orion/ORION_Util.h \
	electronicComponents/orion/ORION_Arbiter.h \
	statistics/StatGroup.h \
	electronicComponents/orion/ORION_1_Params/SIM_power.h \
	electronicComponents/orion/ORION_Array_Info.h \
	electronicComponents/Arbiters/PhotonicArbiter.h \
	electronicComponents/Arbiters/PhotonicNoUturnArbiter.h \
	statistics/StatObject.h \
	addon/HotSpot-5.0/temperature.h \
	simCore/sim_includes.h \
	electronicComponents/Arbiters/ArbiterCanRoute.h \
	electronicComponents/Arbiters/ElectronicArbiter.h \
	processingPlane/addressTranslation/AddressTranslator.h \
	processingPlane/apps/Application.h \
	electronicComponents/Arbiters/Arbiter_SR_Middle.h \
	electronicComponents/Arbiters/Photonic4x4Arbiter.h \
	electronicComponents/orion/ORION_Array.h \
	processingPlane/Processor.h \
	electronicComponents/VC_Control.h \
	addon/HotSpot-5.0/temperature_grid.h
$O/electronicComponents/Arbiters/Arbiter_PM_Node.o: electronicComponents/Arbiters/Arbiter_PM_Node.cc \
	electronicComponents/orion/ORION_Array_Internal.h \
	electronicComponents/orion/ORION_Arbiter.h \
	electronicComponents/orion/ORION_Util.h \
	electronicComponents/Arbiters/Arbiter_PM_Node.h \
	electronicComponents/orion/ORION_1_Params/SIM_power.h \
	simCore/messages_m.h \
	electronicComponents/VirtualChannelControl.h \
	electronicComponents/orion/ORION_Array_Info.h \
	electronicComponents/Arbiters/PhotonicArbiter.h \
	electronicComponents/orion/ORION_1_Params/SIM_power_test.h \
	statistics/StatObject.h \
	simCore/sim_includes.h \
	electronicComponents/orion/ORION_Config.h \
	electronicComponents/Arbiters/Arbiter.h \
	electronicComponents/Arbiters/ArbiterCanRoute.h \
	electronicComponents/Arbiters/ElectronicArbiter.h \
	electronicComponents/orion/ORION_1_Params/SIM_port.h \
	processingPlane/addressTranslation/AddressTranslator.h \
	simCore/NetworkAddress.h \
	electronicComponents/orion/ORION_Array.h \
	electronicComponents/VC_Control.h
$O/electronicComponents/Arbiters/Arbiter_BT_GWS.o: electronicComponents/Arbiters/Arbiter_BT_GWS.cc \
	electronicComponents/orion/ORION_Array_Internal.h \
	electronicComponents/orion/ORION_Arbiter.h \
	electronicComponents/orion/ORION_Util.h \
	electronicComponents/orion/ORION_1_Params/SIM_power.h \
	simCore/messages_m.h \
	electronicComponents/VirtualChannelControl.h \
	electronicComponents/orion/ORION_Array_Info.h \
	electronicComponents/Arbiters/PhotonicArbiter.h \
	electronicComponents/orion/ORION_1_Params/SIM_power_test.h \
	statistics/StatObject.h \
	simCore/sim_includes.h \
	electronicComponents/orion/ORION_Config.h \
	electronicComponents/Arbiters/Arbiter.h \
	electronicComponents/Arbiters/ArbiterCanRoute.h \
	electronicComponents/Arbiters/ElectronicArbiter.h \
	electronicComponents/orion/ORION_1_Params/SIM_port.h \
	processingPlane/addressTranslation/AddressTranslator.h \
	simCore/NetworkAddress.h \
	electronicComponents/orion/ORION_Array.h \
	electronicComponents/VC_Control.h \
	electronicComponents/Arbiters/Arbiter_BT_GWS.h
$O/electronicComponents/Arbiters/Arbiter_FT_Root.o: electronicComponents/Arbiters/Arbiter_FT_Root.cc \
	electronicComponents/orion/ORION_Array_Internal.h \
	statistics/statistics.h \
	simCore/messages_m.h \
	electronicComponents/VirtualChannelControl.h \
	addon/HotSpot-5.0/util.h \
	electronicComponents/orion/ORION_1_Params/SIM_power_test.h \
	statistics/LogFile.h \
	processingPlane/apps/SizeDistribution/SizeDistribution.h \
	electronicComponents/orion/ORION_Config.h \
	electronicComponents/Arbiters/Arbiter.h \
	processingPlane/DRAM_Cfg/DRAM_Cfg.h \
	simCore/NetworkAddress.h \
	electronicComponents/orion/ORION_1_Params/SIM_port.h \
	addon/HotSpot-5.0/flp.h \
	photonic/phylayer.h \
	addon/thermalmodel.h \
	electronicComponents/orion/ORION_Util.h \
	electronicComponents/orion/ORION_Arbiter.h \
	statistics/StatGroup.h \
	electronicComponents/orion/ORION_1_Params/SIM_power.h \
	electronicComponents/Arbiters/Arbiter_FT_Root.h \
	electronicComponents/orion/ORION_Array_Info.h \
	electronicComponents/Arbiters/PhotonicArbiter.h \
	electronicComponents/Arbiters/PhotonicNoUturnArbiter.h \
	addon/HotSpot-5.0/temperature.h \
	statistics/StatObject.h \
	simCore/sim_includes.h \
	electronicComponents/Arbiters/ArbiterCanRoute.h \
	electronicComponents/Arbiters/ElectronicArbiter.h \
	processingPlane/addressTranslation/AddressTranslator.h \
	processingPlane/apps/Application.h \
	electronicComponents/orion/ORION_Array.h \
	processingPlane/Processor.h \
	addon/HotSpot-5.0/temperature_grid.h \
	electronicComponents/VC_Control.h
$O/electronicComponents/Arbiters/Arbiter_NBSiN.o: electronicComponents/Arbiters/Arbiter_NBSiN.cc \
	electronicComponents/orion/ORION_Array_Internal.h \
	electronicComponents/orion/ORION_Arbiter.h \
	electronicComponents/orion/ORION_Util.h \
	electronicComponents/orion/ORION_1_Params/SIM_power.h \
	simCore/messages_m.h \
	electronicComponents/VirtualChannelControl.h \
	electronicComponents/orion/ORION_Array_Info.h \
	electronicComponents/Arbiters/PhotonicArbiter.h \
	electronicComponents/orion/ORION_1_Params/SIM_power_test.h \
	statistics/StatObject.h \
	simCore/sim_includes.h \
	electronicComponents/orion/ORION_Config.h \
	electronicComponents/Arbiters/Arbiter.h \
	electronicComponents/Arbiters/ArbiterCanRoute.h \
	electronicComponents/Arbiters/ElectronicArbiter.h \
	electronicComponents/orion/ORION_1_Params/SIM_port.h \
	processingPlane/addressTranslation/AddressTranslator.h \
	simCore/NetworkAddress.h \
	electronicComponents/Arbiters/Arbiter_NBSiN.h \
	electronicComponents/orion/ORION_Array.h \
	electronicComponents/VC_Control.h
$O/electronicComponents/Arbiters/Arbiter_XB_Node.o: electronicComponents/Arbiters/Arbiter_XB_Node.cc \
	electronicComponents/orion/ORION_Array_Internal.h \
	electronicComponents/orion/ORION_Arbiter.h \
	electronicComponents/orion/ORION_Util.h \
	electronicComponents/Arbiters/Arbiter_XB_Node.h \
	electronicComponents/orion/ORION_1_Params/SIM_power.h \
	simCore/messages_m.h \
	electronicComponents/VirtualChannelControl.h \
	electronicComponents/orion/ORION_Array_Info.h \
	electronicComponents/Arbiters/PhotonicArbiter.h \
	electronicComponents/orion/ORION_1_Params/SIM_power_test.h \
	statistics/StatObject.h \
	electronicComponents/Arbiters/Arbiter.h \
	electronicComponents/orion/ORION_Config.h \
	simCore/sim_includes.h \
	electronicComponents/Arbiters/ArbiterCanRoute.h \
	electronicComponents/Arbiters/ElectronicArbiter.h \
	simCore/NetworkAddress.h \
	processingPlane/addressTranslation/AddressTranslator.h \
	electronicComponents/orion/ORION_1_Params/SIM_port.h \
	electronicComponents/orion/ORION_Array.h \
	electronicComponents/VC_Control.h
$O/electronicComponents/Arbiters/ArbiterCanRoute.o: electronicComponents/Arbiters/ArbiterCanRoute.cc \
	electronicComponents/Arbiters/ArbiterCanRoute.h \
	simCore/NetworkAddress.h \
	simCore/messages_m.h
$O/electronicComponents/Arbiters/Arbiter_MITUCB_MeshNode.o: electronicComponents/Arbiters/Arbiter_MITUCB_MeshNode.cc \
	electronicComponents/orion/ORION_Array_Internal.h \
	electronicComponents/orion/ORION_Arbiter.h \
	electronicComponents/orion/ORION_Util.h \
	electronicComponents/orion/ORION_1_Params/SIM_power.h \
	electronicComponents/Arbiters/Arbiter_MITUCB_MeshNode.h \
	simCore/messages_m.h \
	electronicComponents/VirtualChannelControl.h \
	electronicComponents/orion/ORION_Array_Info.h \
	electronicComponents/orion/ORION_1_Params/SIM_power_test.h \
	statistics/StatObject.h \
	simCore/sim_includes.h \
	electronicComponents/orion/ORION_Config.h \
	electronicComponents/Arbiters/Arbiter.h \
	electronicComponents/Arbiters/ArbiterCanRoute.h \
	electronicComponents/Arbiters/ElectronicArbiter.h \
	electronicComponents/orion/ORION_1_Params/SIM_port.h \
	simCore/NetworkAddress.h \
	processingPlane/addressTranslation/AddressTranslator.h \
	electronicComponents/orion/ORION_Array.h \
	electronicComponents/VC_Control.h
$O/electronicComponents/Arbiters/Photonic4x4Arbiter.o: electronicComponents/Arbiters/Photonic4x4Arbiter.cc \
	statistics/statistics.h \
	electronicComponents/orion/ORION_Array_Internal.h \
	simCore/messages_m.h \
	electronicComponents/VirtualChannelControl.h \
	addon/HotSpot-5.0/util.h \
	statistics/LogFile.h \
	electronicComponents/orion/ORION_1_Params/SIM_power_test.h \
	processingPlane/apps/SizeDistribution/SizeDistribution.h \
	electronicComponents/Arbiters/Arbiter.h \
	electronicComponents/orion/ORION_Config.h \
	processingPlane/DRAM_Cfg/DRAM_Cfg.h \
	simCore/NetworkAddress.h \
	electronicComponents/orion/ORION_1_Params/SIM_port.h \
	addon/HotSpot-5.0/flp.h \
	photonic/phylayer.h \
	addon/thermalmodel.h \
	electronicComponents/orion/ORION_Arbiter.h \
	electronicComponents/orion/ORION_Util.h \
	statistics/StatGroup.h \
	electronicComponents/orion/ORION_1_Params/SIM_power.h \
	electronicComponents/orion/ORION_Array_Info.h \
	electronicComponents/Arbiters/PhotonicArbiter.h \
	electronicComponents/Arbiters/PhotonicNoUturnArbiter.h \
	addon/HotSpot-5.0/temperature.h \
	statistics/StatObject.h \
	simCore/sim_includes.h \
	electronicComponents/Arbiters/ArbiterCanRoute.h \
	electronicComponents/Arbiters/ElectronicArbiter.h \
	processingPlane/addressTranslation/AddressTranslator.h \
	processingPlane/apps/Application.h \
	electronicComponents/Arbiters/Photonic4x4Arbiter.h \
	processingPlane/Processor.h \
	electronicComponents/orion/ORION_Array.h \
	addon/HotSpot-5.0/temperature_grid.h \
	electronicComponents/VC_Control.h
$O/electronicComponents/Arbiters/Arbiter_NBT_Gateway.o: electronicComponents/Arbiters/Arbiter_NBT_Gateway.cc \
	electronicComponents/orion/ORION_Array_Internal.h \
	electronicComponents/orion/ORION_Arbiter.h \
	electronicComponents/orion/ORION_Util.h \
	electronicComponents/orion/ORION_1_Params/SIM_power.h \
	simCore/messages_m.h \
	electronicComponents/VirtualChannelControl.h \
	electronicComponents/Arbiters/Arbiter_NBT_Gateway.h \
	electronicComponents/orion/ORION_Array_Info.h \
	electronicComponents/Arbiters/PhotonicArbiter.h \
	electronicComponents/orion/ORION_1_Params/SIM_power_test.h \
	statistics/StatObject.h \
	simCore/sim_includes.h \
	electronicComponents/orion/ORION_Config.h \
	electronicComponents/Arbiters/Arbiter.h \
	electronicComponents/Arbiters/ArbiterCanRoute.h \
	electronicComponents/Arbiters/ElectronicArbiter.h \
	electronicComponents/orion/ORION_1_Params/SIM_port.h \
	processingPlane/addressTranslation/AddressTranslator.h \
	simCore/NetworkAddress.h \
	electronicComponents/orion/ORION_Array.h \
	electronicComponents/VC_Control.h
$O/electronicComponents/Arbiters/Arbiter_FT.o: electronicComponents/Arbiters/Arbiter_FT.cc \
	statistics/statistics.h \
	electronicComponents/orion/ORION_Array_Internal.h \
	electronicComponents/Arbiters/Arbiter_FT.h \
	simCore/messages_m.h \
	electronicComponents/VirtualChannelControl.h \
	addon/HotSpot-5.0/util.h \
	statistics/LogFile.h \
	electronicComponents/orion/ORION_1_Params/SIM_power_test.h \
	processingPlane/apps/SizeDistribution/SizeDistribution.h \
	electronicComponents/Arbiters/Arbiter.h \
	electronicComponents/orion/ORION_Config.h \
	processingPlane/DRAM_Cfg/DRAM_Cfg.h \
	simCore/NetworkAddress.h \
	electronicComponents/orion/ORION_1_Params/SIM_port.h \
	addon/HotSpot-5.0/flp.h \
	photonic/phylayer.h \
	addon/thermalmodel.h \
	electronicComponents/orion/ORION_Arbiter.h \
	electronicComponents/orion/ORION_Util.h \
	statistics/StatGroup.h \
	electronicComponents/orion/ORION_1_Params/SIM_power.h \
	electronicComponents/orion/ORION_Array_Info.h \
	electronicComponents/Arbiters/PhotonicArbiter.h \
	electronicComponents/Arbiters/PhotonicNoUturnArbiter.h \
	addon/HotSpot-5.0/temperature.h \
	statistics/StatObject.h \
	simCore/sim_includes.h \
	electronicComponents/Arbiters/ArbiterCanRoute.h \
	electronicComponents/Arbiters/ElectronicArbiter.h \
	processingPlane/addressTranslation/AddressTranslator.h \
	processingPlane/apps/Application.h \
	processingPlane/Processor.h \
	electronicComponents/orion/ORION_Array.h \
	addon/HotSpot-5.0/temperature_grid.h \
	electronicComponents/VC_Control.h
$O/electronicComponents/Arbiters/Arbiter_BT_INJ.o: electronicComponents/Arbiters/Arbiter_BT_INJ.cc \
	electronicComponents/orion/ORION_Array_Internal.h \
	statistics/statistics.h \
	electronicComponents/Arbiters/Arbiter_BT_INJ.h \
	simCore/messages_m.h \
	electronicComponents/VirtualChannelControl.h \
	addon/HotSpot-5.0/util.h \
	electronicComponents/orion/ORION_1_Params/SIM_power_test.h \
	statistics/LogFile.h \
	processingPlane/apps/SizeDistribution/SizeDistribution.h \
	electronicComponents/orion/ORION_Config.h \
	electronicComponents/Arbiters/Arbiter.h \
	processingPlane/DRAM_Cfg/DRAM_Cfg.h \
	simCore/NetworkAddress.h \
	electronicComponents/orion/ORION_1_Params/SIM_port.h \
	addon/HotSpot-5.0/flp.h \
	photonic/phylayer.h \
	addon/thermalmodel.h \
	electronicComponents/orion/ORION_Util.h \
	electronicComponents/orion/ORION_Arbiter.h \
	statistics/StatGroup.h \
	electronicComponents/orion/ORION_1_Params/SIM_power.h \
	electronicComponents/orion/ORION_Array_Info.h \
	electronicComponents/Arbiters/PhotonicArbiter.h \
	electronicComponents/Arbiters/PhotonicNoUturnArbiter.h \
	addon/HotSpot-5.0/temperature.h \
	statistics/StatObject.h \
	simCore/sim_includes.h \
	electronicComponents/Arbiters/ArbiterCanRoute.h \
	electronicComponents/Arbiters/ElectronicArbiter.h \
	processingPlane/addressTranslation/AddressTranslator.h \
	processingPlane/apps/Application.h \
	electronicComponents/orion/ORION_Array.h \
	processingPlane/Processor.h \
	addon/HotSpot-5.0/temperature_grid.h \
	electronicComponents/VC_Control.h
$O/electronicComponents/Arbiters/Arbiter_XB_Gateway.o: electronicComponents/Arbiters/Arbiter_XB_Gateway.cc \
	electronicComponents/orion/ORION_Array_Internal.h \
	electronicComponents/Arbiters/Arbiter_XB_Gateway.h \
	electronicComponents/orion/ORION_Arbiter.h \
	electronicComponents/orion/ORION_Util.h \
	electronicComponents/orion/ORION_1_Params/SIM_power.h \
	simCore/messages_m.h \
	electronicComponents/VirtualChannelControl.h \
	electronicComponents/orion/ORION_Array_Info.h \
	electronicComponents/Arbiters/PhotonicArbiter.h \
	electronicComponents/orion/ORION_1_Params/SIM_power_test.h \
	statistics/StatObject.h \
	simCore/sim_includes.h \
	electronicComponents/orion/ORION_Config.h \
	electronicComponents/Arbiters/Arbiter.h \
	electronicComponents/Arbiters/ArbiterCanRoute.h \
	electronicComponents/Arbiters/ElectronicArbiter.h \
	simCore/NetworkAddress.h \
	processingPlane/addressTranslation/AddressTranslator.h \
	electronicComponents/orion/ORION_1_Params/SIM_port.h \
	electronicComponents/orion/ORION_Array.h \
	electronicComponents/VC_Control.h
$O/electronicComponents/Arbiters/Arbiter_EHS.o: electronicComponents/Arbiters/Arbiter_EHS.cc \
	electronicComponents/orion/ORION_Array_Internal.h \
	electronicComponents/orion/ORION_Arbiter.h \
	electronicComponents/orion/ORION_Util.h \
	electronicComponents/orion/ORION_1_Params/SIM_power.h \
	simCore/messages_m.h \
	electronicComponents/VirtualChannelControl.h \
	electronicComponents/orion/ORION_Array_Info.h \
	electronicComponents/Arbiters/Arbiter_EHS.h \
	electronicComponents/orion/ORION_1_Params/SIM_power_test.h \
	statistics/StatObject.h \
	simCore/sim_includes.h \
	electronicComponents/Arbiters/Arbiter.h \
	electronicComponents/orion/ORION_Config.h \
	electronicComponents/Arbiters/ArbiterCanRoute.h \
	electronicComponents/Arbiters/ElectronicArbiter.h \
	electronicComponents/orion/ORION_1_Params/SIM_port.h \
	processingPlane/addressTranslation/AddressTranslator.h \
	simCore/NetworkAddress.h \
	electronicComponents/orion/ORION_Array.h \
	electronicComponents/VC_Control.h
$O/electronicComponents/Arbiters/Arbiter_BT_4x4.o: electronicComponents/Arbiters/Arbiter_BT_4x4.cc \
	electronicComponents/orion/ORION_Array_Internal.h \
	statistics/statistics.h \
	simCore/messages_m.h \
	electronicComponents/VirtualChannelControl.h \
	addon/HotSpot-5.0/util.h \
	electronicComponents/orion/ORION_1_Params/SIM_power_test.h \
	statistics/LogFile.h \
	processingPlane/apps/SizeDistribution/SizeDistribution.h \
	electronicComponents/orion/ORION_Config.h \
	electronicComponents/Arbiters/Arbiter.h \
	processingPlane/DRAM_Cfg/DRAM_Cfg.h \
	simCore/NetworkAddress.h \
	electronicComponents/orion/ORION_1_Params/SIM_port.h \
	addon/HotSpot-5.0/flp.h \
	photonic/phylayer.h \
	addon/thermalmodel.h \
	electronicComponents/orion/ORION_Util.h \
	electronicComponents/orion/ORION_Arbiter.h \
	statistics/StatGroup.h \
	electronicComponents/orion/ORION_1_Params/SIM_power.h \
	electronicComponents/orion/ORION_Array_Info.h \
	electronicComponents/Arbiters/PhotonicArbiter.h \
	electronicComponents/Arbiters/PhotonicNoUturnArbiter.h \
	addon/HotSpot-5.0/temperature.h \
	statistics/StatObject.h \
	simCore/sim_includes.h \
	electronicComponents/Arbiters/ArbiterCanRoute.h \
	electronicComponents/Arbiters/ElectronicArbiter.h \
	electronicComponents/Arbiters/Arbiter_BT_4x4.h \
	processingPlane/addressTranslation/AddressTranslator.h \
	processingPlane/apps/Application.h \
	electronicComponents/orion/ORION_Array.h \
	processingPlane/Processor.h \
	addon/HotSpot-5.0/temperature_grid.h \
	electronicComponents/VC_Control.h
$O/electronicComponents/Arbiters/Arbiter_Crossbar.o: electronicComponents/Arbiters/Arbiter_Crossbar.cc \
	electronicComponents/orion/ORION_Array_Internal.h \
	electronicComponents/orion/ORION_Arbiter.h \
	electronicComponents/orion/ORION_Util.h \
	electronicComponents/orion/ORION_1_Params/SIM_power.h \
	simCore/messages_m.h \
	electronicComponents/VirtualChannelControl.h \
	electronicComponents/orion/ORION_Array_Info.h \
	electronicComponents/Arbiters/PhotonicArbiter.h \
	electronicComponents/orion/ORION_1_Params/SIM_power_test.h \
	statistics/StatObject.h \
	simCore/sim_includes.h \
	electronicComponents/orion/ORION_Config.h \
	electronicComponents/Arbiters/Arbiter.h \
	electronicComponents/Arbiters/ArbiterCanRoute.h \
	electronicComponents/Arbiters/ElectronicArbiter.h \
	electronicComponents/orion/ORION_1_Params/SIM_port.h \
	processingPlane/addressTranslation/AddressTranslator.h \
	simCore/NetworkAddress.h \
	electronicComponents/Arbiters/Arbiter_Crossbar.h \
	electronicComponents/orion/ORION_Array.h \
	electronicComponents/VC_Control.h
$O/electronicComponents/Arbiters/Arbiter_FT_GWC.o: electronicComponents/Arbiters/Arbiter_FT_GWC.cc \
	statistics/statistics.h \
	electronicComponents/orion/ORION_Array_Internal.h \
	simCore/messages_m.h \
	electronicComponents/VirtualChannelControl.h \
	addon/HotSpot-5.0/util.h \
	statistics/LogFile.h \
	electronicComponents/orion/ORION_1_Params/SIM_power_test.h \
	processingPlane/apps/SizeDistribution/SizeDistribution.h \
	electronicComponents/Arbiters/Arbiter.h \
	electronicComponents/orion/ORION_Config.h \
	processingPlane/DRAM_Cfg/DRAM_Cfg.h \
	simCore/NetworkAddress.h \
	electronicComponents/orion/ORION_1_Params/SIM_port.h \
	electronicComponents/Arbiters/Arbiter_FT_GWC.h \
	addon/HotSpot-5.0/flp.h \
	photonic/phylayer.h \
	addon/thermalmodel.h \
	electronicComponents/orion/ORION_Arbiter.h \
	electronicComponents/orion/ORION_Util.h \
	statistics/StatGroup.h \
	electronicComponents/orion/ORION_1_Params/SIM_power.h \
	electronicComponents/orion/ORION_Array_Info.h \
	electronicComponents/Arbiters/PhotonicArbiter.h \
	electronicComponents/Arbiters/PhotonicNoUturnArbiter.h \
	addon/HotSpot-5.0/temperature.h \
	statistics/StatObject.h \
	simCore/sim_includes.h \
	electronicComponents/Arbiters/ArbiterCanRoute.h \
	electronicComponents/Arbiters/ElectronicArbiter.h \
	processingPlane/addressTranslation/AddressTranslator.h \
	processingPlane/apps/Application.h \
	processingPlane/Processor.h \
	electronicComponents/orion/ORION_Array.h \
	addon/HotSpot-5.0/temperature_grid.h \
	electronicComponents/VC_Control.h
$O/electronicComponents/Arbiters/ElectronicArbiter.o: electronicComponents/Arbiters/ElectronicArbiter.cc \
	electronicComponents/orion/ORION_Array_Internal.h \
	electronicComponents/orion/ORION_Arbiter.h \
	electronicComponents/orion/ORION_Util.h \
	electronicComponents/orion/ORION_1_Params/SIM_power.h \
	simCore/messages_m.h \
	electronicComponents/VirtualChannelControl.h \
	electronicComponents/orion/ORION_Array_Info.h \
	electronicComponents/orion/ORION_1_Params/SIM_power_test.h \
	statistics/StatObject.h \
	simCore/sim_includes.h \
	electronicComponents/orion/ORION_Config.h \
	electronicComponents/Arbiters/Arbiter.h \
	electronicComponents/Arbiters/ArbiterCanRoute.h \
	electronicComponents/Arbiters/ElectronicArbiter.h \
	electronicComponents/orion/ORION_1_Params/SIM_port.h \
	simCore/NetworkAddress.h \
	processingPlane/addressTranslation/AddressTranslator.h \
	electronicComponents/orion/ORION_Array.h \
	electronicComponents/VC_Control.h
$O/electronicComponents/orion/ORION_Array_Internal.o: electronicComponents/orion/ORION_Array_Internal.cc \
	electronicComponents/orion/ORION_1_Params/SIM_power_test.h \
	electronicComponents/orion/ORION_Array_Internal.h \
	statistics/StatObject.h \
	simCore/sim_includes.h \
	electronicComponents/orion/ORION_Config.h \
	electronicComponents/orion/ORION_Util.h \
	electronicComponents/orion/ORION_1_Params/SIM_port.h \
	electronicComponents/orion/ORION_1_Params/SIM_power.h
$O/electronicComponents/orion/ORION_Arbiter.o: electronicComponents/orion/ORION_Arbiter.cc \
	electronicComponents/orion/ORION_Array_Internal.h \
	statistics/statistics.h \
	electronicComponents/orion/ORION_Arbiter.h \
	electronicComponents/orion/ORION_Util.h \
	statistics/StatGroup.h \
	electronicComponents/orion/ORION_1_Params/SIM_power.h \
	electronicComponents/orion/ORION_Array_Info.h \
	electronicComponents/orion/ORION_1_Params/SIM_power_test.h \
	statistics/LogFile.h \
	statistics/StatObject.h \
	electronicComponents/orion/ORION_Config.h \
	simCore/sim_includes.h \
	electronicComponents/orion/ORION_1_Params/SIM_port.h \
	electronicComponents/orion/ORION_Array.h \
	photonic/phylayer.h
$O/electronicComponents/orion/ORION_Link.o: electronicComponents/orion/ORION_Link.cc \
	electronicComponents/orion/ORION_1_Params/SIM_power_test.h \
	statistics/StatObject.h \
	simCore/sim_includes.h \
	electronicComponents/orion/ORION_Config.h \
	electronicComponents/orion/ORION_1_Params/SIM_port.h \
	electronicComponents/orion/ORION_1_Params/SIM_power.h \
	electronicComponents/orion/ORION_Link.h
$O/electronicComponents/orion/ORION_Array.o: electronicComponents/orion/ORION_Array.cc \
	electronicComponents/orion/ORION_1_Params/SIM_power_test.h \
	electronicComponents/orion/ORION_Array_Internal.h \
	statistics/StatObject.h \
	electronicComponents/orion/ORION_Config.h \
	simCore/sim_includes.h \
	electronicComponents/orion/ORION_Util.h \
	electronicComponents/orion/ORION_1_Params/SIM_port.h \
	electronicComponents/orion/ORION_1_Params/SIM_power.h \
	electronicComponents/orion/ORION_Array.h \
	electronicComponents/orion/ORION_Array_Info.h
$O/electronicComponents/orion/ORION_Crossbar.o: electronicComponents/orion/ORION_Crossbar.cc \
	electronicComponents/orion/ORION_1_Params/SIM_power_test.h \
	electronicComponents/orion/ORION_Crossbar.h \
	statistics/StatObject.h \
	simCore/sim_includes.h \
	electronicComponents/orion/ORION_Config.h \
	electronicComponents/orion/ORION_Util.h \
	electronicComponents/orion/ORION_1_Params/SIM_port.h \
	electronicComponents/orion/ORION_1_Params/SIM_power.h
$O/electronicComponents/orion/ORION_Util.o: electronicComponents/orion/ORION_Util.cc \
	electronicComponents/orion/ORION_1_Params/SIM_power_test.h \
	statistics/StatObject.h \
	electronicComponents/orion/ORION_Config.h \
	simCore/sim_includes.h \
	electronicComponents/orion/ORION_Util.h \
	electronicComponents/orion/ORION_1_Params/SIM_port.h \
	electronicComponents/orion/ORION_1_Params/SIM_power.h
$O/electronicComponents/orion/ORION_Config.o: electronicComponents/orion/ORION_Config.cc \
	electronicComponents/orion/ORION_1_Params/SIM_power_test.h \
	statistics/StatObject.h \
	simCore/sim_includes.h \
	electronicComponents/orion/ORION_Config.h \
	electronicComponents/orion/ORION_1_Params/SIM_port.h \
	electronicComponents/orion/ORION_1_Params/SIM_power.h
$O/electronicComponents/orion/ORION_Array_Info.o: electronicComponents/orion/ORION_Array_Info.cc \
	electronicComponents/orion/ORION_1_Params/SIM_power_test.h \
	statistics/StatObject.h \
	electronicComponents/orion/ORION_Config.h \
	simCore/sim_includes.h \
	electronicComponents/orion/ORION_1_Params/SIM_port.h \
	electronicComponents/orion/ORION_1_Params/SIM_power.h \
	electronicComponents/orion/ORION_Array_Info.h
$O/ioComponents/cDramModule.o: ioComponents/cDramModule.cc \
	ioComponents/DRAMsim/constants.h \
	ioComponents/DRAMsim/UpdateInterface.h \
	ioComponents/DRAMsim/PendingInfo.h \
	ioComponents/DRAMsim/BusEvent.h \
	ioComponents/cDramModule.h \
	ioComponents/DRAMsim/Global_TQ_info.h \
	processingPlane/apps/SizeDistribution/SizeDistribution.h \
	electronicComponents/orion/ORION_Config.h \
	electronicComponents/Arbiters/Arbiter.h \
	simCore/packetstat.h \
	ioComponents/DRAMsim/DIMM.h \
	ioComponents/DRAMsim/TransactionQueue.h \
	processingPlane/addressTranslation/AddressTranslator_FatTree.h \
	ioComponents/DRAMsim/Bus.h \
	photonic/phylayer.h \
	ioComponents/DRAMsim/PowerMeasure.h \
	ioComponents/DRAMsim/Bank.h \
	addon/thermalmodel.h \
	ioComponents/DRAMsim/Transaction.h \
	ioComponents/DRAMsim/RBRR.h \
	ioComponents/DRAMsim/DRAMaddress.h \
	electronicComponents/orion/ORION_Util.h \
	electronicComponents/orion/ORION_Arbiter.h \
	electronicComponents/orion/ORION_1_Params/SIM_power.h \
	ioComponents/DRAMsim/BIU.h \
	ioComponents/DRAMsim/DRAM_config.h \
	electronicComponents/orion/ORION_Array_Info.h \
	ioComponents/DRAMsim/PowerConfig.h \
	ioComponents/DRAMsim/RefreshQueue.h \
	addon/HotSpot-5.0/temperature.h \
	electronicComponents/Arbiters/ArbiterCanRoute.h \
	processingPlane/addressTranslation/AddressTranslator_Standard.h \
	ioComponents/DRAMsim/Aux_Stat.h \
	electronicComponents/orion/ORION_Array_Internal.h \
	statistics/statistics.h \
	ioComponents/DRAMsim/Command.h \
	simCore/messages_m.h \
	electronicComponents/VirtualChannelControl.h \
	addon/HotSpot-5.0/util.h \
	ioComponents/DRAMsim/PendingQueue.h \
	statistics/LogFile.h \
	electronicComponents/orion/ORION_1_Params/SIM_power_test.h \
	ioComponents/DRAMsim/Rank.h \
	processingPlane/DRAM_Cfg/DRAM_Cfg.h \
	ioComponents/DRAMsim/BIU_slot.h \
	simCore/NetworkAddress.h \
	electronicComponents/orion/ORION_1_Params/SIM_port.h \
	ioComponents/DRAMsim/PowerCounter.h \
	addon/HotSpot-5.0/flp.h \
	ioComponents/DRAMsim/DRAM.h \
	statistics/StatGroup.h \
	processingPlane/addressTranslation/AddressTranslator_SquareRoot.h \
	statistics/StatObject.h \
	simCore/sim_includes.h \
	processingPlane/addressTranslation/AddressTranslator.h \
	processingPlane/apps/Application.h \
	ioComponents/DRAMsim/DRAM_Sim.h \
	ioComponents/DRAMsim/Memory_Controller.h \
	electronicComponents/orion/ORION_Array.h \
	processingPlane/Processor.h \
	addon/HotSpot-5.0/temperature_grid.h \
	electronicComponents/VC_Control.h
$O/ioComponents/DRAM_LRL/OCM_Control.o: ioComponents/DRAM_LRL/OCM_Control.cc \
	statistics/statistics.h \
	statistics/StatGroup.h \
	electronicComponents/orion/ORION_1_Params/SIM_power.h \
	simCore/messages_m.h \
	electronicComponents/orion/ORION_1_Params/SIM_power_test.h \
	statistics/LogFile.h \
	statistics/StatObject.h \
	simCore/sim_includes.h \
	electronicComponents/orion/ORION_Config.h \
	ioComponents/DRAM_LRL/OCM_Control.h \
	processingPlane/addressTranslation/AddressTranslator.h \
	electronicComponents/orion/ORION_1_Params/SIM_port.h \
	simCore/NetworkAddress.h \
	photonic/phylayer.h
$O/ioComponents/DRAM_LRL/ProcessorDataStripper.o: ioComponents/DRAM_LRL/ProcessorDataStripper.cc \
	statistics/statistics.h \
	ioComponents/DRAM_LRL/ProcessorDataStripper.h \
	statistics/StatGroup.h \
	electronicComponents/orion/ORION_1_Params/SIM_power.h \
	simCore/messages_m.h \
	electronicComponents/orion/ORION_1_Params/SIM_power_test.h \
	statistics/LogFile.h \
	statistics/StatObject.h \
	simCore/sim_includes.h \
	electronicComponents/orion/ORION_Config.h \
	simCore/NetworkAddress.h \
	electronicComponents/orion/ORION_1_Params/SIM_port.h \
	photonic/phylayer.h
$O/ioComponents/DRAM_LRL/MemoryControl.o: ioComponents/DRAM_LRL/MemoryControl.cc \
	statistics/StatObject.h \
	simCore/sim_includes.h \
	simCore/messages_m.h \
	ioComponents/DRAM_LRL/MemoryControl.h
$O/ioComponents/DRAM_LRL/DRAM_Bank.o: ioComponents/DRAM_LRL/DRAM_Bank.cc \
	statistics/statistics.h \
	statistics/StatGroup.h \
	ioComponents/DRAM_LRL/DRAM_Bank.h \
	electronicComponents/orion/ORION_1_Params/SIM_power.h \
	simCore/messages_m.h \
	statistics/LogFile.h \
	electronicComponents/orion/ORION_1_Params/SIM_power_test.h \
	statistics/StatObject.h \
	electronicComponents/orion/ORION_Config.h \
	simCore/sim_includes.h \
	electronicComponents/orion/ORION_1_Params/SIM_port.h \
	simCore/NetworkAddress.h \
	photonic/phylayer.h
$O/ioComponents/DRAM_LRL/NIF_MemGW.o: ioComponents/DRAM_LRL/NIF_MemGW.cc \
	statistics/statistics.h \
	electronicComponents/orion/ORION_Array_Internal.h \
	simCore/messages_m.h \
	electronicComponents/VirtualChannelControl.h \
	processingPlane/interfaces/NIF_Packet_Credit.h \
	addon/HotSpot-5.0/util.h \
	statistics/LogFile.h \
	electronicComponents/orion/ORION_1_Params/SIM_power_test.h \
	processingPlane/apps/SizeDistribution/SizeDistribution.h \
	electronicComponents/orion/ORION_Config.h \
	electronicComponents/Arbiters/Arbiter.h \
	processingPlane/DRAM_Cfg/DRAM_Cfg.h \
	simCore/NetworkAddress.h \
	electronicComponents/orion/ORION_1_Params/SIM_port.h \
	addon/HotSpot-5.0/flp.h \
	photonic/phylayer.h \
	addon/thermalmodel.h \
	electronicComponents/orion/ORION_Arbiter.h \
	electronicComponents/orion/ORION_Util.h \
	statistics/StatGroup.h \
	electronicComponents/orion/ORION_1_Params/SIM_power.h \
	electronicComponents/orion/ORION_Array_Info.h \
	processingPlane/interfaces/NIF.h \
	addon/HotSpot-5.0/temperature.h \
	statistics/StatObject.h \
	simCore/sim_includes.h \
	processingPlane/interfaces/NIF_Circuit.h \
	electronicComponents/Arbiters/ArbiterCanRoute.h \
	processingPlane/addressTranslation/AddressTranslator.h \
	processingPlane/apps/Application.h \
	electronicComponents/orion/ORION_Array.h \
	processingPlane/Processor.h \
	addon/HotSpot-5.0/temperature_grid.h \
	electronicComponents/VC_Control.h \
	ioComponents/DRAM_LRL/NIF_MemGW.h
$O/ioComponents/DRAMsim/DRAM_config.o: ioComponents/DRAMsim/DRAM_config.cc \
	ioComponents/DRAMsim/constants.h \
	ioComponents/DRAMsim/BIU_slot.h \
	ioComponents/DRAMsim/DRAMaddress.h \
	ioComponents/DRAMsim/UpdateInterface.h \
	ioComponents/DRAMsim/TransactionQueue.h \
	ioComponents/DRAMsim/Command.h \
	ioComponents/DRAMsim/BIU.h \
	ioComponents/DRAMsim/Global_TQ_info.h \
	ioComponents/DRAMsim/DRAM_config.h \
	ioComponents/DRAMsim/Bank.h \
	ioComponents/DRAMsim/Aux_Stat.h \
	ioComponents/DRAMsim/Transaction.h
$O/ioComponents/DRAMsim/Aux_Stat.o: ioComponents/DRAMsim/Aux_Stat.cc \
	ioComponents/DRAMsim/constants.h \
	ioComponents/DRAMsim/UpdateInterface.h \
	ioComponents/DRAMsim/Command.h \
	ioComponents/DRAMsim/Global_TQ_info.h \
	ioComponents/DRAMsim/DRAM_config.h \
	ioComponents/DRAMsim/Aux_Stat.h
$O/ioComponents/DRAMsim/Command.o: ioComponents/DRAMsim/Command.cc \
	ioComponents/DRAMsim/constants.h \
	ioComponents/DRAMsim/Command.h
$O/ioComponents/DRAMsim/DIMM.o: ioComponents/DRAMsim/DIMM.cc \
	ioComponents/DRAMsim/constants.h \
	ioComponents/DRAMsim/DIMM.h \
	ioComponents/DRAMsim/UpdateInterface.h \
	ioComponents/DRAMsim/Command.h \
	ioComponents/DRAMsim/Global_TQ_info.h \
	ioComponents/DRAMsim/DRAM_config.h
$O/ioComponents/DRAMsim/DRAM_Sim.o: ioComponents/DRAMsim/DRAM_Sim.cc \
	ioComponents/DRAMsim/constants.h \
	ioComponents/DRAMsim/UpdateInterface.h \
	ioComponents/DRAMsim/PendingInfo.h \
	ioComponents/DRAMsim/BusEvent.h \
	ioComponents/DRAMsim/Command.h \
	ioComponents/DRAMsim/Global_TQ_info.h \
	ioComponents/DRAMsim/PendingQueue.h \
	ioComponents/DRAMsim/Rank.h \
	ioComponents/DRAMsim/BIU_slot.h \
	ioComponents/DRAMsim/DIMM.h \
	ioComponents/DRAMsim/TransactionQueue.h \
	ioComponents/DRAMsim/PowerCounter.h \
	ioComponents/DRAMsim/DRAM.h \
	ioComponents/DRAMsim/Bus.h \
	ioComponents/DRAMsim/PowerMeasure.h \
	ioComponents/DRAMsim/Bank.h \
	ioComponents/DRAMsim/Transaction.h \
	ioComponents/DRAMsim/RBRR.h \
	ioComponents/DRAMsim/DRAMaddress.h \
	ioComponents/DRAMsim/BIU.h \
	ioComponents/DRAMsim/DRAM_config.h \
	ioComponents/DRAMsim/PowerConfig.h \
	ioComponents/DRAMsim/RefreshQueue.h \
	ioComponents/DRAMsim/Memory_Controller.h \
	ioComponents/DRAMsim/DRAM_Sim.h \
	ioComponents/DRAMsim/Aux_Stat.h
$O/ioComponents/DRAMsim/Transaction.o: ioComponents/DRAMsim/Transaction.cc \
	ioComponents/DRAMsim/constants.h \
	ioComponents/DRAMsim/DRAMaddress.h \
	ioComponents/DRAMsim/UpdateInterface.h \
	ioComponents/DRAMsim/Command.h \
	ioComponents/DRAMsim/Global_TQ_info.h \
	ioComponents/DRAMsim/DRAM_config.h \
	ioComponents/DRAMsim/Transaction.h
$O/ioComponents/DRAMsim/RefreshQueue.o: ioComponents/DRAMsim/RefreshQueue.cc \
	ioComponents/DRAMsim/constants.h \
	ioComponents/DRAMsim/Command.h \
	ioComponents/DRAMsim/RefreshQueue.h
$O/ioComponents/DRAMsim/DRAM.o: ioComponents/DRAMsim/DRAM.cc \
	ioComponents/DRAMsim/constants.h \
	ioComponents/DRAMsim/UpdateInterface.h \
	ioComponents/DRAMsim/PendingInfo.h \
	ioComponents/DRAMsim/utilities.h \
	ioComponents/DRAMsim/Command.h \
	ioComponents/DRAMsim/Global_TQ_info.h \
	ioComponents/DRAMsim/PendingQueue.h \
	ioComponents/DRAMsim/Rank.h \
	ioComponents/DRAMsim/BIU_slot.h \
	ioComponents/DRAMsim/DIMM.h \
	ioComponents/DRAMsim/TransactionQueue.h \
	ioComponents/DRAMsim/PowerCounter.h \
	ioComponents/DRAMsim/DRAM.h \
	ioComponents/DRAMsim/Bus.h \
	ioComponents/DRAMsim/PowerMeasure.h \
	ioComponents/DRAMsim/Bank.h \
	ioComponents/DRAMsim/Transaction.h \
	ioComponents/DRAMsim/RBRR.h \
	ioComponents/DRAMsim/DRAMaddress.h \
	ioComponents/DRAMsim/BIU.h \
	ioComponents/DRAMsim/DRAM_config.h \
	ioComponents/DRAMsim/PowerConfig.h \
	ioComponents/DRAMsim/RefreshQueue.h \
	ioComponents/DRAMsim/Memory_Controller.h \
	ioComponents/DRAMsim/Aux_Stat.h
$O/ioComponents/DRAMsim/PrefetchCache.o: ioComponents/DRAMsim/PrefetchCache.cc \
	ioComponents/DRAMsim/constants.h \
	ioComponents/DRAMsim/PrefetchCache.h
$O/ioComponents/DRAMsim/PowerConfig.o: ioComponents/DRAMsim/PowerConfig.cc \
	ioComponents/DRAMsim/constants.h \
	ioComponents/DRAMsim/RBRR.h \
	ioComponents/DRAMsim/DRAMaddress.h \
	ioComponents/DRAMsim/UpdateInterface.h \
	ioComponents/DRAMsim/PendingInfo.h \
	ioComponents/DRAMsim/Command.h \
	ioComponents/DRAMsim/BIU.h \
	ioComponents/DRAMsim/Global_TQ_info.h \
	ioComponents/DRAMsim/DRAM_config.h \
	ioComponents/DRAMsim/PowerConfig.h \
	ioComponents/DRAMsim/RefreshQueue.h \
	ioComponents/DRAMsim/PendingQueue.h \
	ioComponents/DRAMsim/Rank.h \
	ioComponents/DRAMsim/BIU_slot.h \
	ioComponents/DRAMsim/DIMM.h \
	ioComponents/DRAMsim/TransactionQueue.h \
	ioComponents/DRAMsim/Memory_Controller.h \
	ioComponents/DRAMsim/PowerCounter.h \
	ioComponents/DRAMsim/Bus.h \
	ioComponents/DRAMsim/PowerMeasure.h \
	ioComponents/DRAMsim/Bank.h \
	ioComponents/DRAMsim/Transaction.h \
	ioComponents/DRAMsim/Aux_Stat.h
$O/ioComponents/DRAMsim/PendingQueue.o: ioComponents/DRAMsim/PendingQueue.cc \
	ioComponents/DRAMsim/constants.h \
	ioComponents/DRAMsim/PendingInfo.h \
	ioComponents/DRAMsim/PendingQueue.h
$O/ioComponents/DRAMsim/BIU.o: ioComponents/DRAMsim/BIU.cc \
	ioComponents/DRAMsim/constants.h \
	ioComponents/DRAMsim/BIU_slot.h \
	ioComponents/DRAMsim/DRAMaddress.h \
	ioComponents/DRAMsim/UpdateInterface.h \
	ioComponents/DRAMsim/Command.h \
	ioComponents/DRAMsim/BIU.h \
	ioComponents/DRAMsim/Global_TQ_info.h \
	ioComponents/DRAMsim/DRAM_config.h \
	ioComponents/DRAMsim/Aux_Stat.h
$O/ioComponents/DRAMsim/utilities.o: ioComponents/DRAMsim/utilities.cc \
	ioComponents/DRAMsim/constants.h \
	ioComponents/DRAMsim/utilities.h \
	ioComponents/DRAMsim/Command.h
$O/ioComponents/DRAMsim/RBRR.o: ioComponents/DRAMsim/RBRR.cc \
	ioComponents/DRAMsim/constants.h \
	ioComponents/DRAMsim/RBRR.h
$O/ioComponents/DRAMsim/DRAMaddress.o: ioComponents/DRAMsim/DRAMaddress.cc \
	ioComponents/DRAMsim/constants.h \
	ioComponents/DRAMsim/DRAMaddress.h \
	ioComponents/DRAMsim/UpdateInterface.h \
	ioComponents/DRAMsim/Command.h \
	ioComponents/DRAMsim/Global_TQ_info.h \
	ioComponents/DRAMsim/DRAM_config.h
$O/ioComponents/DRAMsim/Global_TQ_info.o: ioComponents/DRAMsim/Global_TQ_info.cc \
	ioComponents/DRAMsim/constants.h \
	ioComponents/DRAMsim/Command.h \
	ioComponents/DRAMsim/Global_TQ_info.h
$O/ioComponents/DRAMsim/Memory_Controller.o: ioComponents/DRAMsim/Memory_Controller.cc \
	ioComponents/DRAMsim/constants.h \
	ioComponents/DRAMsim/UpdateInterface.h \
	ioComponents/DRAMsim/PendingInfo.h \
	ioComponents/DRAMsim/utilities.h \
	ioComponents/DRAMsim/Command.h \
	ioComponents/DRAMsim/Global_TQ_info.h \
	ioComponents/DRAMsim/PendingQueue.h \
	ioComponents/DRAMsim/Rank.h \
	ioComponents/DRAMsim/BIU_slot.h \
	ioComponents/DRAMsim/DIMM.h \
	ioComponents/DRAMsim/TransactionQueue.h \
	ioComponents/DRAMsim/PowerCounter.h \
	ioComponents/DRAMsim/DRAM.h \
	ioComponents/DRAMsim/Bus.h \
	ioComponents/DRAMsim/PowerMeasure.h \
	ioComponents/DRAMsim/Bank.h \
	ioComponents/DRAMsim/Transaction.h \
	ioComponents/DRAMsim/RBRR.h \
	ioComponents/DRAMsim/DRAMaddress.h \
	ioComponents/DRAMsim/BIU.h \
	ioComponents/DRAMsim/DRAM_config.h \
	ioComponents/DRAMsim/PowerConfig.h \
	ioComponents/DRAMsim/RefreshQueue.h \
	ioComponents/DRAMsim/Memory_Controller.h \
	ioComponents/DRAMsim/Aux_Stat.h
$O/ioComponents/DRAMsim/TransactionQueue.o: ioComponents/DRAMsim/TransactionQueue.cc \
	ioComponents/DRAMsim/constants.h \
	ioComponents/DRAMsim/BIU_slot.h \
	ioComponents/DRAMsim/DRAMaddress.h \
	ioComponents/DRAMsim/UpdateInterface.h \
	ioComponents/DRAMsim/TransactionQueue.h \
	ioComponents/DRAMsim/Command.h \
	ioComponents/DRAMsim/BIU.h \
	ioComponents/DRAMsim/Global_TQ_info.h \
	ioComponents/DRAMsim/DRAM_config.h \
	ioComponents/DRAMsim/Bank.h \
	ioComponents/DRAMsim/Aux_Stat.h \
	ioComponents/DRAMsim/Transaction.h
$O/photonic/modulatorelement.o: photonic/modulatorelement.cc \
	statistics/statistics.h \
	photonic/modulatorelement.h \
	statistics/StatGroup.h \
	electronicComponents/orion/ORION_1_Params/SIM_power.h \
	simCore/messages_m.h \
	addon/HotSpot-5.0/util.h \
	photonic/basicelement.h \
	electronicComponents/orion/ORION_1_Params/SIM_power_test.h \
	statistics/LogFile.h \
	addon/HotSpot-5.0/temperature.h \
	statistics/StatObject.h \
	electronicComponents/orion/ORION_Config.h \
	simCore/sim_includes.h \
	simCore/packetstat.h \
	electronicComponents/orion/ORION_1_Params/SIM_port.h \
	addon/HotSpot-5.0/flp.h \
	photonic/phylayer.h \
	photonic/ringelement.h \
	addon/HotSpot-5.0/temperature_grid.h \
	addon/thermalmodel.h
$O/photonic/ringelement.o: photonic/ringelement.cc \
	statistics/statistics.h \
	statistics/StatGroup.h \
	electronicComponents/orion/ORION_1_Params/SIM_power.h \
	simCore/messages_m.h \
	addon/HotSpot-5.0/util.h \
	photonic/basicelement.h \
	electronicComponents/orion/ORION_1_Params/SIM_power_test.h \
	statistics/LogFile.h \
	addon/HotSpot-5.0/temperature.h \
	statistics/StatObject.h \
	electronicComponents/orion/ORION_Config.h \
	simCore/sim_includes.h \
	simCore/packetstat.h \
	electronicComponents/orion/ORION_1_Params/SIM_port.h \
	addon/HotSpot-5.0/flp.h \
	photonic/phylayer.h \
	photonic/ringelement.h \
	addon/HotSpot-5.0/temperature_grid.h \
	addon/thermalmodel.h
$O/photonic/phylayer.o: photonic/phylayer.cc \
	photonic/phylayer.h
$O/photonic/machzehnderelement.o: photonic/machzehnderelement.cc \
	photonic/machzehnderelement.h
$O/photonic/activetworingelement.o: photonic/activetworingelement.cc \
	statistics/statistics.h \
	statistics/StatGroup.h \
	electronicComponents/orion/ORION_1_Params/SIM_power.h \
	simCore/messages_m.h \
	photonic/activetworingelement.h \
	addon/HotSpot-5.0/util.h \
	photonic/basicelement.h \
	electronicComponents/orion/ORION_1_Params/SIM_power_test.h \
	statistics/LogFile.h \
	addon/HotSpot-5.0/temperature.h \
	statistics/StatObject.h \
	simCore/sim_includes.h \
	electronicComponents/orion/ORION_Config.h \
	simCore/packetstat.h \
	photonic/activeelement.h \
	electronicComponents/orion/ORION_1_Params/SIM_port.h \
	photonic/tworingelement.h \
	addon/HotSpot-5.0/flp.h \
	photonic/ringelement.h \
	photonic/phylayer.h \
	addon/HotSpot-5.0/temperature_grid.h \
	addon/thermalmodel.h
$O/photonic/domainchanger.o: photonic/domainchanger.cc \
	simCore/messages_m.h \
	photonic/domainchanger.h
$O/photonic/activeringelement.o: photonic/activeringelement.cc \
	statistics/statistics.h \
	statistics/StatGroup.h \
	electronicComponents/orion/ORION_1_Params/SIM_power.h \
	photonic/activeringelement.h \
	simCore/messages_m.h \
	addon/HotSpot-5.0/util.h \
	photonic/basicelement.h \
	electronicComponents/orion/ORION_1_Params/SIM_power_test.h \
	statistics/LogFile.h \
	addon/HotSpot-5.0/temperature.h \
	statistics/StatObject.h \
	simCore/sim_includes.h \
	electronicComponents/orion/ORION_Config.h \
	simCore/packetstat.h \
	photonic/activeelement.h \
	electronicComponents/orion/ORION_1_Params/SIM_port.h \
	addon/HotSpot-5.0/flp.h \
	photonic/phylayer.h \
	photonic/ringelement.h \
	addon/HotSpot-5.0/temperature_grid.h \
	addon/thermalmodel.h
$O/photonic/activemultiringelement.o: photonic/activemultiringelement.cc \
	statistics/statistics.h \
	photonic/activemultiringelement.h \
	statistics/StatGroup.h \
	electronicComponents/orion/ORION_1_Params/SIM_power.h \
	simCore/messages_m.h \
	photonic/multiringelement.h \
	addon/HotSpot-5.0/util.h \
	photonic/basicelement.h \
	electronicComponents/orion/ORION_1_Params/SIM_power_test.h \
	statistics/LogFile.h \
	addon/HotSpot-5.0/temperature.h \
	statistics/StatObject.h \
	simCore/sim_includes.h \
	electronicComponents/orion/ORION_Config.h \
	simCore/packetstat.h \
	photonic/activeelement.h \
	electronicComponents/orion/ORION_1_Params/SIM_port.h \
	addon/HotSpot-5.0/flp.h \
	photonic/phylayer.h \
	addon/HotSpot-5.0/temperature_grid.h \
	addon/thermalmodel.h
$O/photonic/activeelement.o: photonic/activeelement.cc \
	statistics/statistics.h \
	statistics/StatGroup.h \
	electronicComponents/orion/ORION_1_Params/SIM_power.h \
	simCore/messages_m.h \
	addon/HotSpot-5.0/util.h \
	photonic/basicelement.h \
	electronicComponents/orion/ORION_1_Params/SIM_power_test.h \
	statistics/LogFile.h \
	addon/HotSpot-5.0/temperature.h \
	statistics/StatObject.h \
	electronicComponents/orion/ORION_Config.h \
	simCore/sim_includes.h \
	simCore/packetstat.h \
	photonic/activeelement.h \
	electronicComponents/orion/ORION_1_Params/SIM_port.h \
	addon/HotSpot-5.0/flp.h \
	photonic/phylayer.h \
	addon/HotSpot-5.0/temperature_grid.h \
	addon/thermalmodel.h
$O/photonic/multiringelement.o: photonic/multiringelement.cc \
	statistics/statistics.h \
	statistics/StatGroup.h \
	electronicComponents/orion/ORION_1_Params/SIM_power.h \
	simCore/messages_m.h \
	photonic/multiringelement.h \
	addon/HotSpot-5.0/util.h \
	photonic/basicelement.h \
	electronicComponents/orion/ORION_1_Params/SIM_power_test.h \
	statistics/LogFile.h \
	addon/HotSpot-5.0/temperature.h \
	statistics/StatObject.h \
	electronicComponents/orion/ORION_Config.h \
	simCore/sim_includes.h \
	simCore/packetstat.h \
	electronicComponents/orion/ORION_1_Params/SIM_port.h \
	addon/HotSpot-5.0/flp.h \
	photonic/phylayer.h \
	addon/HotSpot-5.0/temperature_grid.h \
	addon/thermalmodel.h
$O/photonic/detectorelement.o: photonic/detectorelement.cc \
	statistics/statistics.h \
	photonic/detectorelement.h \
	statistics/StatGroup.h \
	electronicComponents/orion/ORION_1_Params/SIM_power.h \
	simCore/messages_m.h \
	addon/HotSpot-5.0/util.h \
	photonic/basicelement.h \
	electronicComponents/orion/ORION_1_Params/SIM_power_test.h \
	statistics/LogFile.h \
	addon/HotSpot-5.0/temperature.h \
	statistics/StatObject.h \
	electronicComponents/orion/ORION_Config.h \
	simCore/sim_includes.h \
	simCore/packetstat.h \
	electronicComponents/orion/ORION_1_Params/SIM_port.h \
	addon/HotSpot-5.0/flp.h \
	photonic/phylayer.h \
	photonic/ringelement.h \
	addon/HotSpot-5.0/temperature_grid.h \
	addon/thermalmodel.h
$O/photonic/basicelement.o: photonic/basicelement.cc \
	statistics/statistics.h \
	statistics/StatGroup.h \
	electronicComponents/orion/ORION_1_Params/SIM_power.h \
	simCore/messages_m.h \
	addon/HotSpot-5.0/util.h \
	photonic/basicelement.h \
	electronicComponents/orion/ORION_1_Params/SIM_power_test.h \
	statistics/LogFile.h \
	addon/HotSpot-5.0/temperature.h \
	statistics/StatObject.h \
	simCore/sim_includes.h \
	electronicComponents/orion/ORION_Config.h \
	simCore/packetstat.h \
	electronicComponents/orion/ORION_1_Params/SIM_port.h \
	addon/HotSpot-5.0/flp.h \
	photonic/phylayer.h \
	addon/HotSpot-5.0/temperature_grid.h \
	addon/thermalmodel.h
$O/photonic/resonanceprofile.o: photonic/resonanceprofile.cc \
	photonic/resonanceprofile.h
$O/photonic/tworingelement.o: photonic/tworingelement.cc \
	statistics/statistics.h \
	statistics/StatGroup.h \
	electronicComponents/orion/ORION_1_Params/SIM_power.h \
	simCore/messages_m.h \
	addon/HotSpot-5.0/util.h \
	electronicComponents/orion/ORION_1_Params/SIM_power_test.h \
	statistics/LogFile.h \
	photonic/basicelement.h \
	addon/HotSpot-5.0/temperature.h \
	statistics/StatObject.h \
	electronicComponents/orion/ORION_Config.h \
	simCore/sim_includes.h \
	simCore/packetstat.h \
	electronicComponents/orion/ORION_1_Params/SIM_port.h \
	photonic/tworingelement.h \
	addon/HotSpot-5.0/flp.h \
	photonic/phylayer.h \
	photonic/ringelement.h \
	addon/HotSpot-5.0/temperature_grid.h \
	addon/thermalmodel.h
$O/photonic/devices/pse1x2.o: photonic/devices/pse1x2.cc \
	statistics/statistics.h \
	statistics/StatGroup.h \
	electronicComponents/orion/ORION_1_Params/SIM_power.h \
	photonic/activeringelement.h \
	simCore/messages_m.h \
	addon/HotSpot-5.0/util.h \
	photonic/basicelement.h \
	electronicComponents/orion/ORION_1_Params/SIM_power_test.h \
	statistics/LogFile.h \
	addon/HotSpot-5.0/temperature.h \
	statistics/StatObject.h \
	simCore/sim_includes.h \
	electronicComponents/orion/ORION_Config.h \
	simCore/packetstat.h \
	photonic/activeelement.h \
	electronicComponents/orion/ORION_1_Params/SIM_port.h \
	addon/HotSpot-5.0/flp.h \
	photonic/ringelement.h \
	photonic/phylayer.h \
	addon/HotSpot-5.0/temperature_grid.h \
	addon/thermalmodel.h \
	photonic/devices/pse1x2.h
$O/photonic/devices/activedirectionalcoupler.o: photonic/devices/activedirectionalcoupler.cc \
	statistics/statistics.h \
	statistics/StatGroup.h \
	electronicComponents/orion/ORION_1_Params/SIM_power.h \
	simCore/messages_m.h \
	photonic/devices/activedirectionalcoupler.h \
	addon/HotSpot-5.0/util.h \
	photonic/basicelement.h \
	electronicComponents/orion/ORION_1_Params/SIM_power_test.h \
	statistics/LogFile.h \
	addon/HotSpot-5.0/temperature.h \
	statistics/StatObject.h \
	electronicComponents/orion/ORION_Config.h \
	simCore/sim_includes.h \
	simCore/packetstat.h \
	photonic/activeelement.h \
	electronicComponents/orion/ORION_1_Params/SIM_port.h \
	addon/HotSpot-5.0/flp.h \
	photonic/phylayer.h \
	addon/HotSpot-5.0/temperature_grid.h \
	addon/thermalmodel.h
$O/photonic/devices/pse2x2.o: photonic/devices/pse2x2.cc \
	statistics/statistics.h \
	statistics/StatGroup.h \
	electronicComponents/orion/ORION_1_Params/SIM_power.h \
	photonic/activeringelement.h \
	simCore/messages_m.h \
	photonic/devices/pse2x2.h \
	addon/HotSpot-5.0/util.h \
	photonic/basicelement.h \
	electronicComponents/orion/ORION_1_Params/SIM_power_test.h \
	statistics/LogFile.h \
	addon/HotSpot-5.0/temperature.h \
	statistics/StatObject.h \
	simCore/sim_includes.h \
	electronicComponents/orion/ORION_Config.h \
	simCore/packetstat.h \
	photonic/activeelement.h \
	electronicComponents/orion/ORION_1_Params/SIM_port.h \
	addon/HotSpot-5.0/flp.h \
	photonic/ringelement.h \
	photonic/phylayer.h \
	addon/HotSpot-5.0/temperature_grid.h \
	addon/thermalmodel.h
$O/photonic/devices/modulator.o: photonic/devices/modulator.cc \
	statistics/statistics.h \
	photonic/devices/modulator.h \
	photonic/modulatorelement.h \
	statistics/StatGroup.h \
	electronicComponents/orion/ORION_1_Params/SIM_power.h \
	simCore/messages_m.h \
	addon/HotSpot-5.0/util.h \
	photonic/basicelement.h \
	electronicComponents/orion/ORION_1_Params/SIM_power_test.h \
	statistics/LogFile.h \
	addon/HotSpot-5.0/temperature.h \
	statistics/StatObject.h \
	simCore/sim_includes.h \
	electronicComponents/orion/ORION_Config.h \
	simCore/packetstat.h \
	electronicComponents/orion/ORION_1_Params/SIM_port.h \
	simCore/convert.h \
	addon/HotSpot-5.0/flp.h \
	photonic/phylayer.h \
	photonic/ringelement.h \
	addon/HotSpot-5.0/temperature_grid.h \
	addon/thermalmodel.h
$O/photonic/devices/attenuator.o: photonic/devices/attenuator.cc \
	statistics/StatObject.h \
	simCore/sim_includes.h \
	photonic/devices/attenuator.h \
	simCore/messages_m.h
$O/photonic/devices/bend.o: photonic/devices/bend.cc \
	statistics/statistics.h \
	statistics/StatGroup.h \
	photonic/devices/bend.h \
	electronicComponents/orion/ORION_1_Params/SIM_power.h \
	simCore/messages_m.h \
	addon/HotSpot-5.0/util.h \
	photonic/basicelement.h \
	electronicComponents/orion/ORION_1_Params/SIM_power_test.h \
	statistics/LogFile.h \
	addon/HotSpot-5.0/temperature.h \
	statistics/StatObject.h \
	electronicComponents/orion/ORION_Config.h \
	simCore/sim_includes.h \
	simCore/packetstat.h \
	electronicComponents/orion/ORION_1_Params/SIM_port.h \
	addon/HotSpot-5.0/flp.h \
	photonic/phylayer.h \
	addon/HotSpot-5.0/temperature_grid.h \
	addon/thermalmodel.h
$O/photonic/devices/cross.o: photonic/devices/cross.cc \
	statistics/statistics.h \
	statistics/StatGroup.h \
	electronicComponents/orion/ORION_1_Params/SIM_power.h \
	photonic/devices/cross.h \
	simCore/messages_m.h \
	addon/HotSpot-5.0/util.h \
	statistics/LogFile.h \
	photonic/basicelement.h \
	electronicComponents/orion/ORION_1_Params/SIM_power_test.h \
	addon/HotSpot-5.0/temperature.h \
	statistics/StatObject.h \
	electronicComponents/orion/ORION_Config.h \
	simCore/sim_includes.h \
	simCore/packetstat.h \
	electronicComponents/orion/ORION_1_Params/SIM_port.h \
	addon/HotSpot-5.0/flp.h \
	photonic/phylayer.h \
	addon/HotSpot-5.0/temperature_grid.h \
	addon/thermalmodel.h
$O/photonic/devices/line.o: photonic/devices/line.cc \
	statistics/statistics.h \
	statistics/StatGroup.h \
	electronicComponents/orion/ORION_1_Params/SIM_power.h \
	simCore/messages_m.h \
	addon/HotSpot-5.0/util.h \
	statistics/LogFile.h \
	photonic/basicelement.h \
	electronicComponents/orion/ORION_1_Params/SIM_power_test.h \
	addon/HotSpot-5.0/temperature.h \
	statistics/StatObject.h \
	electronicComponents/orion/ORION_Config.h \
	simCore/sim_includes.h \
	simCore/packetstat.h \
	photonic/devices/line.h \
	electronicComponents/orion/ORION_1_Params/SIM_port.h \
	addon/HotSpot-5.0/flp.h \
	photonic/phylayer.h \
	addon/HotSpot-5.0/temperature_grid.h \
	addon/thermalmodel.h
$O/photonic/devices/end.o: photonic/devices/end.cc \
	statistics/StatObject.h \
	simCore/sim_includes.h \
	simCore/packetstat.h \
	simCore/messages_m.h \
	photonic/devices/end.h
$O/photonic/devices/laser.o: photonic/devices/laser.cc \
	statistics/StatObject.h \
	simCore/sim_includes.h \
	photonic/devices/laser.h \
	simCore/packetstat.h \
	simCore/messages_m.h
$O/photonic/devices/detector.o: photonic/devices/detector.cc \
	statistics/statistics.h \
	photonic/detectorelement.h \
	statistics/StatGroup.h \
	electronicComponents/orion/ORION_1_Params/SIM_power.h \
	simCore/messages_m.h \
	addon/HotSpot-5.0/util.h \
	photonic/basicelement.h \
	photonic/devices/detector.h \
	electronicComponents/orion/ORION_1_Params/SIM_power_test.h \
	statistics/LogFile.h \
	addon/HotSpot-5.0/temperature.h \
	statistics/StatObject.h \
	simCore/sim_includes.h \
	electronicComponents/orion/ORION_Config.h \
	simCore/packetstat.h \
	electronicComponents/orion/ORION_1_Params/SIM_port.h \
	simCore/convert.h \
	addon/HotSpot-5.0/flp.h \
	photonic/ringelement.h \
	photonic/phylayer.h \
	addon/HotSpot-5.0/temperature_grid.h \
	addon/thermalmodel.h
$O/photonic/devices/facet.o: photonic/devices/facet.cc \
	statistics/statistics.h \
	statistics/StatGroup.h \
	electronicComponents/orion/ORION_1_Params/SIM_power.h \
	simCore/messages_m.h \
	addon/HotSpot-5.0/util.h \
	photonic/devices/facet.h \
	photonic/basicelement.h \
	electronicComponents/orion/ORION_1_Params/SIM_power_test.h \
	statistics/LogFile.h \
	addon/HotSpot-5.0/temperature.h \
	statistics/StatObject.h \
	electronicComponents/orion/ORION_Config.h \
	simCore/sim_includes.h \
	simCore/packetstat.h \
	electronicComponents/orion/ORION_1_Params/SIM_port.h \
	addon/HotSpot-5.0/flp.h \
	photonic/phylayer.h \
	addon/HotSpot-5.0/temperature_grid.h \
	addon/thermalmodel.h
$O/photonic/devices/SwitchingFilter.o: photonic/devices/SwitchingFilter.cc \
	statistics/statistics.h \
	statistics/StatGroup.h \
	electronicComponents/orion/ORION_1_Params/SIM_power.h \
	photonic/activeringelement.h \
	simCore/messages_m.h \
	addon/HotSpot-5.0/util.h \
	statistics/LogFile.h \
	photonic/basicelement.h \
	electronicComponents/orion/ORION_1_Params/SIM_power_test.h \
	addon/HotSpot-5.0/temperature.h \
	statistics/StatObject.h \
	electronicComponents/orion/ORION_Config.h \
	simCore/sim_includes.h \
	photonic/devices/SwitchingFilter.h \
	simCore/packetstat.h \
	photonic/activeelement.h \
	electronicComponents/orion/ORION_1_Params/SIM_port.h \
	addon/HotSpot-5.0/flp.h \
	photonic/phylayer.h \
	photonic/ringelement.h \
	addon/HotSpot-5.0/temperature_grid.h \
	addon/thermalmodel.h
$O/photonic/devices/pse1x2nx.o: photonic/devices/pse1x2nx.cc \
	statistics/statistics.h \
	statistics/StatGroup.h \
	electronicComponents/orion/ORION_1_Params/SIM_power.h \
	photonic/devices/pse1x2nx.h \
	photonic/activeringelement.h \
	simCore/messages_m.h \
	addon/HotSpot-5.0/util.h \
	statistics/LogFile.h \
	electronicComponents/orion/ORION_1_Params/SIM_power_test.h \
	photonic/basicelement.h \
	addon/HotSpot-5.0/temperature.h \
	statistics/StatObject.h \
	electronicComponents/orion/ORION_Config.h \
	simCore/sim_includes.h \
	simCore/packetstat.h \
	photonic/activeelement.h \
	electronicComponents/orion/ORION_1_Params/SIM_port.h \
	addon/HotSpot-5.0/flp.h \
	photonic/phylayer.h \
	photonic/ringelement.h \
	addon/HotSpot-5.0/temperature_grid.h \
	addon/thermalmodel.h
$O/photonic/devices/join.o: photonic/devices/join.cc \
	statistics/statistics.h \
	statistics/StatGroup.h \
	photonic/devices/join.h \
	electronicComponents/orion/ORION_1_Params/SIM_power.h \
	simCore/messages_m.h \
	addon/HotSpot-5.0/util.h \
	statistics/LogFile.h \
	photonic/basicelement.h \
	electronicComponents/orion/ORION_1_Params/SIM_power_test.h \
	addon/HotSpot-5.0/temperature.h \
	statistics/StatObject.h \
	electronicComponents/orion/ORION_Config.h \
	simCore/sim_includes.h \
	simCore/packetstat.h \
	electronicComponents/orion/ORION_1_Params/SIM_port.h \
	addon/HotSpot-5.0/flp.h \
	photonic/phylayer.h \
	addon/HotSpot-5.0/temperature_grid.h \
	addon/thermalmodel.h
$O/photonic/devices/StarCoupler.o: photonic/devices/StarCoupler.cc \
	photonic/devices/StarCoupler.h
$O/photonic/devices/filter1x2.o: photonic/devices/filter1x2.cc \
	statistics/statistics.h \
	statistics/StatGroup.h \
	electronicComponents/orion/ORION_1_Params/SIM_power.h \
	simCore/messages_m.h \
	addon/HotSpot-5.0/util.h \
	photonic/devices/filter1x2.h \
	photonic/basicelement.h \
	electronicComponents/orion/ORION_1_Params/SIM_power_test.h \
	statistics/LogFile.h \
	addon/HotSpot-5.0/temperature.h \
	statistics/StatObject.h \
	simCore/sim_includes.h \
	electronicComponents/orion/ORION_Config.h \
	simCore/packetstat.h \
	electronicComponents/orion/ORION_1_Params/SIM_port.h \
	addon/HotSpot-5.0/flp.h \
	photonic/phylayer.h \
	photonic/ringelement.h \
	addon/HotSpot-5.0/temperature_grid.h \
	addon/thermalmodel.h
$O/photonic/devices/machzehnder/mzswitch2x2.o: photonic/devices/machzehnder/mzswitch2x2.cc \
	statistics/statistics.h \
	photonic/devices/machzehnder/mzswitch2x2.h \
	statistics/StatGroup.h \
	electronicComponents/orion/ORION_1_Params/SIM_power.h \
	simCore/messages_m.h \
	addon/HotSpot-5.0/util.h \
	photonic/basicelement.h \
	electronicComponents/orion/ORION_1_Params/SIM_power_test.h \
	statistics/LogFile.h \
	addon/HotSpot-5.0/temperature.h \
	statistics/StatObject.h \
	simCore/sim_includes.h \
	electronicComponents/orion/ORION_Config.h \
	simCore/packetstat.h \
	photonic/activeelement.h \
	electronicComponents/orion/ORION_1_Params/SIM_port.h \
	addon/HotSpot-5.0/flp.h \
	photonic/phylayer.h \
	addon/HotSpot-5.0/temperature_grid.h \
	addon/thermalmodel.h
$O/photonic/devices/machzehnder/mzswitch1x2.o: photonic/devices/machzehnder/mzswitch1x2.cc \
	statistics/statistics.h \
	statistics/StatGroup.h \
	electronicComponents/orion/ORION_1_Params/SIM_power.h \
	simCore/messages_m.h \
	addon/HotSpot-5.0/util.h \
	photonic/basicelement.h \
	electronicComponents/orion/ORION_1_Params/SIM_power_test.h \
	statistics/LogFile.h \
	addon/HotSpot-5.0/temperature.h \
	statistics/StatObject.h \
	simCore/sim_includes.h \
	electronicComponents/orion/ORION_Config.h \
	simCore/packetstat.h \
	photonic/activeelement.h \
	electronicComponents/orion/ORION_1_Params/SIM_port.h \
	addon/HotSpot-5.0/flp.h \
	photonic/phylayer.h \
	addon/HotSpot-5.0/temperature_grid.h \
	photonic/devices/machzehnder/mzswitch1x2.h \
	addon/thermalmodel.h
$O/photonic/devices/multiring/pse2x2multiring.o: photonic/devices/multiring/pse2x2multiring.cc \
	statistics/statistics.h \
	photonic/activemultiringelement.h \
	statistics/StatGroup.h \
	electronicComponents/orion/ORION_1_Params/SIM_power.h \
	simCore/messages_m.h \
	photonic/multiringelement.h \
	addon/HotSpot-5.0/util.h \
	statistics/LogFile.h \
	photonic/basicelement.h \
	electronicComponents/orion/ORION_1_Params/SIM_power_test.h \
	addon/HotSpot-5.0/temperature.h \
	statistics/StatObject.h \
	electronicComponents/orion/ORION_Config.h \
	simCore/sim_includes.h \
	simCore/packetstat.h \
	photonic/activeelement.h \
	electronicComponents/orion/ORION_1_Params/SIM_port.h \
	addon/HotSpot-5.0/flp.h \
	photonic/devices/multiring/pse2x2multiring.h \
	photonic/phylayer.h \
	addon/HotSpot-5.0/temperature_grid.h \
	addon/thermalmodel.h
$O/photonic/devices/multiring/pse1x2nxmultiring.o: photonic/devices/multiring/pse1x2nxmultiring.cc \
	statistics/statistics.h \
	photonic/devices/multiring/pse1x2nxmultiring.h \
	photonic/activemultiringelement.h \
	statistics/StatGroup.h \
	electronicComponents/orion/ORION_1_Params/SIM_power.h \
	simCore/messages_m.h \
	photonic/multiringelement.h \
	addon/HotSpot-5.0/util.h \
	statistics/LogFile.h \
	electronicComponents/orion/ORION_1_Params/SIM_power_test.h \
	photonic/basicelement.h \
	addon/HotSpot-5.0/temperature.h \
	statistics/StatObject.h \
	electronicComponents/orion/ORION_Config.h \
	simCore/sim_includes.h \
	simCore/packetstat.h \
	photonic/activeelement.h \
	electronicComponents/orion/ORION_1_Params/SIM_port.h \
	addon/HotSpot-5.0/flp.h \
	photonic/phylayer.h \
	addon/HotSpot-5.0/temperature_grid.h \
	addon/thermalmodel.h
$O/photonic/devices/multiring/pse1x2r2.o: photonic/devices/multiring/pse1x2r2.cc \
	statistics/statistics.h \
	statistics/StatGroup.h \
	electronicComponents/orion/ORION_1_Params/SIM_power.h \
	simCore/messages_m.h \
	photonic/activetworingelement.h \
	addon/HotSpot-5.0/util.h \
	statistics/LogFile.h \
	photonic/basicelement.h \
	electronicComponents/orion/ORION_1_Params/SIM_power_test.h \
	addon/HotSpot-5.0/temperature.h \
	statistics/StatObject.h \
	electronicComponents/orion/ORION_Config.h \
	simCore/sim_includes.h \
	simCore/packetstat.h \
	photonic/activeelement.h \
	electronicComponents/orion/ORION_1_Params/SIM_port.h \
	photonic/tworingelement.h \
	photonic/devices/multiring/pse1x2r2.h \
	addon/HotSpot-5.0/flp.h \
	photonic/ringelement.h \
	photonic/phylayer.h \
	addon/HotSpot-5.0/temperature_grid.h \
	addon/thermalmodel.h
$O/photonic/devices/multiring/pse1x2multiring.o: photonic/devices/multiring/pse1x2multiring.cc \
	statistics/statistics.h \
	photonic/activemultiringelement.h \
	statistics/StatGroup.h \
	photonic/devices/multiring/pse1x2multiring.h \
	electronicComponents/orion/ORION_1_Params/SIM_power.h \
	simCore/messages_m.h \
	photonic/multiringelement.h \
	addon/HotSpot-5.0/util.h \
	photonic/basicelement.h \
	electronicComponents/orion/ORION_1_Params/SIM_power_test.h \
	statistics/LogFile.h \
	addon/HotSpot-5.0/temperature.h \
	statistics/StatObject.h \
	electronicComponents/orion/ORION_Config.h \
	simCore/sim_includes.h \
	simCore/packetstat.h \
	photonic/activeelement.h \
	electronicComponents/orion/ORION_1_Params/SIM_port.h \
	addon/HotSpot-5.0/flp.h \
	photonic/phylayer.h \
	addon/HotSpot-5.0/temperature_grid.h \
	addon/thermalmodel.h
$O/photonic/devices/multiring/pse2x2r3.o: photonic/devices/multiring/pse2x2r3.cc \
	statistics/statistics.h \
	photonic/activemultiringelement.h \
	statistics/StatGroup.h \
	electronicComponents/orion/ORION_1_Params/SIM_power.h \
	simCore/messages_m.h \
	photonic/devices/multiring/pse2x2r3.h \
	photonic/multiringelement.h \
	addon/HotSpot-5.0/util.h \
	statistics/LogFile.h \
	photonic/basicelement.h \
	electronicComponents/orion/ORION_1_Params/SIM_power_test.h \
	addon/HotSpot-5.0/temperature.h \
	statistics/StatObject.h \
	electronicComponents/orion/ORION_Config.h \
	simCore/sim_includes.h \
	simCore/packetstat.h \
	photonic/activeelement.h \
	electronicComponents/orion/ORION_1_Params/SIM_port.h \
	addon/HotSpot-5.0/flp.h \
	photonic/phylayer.h \
	addon/HotSpot-5.0/temperature_grid.h \
	addon/thermalmodel.h
$O/photonic/devices/multiring/pse2x2r2.o: photonic/devices/multiring/pse2x2r2.cc \
	statistics/statistics.h \
	statistics/StatGroup.h \
	electronicComponents/orion/ORION_1_Params/SIM_power.h \
	simCore/messages_m.h \
	photonic/activetworingelement.h \
	addon/HotSpot-5.0/util.h \
	photonic/basicelement.h \
	electronicComponents/orion/ORION_1_Params/SIM_power_test.h \
	statistics/LogFile.h \
	addon/HotSpot-5.0/temperature.h \
	statistics/StatObject.h \
	simCore/sim_includes.h \
	electronicComponents/orion/ORION_Config.h \
	simCore/packetstat.h \
	photonic/activeelement.h \
	electronicComponents/orion/ORION_1_Params/SIM_port.h \
	photonic/devices/multiring/pse2x2r2.h \
	photonic/tworingelement.h \
	addon/HotSpot-5.0/flp.h \
	photonic/phylayer.h \
	photonic/ringelement.h \
	addon/HotSpot-5.0/temperature_grid.h \
	addon/thermalmodel.h
$O/photonic/devices/multiring/pse1x2r3.o: photonic/devices/multiring/pse1x2r3.cc \
	statistics/statistics.h \
	photonic/activemultiringelement.h \
	statistics/StatGroup.h \
	electronicComponents/orion/ORION_1_Params/SIM_power.h \
	simCore/messages_m.h \
	photonic/multiringelement.h \
	addon/HotSpot-5.0/util.h \
	statistics/LogFile.h \
	photonic/basicelement.h \
	electronicComponents/orion/ORION_1_Params/SIM_power_test.h \
	addon/HotSpot-5.0/temperature.h \
	statistics/StatObject.h \
	electronicComponents/orion/ORION_Config.h \
	simCore/sim_includes.h \
	simCore/packetstat.h \
	photonic/activeelement.h \
	electronicComponents/orion/ORION_1_Params/SIM_port.h \
	addon/HotSpot-5.0/flp.h \
	photonic/phylayer.h \
	addon/HotSpot-5.0/temperature_grid.h \
	addon/thermalmodel.h \
	photonic/devices/multiring/pse1x2r3.h
$O/photonic/devices/multiring/pse1x2nxr3.o: photonic/devices/multiring/pse1x2nxr3.cc \
	statistics/statistics.h \
	photonic/activemultiringelement.h \
	statistics/StatGroup.h \
	electronicComponents/orion/ORION_1_Params/SIM_power.h \
	simCore/messages_m.h \
	photonic/multiringelement.h \
	addon/HotSpot-5.0/util.h \
	statistics/LogFile.h \
	photonic/basicelement.h \
	electronicComponents/orion/ORION_1_Params/SIM_power_test.h \
	addon/HotSpot-5.0/temperature.h \
	statistics/StatObject.h \
	electronicComponents/orion/ORION_Config.h \
	simCore/sim_includes.h \
	simCore/packetstat.h \
	photonic/devices/multiring/pse1x2nxr3.h \
	photonic/activeelement.h \
	electronicComponents/orion/ORION_1_Params/SIM_port.h \
	addon/HotSpot-5.0/flp.h \
	photonic/phylayer.h \
	addon/HotSpot-5.0/temperature_grid.h \
	addon/thermalmodel.h
$O/photonic/devices/multiring/pse1x2nxr2.o: photonic/devices/multiring/pse1x2nxr2.cc \
	statistics/statistics.h \
	photonic/devices/multiring/pse1x2nxr2.h \
	statistics/StatGroup.h \
	electronicComponents/orion/ORION_1_Params/SIM_power.h \
	simCore/messages_m.h \
	photonic/activetworingelement.h \
	addon/HotSpot-5.0/util.h \
	statistics/LogFile.h \
	photonic/basicelement.h \
	electronicComponents/orion/ORION_1_Params/SIM_power_test.h \
	addon/HotSpot-5.0/temperature.h \
	statistics/StatObject.h \
	electronicComponents/orion/ORION_Config.h \
	simCore/sim_includes.h \
	simCore/packetstat.h \
	photonic/activeelement.h \
	electronicComponents/orion/ORION_1_Params/SIM_port.h \
	photonic/tworingelement.h \
	addon/HotSpot-5.0/flp.h \
	photonic/ringelement.h \
	photonic/phylayer.h \
	addon/HotSpot-5.0/temperature_grid.h \
	addon/thermalmodel.h
$O/processingPlane/Processor.o: processingPlane/Processor.cc \
	processingPlane/DRAM_Cfg/DRAM_Cfg_Periphery_SR8x8.h \
	processingPlane/apps/synthetics/AppNeighbor.h \
	processingPlane/apps/trace/App_LL_DepGraph.h \
	processingPlane/apps/synthetics/AppHotspot.h \
	processingPlane/apps/SizeDistribution/SizeDistribution.h \
	electronicComponents/Arbiters/Arbiter.h \
	processingPlane/apps/DRAM/AppDRAMAll2One.h \
	electronicComponents/orion/ORION_Config.h \
	processingPlane/apps/MITUCB/App_MITUCB_All2All.h \
	processingPlane/apps/model/AppFFT.h \
	processingPlane/addressTranslation/AddressTranslator_FatTree.h \
	photonic/phylayer.h \
	addon/thermalmodel.h \
	electronicComponents/orion/ORION_Util.h \
	electronicComponents/orion/ORION_Arbiter.h \
	processingPlane/apps/model/AppDataFlow.h \
	processingPlane/apps/SizeDistribution/SizeDistribution_Constant.h \
	electronicComponents/orion/ORION_1_Params/SIM_power.h \
	processingPlane/apps/synthetics/AppRandom_FiniteQueue.h \
	electronicComponents/orion/ORION_Array_Info.h \
	processingPlane/apps/synthetics/AppRandom.h \
	processingPlane/apps/synthetics/AppBitreverse.h \
	addon/HotSpot-5.0/temperature.h \
	processingPlane/apps/test/AppAll2All.h \
	processingPlane/apps/DRAM/AppDRAMOne2One.h \
	electronicComponents/Arbiters/ArbiterCanRoute.h \
	processingPlane/addressTranslation/AddressTranslator_Standard.h \
	processingPlane/apps/DRAM/AppDRAMRandom.h \
	processingPlane/DRAM_Cfg/DRAM_Cfg_Periphery_SR4x4.h \
	processingPlane/apps/test/AppOne2One.h \
	electronicComponents/orion/ORION_Array_Internal.h \
	statistics/statistics.h \
	processingPlane/apps/SizeDistribution/SizeDistribution_Gamma.h \
	simCore/messages_m.h \
	electronicComponents/VirtualChannelControl.h \
	addon/HotSpot-5.0/util.h \
	electronicComponents/orion/ORION_1_Params/SIM_power_test.h \
	statistics/LogFile.h \
	processingPlane/apps/SizeDistribution/SizeDistribution_InvGamma.h \
	processingPlane/apps/model/AppFFTstream.h \
	processingPlane/DRAM_Cfg/DRAM_Cfg.h \
	electronicComponents/orion/ORION_1_Params/SIM_port.h \
	simCore/NetworkAddress.h \
	processingPlane/DRAM_Cfg/DRAM_Cfg_Periphery.h \
	addon/HotSpot-5.0/flp.h \
	processingPlane/apps/MITUCB/App_MITUCB_Random.h \
	processingPlane/apps/DRAM/AppDRAMOne2All.h \
	statistics/StatGroup.h \
	processingPlane/DRAM_Cfg/DRAM_Cfg_Periphery_SR16x16.h \
	processingPlane/DRAM_Cfg/DRAM_Cfg_AllNodes.h \
	processingPlane/apps/synthetics/AppTornado.h \
	processingPlane/apps/test/AppBcastTest.h \
	processingPlane/apps/SizeDistribution/SizeDistribution_Control.h \
	processingPlane/addressTranslation/AddressTranslator_SquareRoot.h \
	statistics/StatObject.h \
	simCore/sim_includes.h \
	processingPlane/apps/SizeDistribution/SizeDistribution_Normal.h \
	processingPlane/apps/trace/AppTrace_LBL.h \
	processingPlane/addressTranslation/AddressTranslator.h \
	processingPlane/apps/Application.h \
	electronicComponents/orion/ORION_Array.h \
	processingPlane/Processor.h \
	addon/HotSpot-5.0/temperature_grid.h \
	electronicComponents/VC_Control.h
$O/processingPlane/GatewaySignalControl.o: processingPlane/GatewaySignalControl.cc \
	statistics/StatObject.h \
	simCore/sim_includes.h \
	processingPlane/GatewaySignalControl.h \
	simCore/messages_m.h
$O/processingPlane/ProcessorRouterArbiter.o: processingPlane/ProcessorRouterArbiter.cc \
	electronicComponents/orion/ORION_Array_Internal.h \
	statistics/statistics.h \
	simCore/messages_m.h \
	electronicComponents/VirtualChannelControl.h \
	processingPlane/ProcessorRouterArbiter.h \
	addon/HotSpot-5.0/util.h \
	electronicComponents/RouterArbiter.h \
	statistics/LogFile.h \
	electronicComponents/orion/ORION_1_Params/SIM_power_test.h \
	processingPlane/apps/SizeDistribution/SizeDistribution.h \
	electronicComponents/orion/ORION_Config.h \
	electronicComponents/Arbiters/Arbiter.h \
	processingPlane/DRAM_Cfg/DRAM_Cfg.h \
	simCore/NetworkAddress.h \
	electronicComponents/orion/ORION_1_Params/SIM_port.h \
	addon/HotSpot-5.0/flp.h \
	photonic/phylayer.h \
	addon/thermalmodel.h \
	electronicComponents/orion/ORION_Util.h \
	electronicComponents/orion/ORION_Arbiter.h \
	statistics/StatGroup.h \
	electronicComponents/orion/ORION_1_Params/SIM_power.h \
	electronicComponents/orion/ORION_Array_Info.h \
	addon/HotSpot-5.0/temperature.h \
	statistics/StatObject.h \
	simCore/sim_includes.h \
	electronicComponents/Arbiters/ArbiterCanRoute.h \
	processingPlane/addressTranslation/AddressTranslator.h \
	processingPlane/apps/Application.h \
	processingPlane/Processor.h \
	electronicComponents/orion/ORION_Array.h \
	addon/HotSpot-5.0/temperature_grid.h \
	electronicComponents/VC_Control.h
$O/processingPlane/DRAM_Cfg/DRAM_Cfg_Periphery_SR8x8.o: processingPlane/DRAM_Cfg/DRAM_Cfg_Periphery_SR8x8.cc \
	processingPlane/DRAM_Cfg/DRAM_Cfg.h \
	processingPlane/DRAM_Cfg/DRAM_Cfg_Periphery_SR8x8.h
$O/processingPlane/DRAM_Cfg/DRAM_Cfg_Periphery_SR16x16.o: processingPlane/DRAM_Cfg/DRAM_Cfg_Periphery_SR16x16.cc \
	processingPlane/DRAM_Cfg/DRAM_Cfg.h \
	processingPlane/DRAM_Cfg/DRAM_Cfg_Periphery_SR16x16.h
$O/processingPlane/DRAM_Cfg/DRAM_Cfg.o: processingPlane/DRAM_Cfg/DRAM_Cfg.cc \
	processingPlane/DRAM_Cfg/DRAM_Cfg.h
$O/processingPlane/DRAM_Cfg/DRAM_Cfg_Periphery.o: processingPlane/DRAM_Cfg/DRAM_Cfg_Periphery.cc \
	processingPlane/DRAM_Cfg/DRAM_Cfg.h \
	processingPlane/DRAM_Cfg/DRAM_Cfg_Periphery.h
$O/processingPlane/DRAM_Cfg/DRAM_Cfg_Periphery_SR4x4.o: processingPlane/DRAM_Cfg/DRAM_Cfg_Periphery_SR4x4.cc \
	processingPlane/DRAM_Cfg/DRAM_Cfg.h \
	processingPlane/DRAM_Cfg/DRAM_Cfg_Periphery_SR4x4.h
$O/processingPlane/DRAM_Cfg/DRAM_Cfg_AllNodes.o: processingPlane/DRAM_Cfg/DRAM_Cfg_AllNodes.cc \
	processingPlane/DRAM_Cfg/DRAM_Cfg.h \
	processingPlane/DRAM_Cfg/DRAM_Cfg_AllNodes.h
$O/processingPlane/addressTranslation/AddressTranslator_Standard.o: processingPlane/addressTranslation/AddressTranslator_Standard.cc \
	processingPlane/addressTranslation/AddressTranslator_Standard.h \
	simCore/NetworkAddress.h \
	processingPlane/addressTranslation/AddressTranslator.h \
	simCore/messages_m.h
$O/processingPlane/addressTranslation/AddressTranslator.o: processingPlane/addressTranslation/AddressTranslator.cc \
	simCore/NetworkAddress.h \
	processingPlane/addressTranslation/AddressTranslator.h \
	simCore/messages_m.h
$O/processingPlane/addressTranslation/AddressTranslator_SquareRoot.o: processingPlane/addressTranslation/AddressTranslator_SquareRoot.cc \
	processingPlane/addressTranslation/AddressTranslator_SquareRoot.h \
	simCore/NetworkAddress.h \
	processingPlane/addressTranslation/AddressTranslator.h \
	simCore/messages_m.h
$O/processingPlane/addressTranslation/AddressTranslator_FatTree.o: processingPlane/addressTranslation/AddressTranslator_FatTree.cc \
	simCore/NetworkAddress.h \
	processingPlane/addressTranslation/AddressTranslator.h \
	simCore/messages_m.h \
	processingPlane/addressTranslation/AddressTranslator_FatTree.h
$O/processingPlane/apps/Application.o: processingPlane/apps/Application.cc \
	statistics/statistics.h \
	statistics/StatGroup.h \
	electronicComponents/orion/ORION_1_Params/SIM_power.h \
	simCore/messages_m.h \
	electronicComponents/orion/ORION_1_Params/SIM_power_test.h \
	statistics/LogFile.h \
	statistics/StatObject.h \
	processingPlane/apps/SizeDistribution/SizeDistribution.h \
	electronicComponents/orion/ORION_Config.h \
	simCore/sim_includes.h \
	processingPlane/DRAM_Cfg/DRAM_Cfg.h \
	simCore/NetworkAddress.h \
	electronicComponents/orion/ORION_1_Params/SIM_port.h \
	processingPlane/apps/Application.h \
	photonic/phylayer.h
$O/processingPlane/apps/DRAM/AppDRAMOne2All.o: processingPlane/apps/DRAM/AppDRAMOne2All.cc \
	statistics/statistics.h \
	processingPlane/apps/DRAM/AppDRAMOne2All.h \
	statistics/StatGroup.h \
	electronicComponents/orion/ORION_1_Params/SIM_power.h \
	simCore/messages_m.h \
	electronicComponents/orion/ORION_1_Params/SIM_power_test.h \
	statistics/LogFile.h \
	processingPlane/apps/SizeDistribution/SizeDistribution.h \
	statistics/StatObject.h \
	simCore/sim_includes.h \
	electronicComponents/orion/ORION_Config.h \
	processingPlane/DRAM_Cfg/DRAM_Cfg.h \
	electronicComponents/orion/ORION_1_Params/SIM_port.h \
	simCore/NetworkAddress.h \
	processingPlane/apps/Application.h \
	photonic/phylayer.h
$O/processingPlane/apps/DRAM/AppDRAMAll2One.o: processingPlane/apps/DRAM/AppDRAMAll2One.cc \
	statistics/statistics.h \
	statistics/StatGroup.h \
	electronicComponents/orion/ORION_1_Params/SIM_power.h \
	simCore/messages_m.h \
	electronicComponents/orion/ORION_1_Params/SIM_power_test.h \
	statistics/LogFile.h \
	processingPlane/apps/SizeDistribution/SizeDistribution.h \
	statistics/StatObject.h \
	processingPlane/apps/DRAM/AppDRAMAll2One.h \
	simCore/sim_includes.h \
	electronicComponents/orion/ORION_Config.h \
	processingPlane/DRAM_Cfg/DRAM_Cfg.h \
	electronicComponents/orion/ORION_1_Params/SIM_port.h \
	simCore/NetworkAddress.h \
	processingPlane/apps/Application.h \
	photonic/phylayer.h
$O/processingPlane/apps/DRAM/AppDRAMRandom.o: processingPlane/apps/DRAM/AppDRAMRandom.cc \
	statistics/statistics.h \
	statistics/StatGroup.h \
	electronicComponents/orion/ORION_1_Params/SIM_power.h \
	simCore/messages_m.h \
	electronicComponents/orion/ORION_1_Params/SIM_power_test.h \
	statistics/LogFile.h \
	processingPlane/apps/SizeDistribution/SizeDistribution.h \
	statistics/StatObject.h \
	simCore/sim_includes.h \
	electronicComponents/orion/ORION_Config.h \
	processingPlane/DRAM_Cfg/DRAM_Cfg.h \
	processingPlane/apps/DRAM/AppDRAMRandom.h \
	electronicComponents/orion/ORION_1_Params/SIM_port.h \
	simCore/NetworkAddress.h \
	processingPlane/apps/Application.h \
	photonic/phylayer.h
$O/processingPlane/apps/DRAM/AppDRAMOne2One.o: processingPlane/apps/DRAM/AppDRAMOne2One.cc \
	statistics/statistics.h \
	statistics/StatGroup.h \
	electronicComponents/orion/ORION_1_Params/SIM_power.h \
	simCore/messages_m.h \
	electronicComponents/orion/ORION_1_Params/SIM_power_test.h \
	statistics/LogFile.h \
	processingPlane/apps/SizeDistribution/SizeDistribution.h \
	statistics/StatObject.h \
	electronicComponents/orion/ORION_Config.h \
	simCore/sim_includes.h \
	processingPlane/apps/DRAM/AppDRAMOne2One.h \
	processingPlane/DRAM_Cfg/DRAM_Cfg.h \
	electronicComponents/orion/ORION_1_Params/SIM_port.h \
	simCore/NetworkAddress.h \
	processingPlane/apps/Application.h \
	photonic/phylayer.h
$O/processingPlane/apps/MITUCB/App_MITUCB_All2All.o: processingPlane/apps/MITUCB/App_MITUCB_All2All.cc \
	statistics/statistics.h \
	statistics/StatGroup.h \
	electronicComponents/orion/ORION_1_Params/SIM_power.h \
	simCore/messages_m.h \
	electronicComponents/orion/ORION_1_Params/SIM_power_test.h \
	statistics/LogFile.h \
	processingPlane/apps/SizeDistribution/SizeDistribution.h \
	statistics/StatObject.h \
	simCore/sim_includes.h \
	electronicComponents/orion/ORION_Config.h \
	processingPlane/DRAM_Cfg/DRAM_Cfg.h \
	electronicComponents/orion/ORION_1_Params/SIM_port.h \
	simCore/NetworkAddress.h \
	processingPlane/apps/MITUCB/App_MITUCB_All2All.h \
	processingPlane/apps/Application.h \
	photonic/phylayer.h
$O/processingPlane/apps/MITUCB/App_MITUCB_Random.o: processingPlane/apps/MITUCB/App_MITUCB_Random.cc \
	statistics/statistics.h \
	statistics/StatGroup.h \
	electronicComponents/orion/ORION_1_Params/SIM_power.h \
	simCore/messages_m.h \
	electronicComponents/orion/ORION_1_Params/SIM_power_test.h \
	statistics/LogFile.h \
	processingPlane/apps/SizeDistribution/SizeDistribution.h \
	statistics/StatObject.h \
	simCore/sim_includes.h \
	electronicComponents/orion/ORION_Config.h \
	processingPlane/DRAM_Cfg/DRAM_Cfg.h \
	electronicComponents/orion/ORION_1_Params/SIM_port.h \
	simCore/NetworkAddress.h \
	processingPlane/apps/Application.h \
	photonic/phylayer.h \
	processingPlane/apps/MITUCB/App_MITUCB_Random.h
$O/processingPlane/apps/SizeDistribution/SizeDistribution_Control.o: processingPlane/apps/SizeDistribution/SizeDistribution_Control.cc \
	processingPlane/apps/SizeDistribution/SizeDistribution.h \
	processingPlane/apps/SizeDistribution/SizeDistribution_Control.h
$O/processingPlane/apps/SizeDistribution/SizeDistribution.o: processingPlane/apps/SizeDistribution/SizeDistribution.cc \
	processingPlane/apps/SizeDistribution/SizeDistribution.h
$O/processingPlane/apps/SizeDistribution/SizeDistribution_Normal.o: processingPlane/apps/SizeDistribution/SizeDistribution_Normal.cc \
	processingPlane/apps/SizeDistribution/SizeDistribution.h \
	processingPlane/apps/SizeDistribution/SizeDistribution_Normal.h
$O/processingPlane/apps/SizeDistribution/SizeDistribution_InvGamma.o: processingPlane/apps/SizeDistribution/SizeDistribution_InvGamma.cc \
	processingPlane/apps/SizeDistribution/SizeDistribution.h \
	processingPlane/apps/SizeDistribution/SizeDistribution_InvGamma.h \
	processingPlane/apps/SizeDistribution/SizeDistribution_Gamma.h
$O/processingPlane/apps/SizeDistribution/SizeDistribution_Gamma.o: processingPlane/apps/SizeDistribution/SizeDistribution_Gamma.cc \
	processingPlane/apps/SizeDistribution/SizeDistribution.h \
	processingPlane/apps/SizeDistribution/SizeDistribution_Gamma.h
$O/processingPlane/apps/SizeDistribution/SizeDistribution_Constant.o: processingPlane/apps/SizeDistribution/SizeDistribution_Constant.cc \
	processingPlane/apps/SizeDistribution/SizeDistribution.h \
	processingPlane/apps/SizeDistribution/SizeDistribution_Constant.h
$O/processingPlane/apps/model/AppFFT.o: processingPlane/apps/model/AppFFT.cc \
	statistics/statistics.h \
	statistics/StatGroup.h \
	electronicComponents/orion/ORION_1_Params/SIM_power.h \
	simCore/messages_m.h \
	electronicComponents/orion/ORION_1_Params/SIM_power_test.h \
	statistics/LogFile.h \
	processingPlane/apps/SizeDistribution/SizeDistribution.h \
	statistics/StatObject.h \
	simCore/sim_includes.h \
	electronicComponents/orion/ORION_Config.h \
	processingPlane/DRAM_Cfg/DRAM_Cfg.h \
	electronicComponents/orion/ORION_1_Params/SIM_port.h \
	simCore/NetworkAddress.h \
	processingPlane/apps/Application.h \
	processingPlane/apps/model/AppFFT.h \
	photonic/phylayer.h
$O/processingPlane/apps/model/AppFFTstream.o: processingPlane/apps/model/AppFFTstream.cc \
	statistics/statistics.h \
	statistics/StatGroup.h \
	electronicComponents/orion/ORION_1_Params/SIM_power.h \
	simCore/messages_m.h \
	electronicComponents/orion/ORION_1_Params/SIM_power_test.h \
	statistics/LogFile.h \
	processingPlane/apps/SizeDistribution/SizeDistribution.h \
	statistics/StatObject.h \
	simCore/sim_includes.h \
	electronicComponents/orion/ORION_Config.h \
	processingPlane/apps/model/AppFFTstream.h \
	processingPlane/DRAM_Cfg/DRAM_Cfg.h \
	simCore/NetworkAddress.h \
	electronicComponents/orion/ORION_1_Params/SIM_port.h \
	processingPlane/apps/Application.h \
	photonic/phylayer.h
$O/processingPlane/apps/model/AppDataFlow.o: processingPlane/apps/model/AppDataFlow.cc \
	statistics/statistics.h \
	statistics/StatGroup.h \
	processingPlane/apps/model/AppDataFlow.h \
	electronicComponents/orion/ORION_1_Params/SIM_power.h \
	simCore/messages_m.h \
	electronicComponents/orion/ORION_1_Params/SIM_power_test.h \
	statistics/LogFile.h \
	processingPlane/apps/SizeDistribution/SizeDistribution.h \
	statistics/StatObject.h \
	simCore/sim_includes.h \
	electronicComponents/orion/ORION_Config.h \
	processingPlane/DRAM_Cfg/DRAM_Cfg.h \
	simCore/NetworkAddress.h \
	electronicComponents/orion/ORION_1_Params/SIM_port.h \
	processingPlane/apps/Application.h \
	photonic/phylayer.h
$O/processingPlane/apps/synthetics/AppRandom_FiniteQueue.o: processingPlane/apps/synthetics/AppRandom_FiniteQueue.cc \
	statistics/statistics.h \
	statistics/StatGroup.h \
	electronicComponents/orion/ORION_1_Params/SIM_power.h \
	simCore/messages_m.h \
	processingPlane/apps/synthetics/AppRandom_FiniteQueue.h \
	electronicComponents/orion/ORION_1_Params/SIM_power_test.h \
	statistics/LogFile.h \
	processingPlane/apps/SizeDistribution/SizeDistribution.h \
	statistics/StatObject.h \
	simCore/sim_includes.h \
	electronicComponents/orion/ORION_Config.h \
	processingPlane/DRAM_Cfg/DRAM_Cfg.h \
	electronicComponents/orion/ORION_1_Params/SIM_port.h \
	simCore/NetworkAddress.h \
	processingPlane/apps/Application.h \
	photonic/phylayer.h
$O/processingPlane/apps/synthetics/AppRandom.o: processingPlane/apps/synthetics/AppRandom.cc \
	statistics/statistics.h \
	statistics/StatGroup.h \
	electronicComponents/orion/ORION_1_Params/SIM_power.h \
	simCore/messages_m.h \
	processingPlane/apps/synthetics/AppRandom.h \
	electronicComponents/orion/ORION_1_Params/SIM_power_test.h \
	statistics/LogFile.h \
	processingPlane/apps/SizeDistribution/SizeDistribution.h \
	statistics/StatObject.h \
	simCore/sim_includes.h \
	electronicComponents/orion/ORION_Config.h \
	processingPlane/DRAM_Cfg/DRAM_Cfg.h \
	electronicComponents/orion/ORION_1_Params/SIM_port.h \
	simCore/NetworkAddress.h \
	processingPlane/apps/Application.h \
	photonic/phylayer.h
$O/processingPlane/apps/synthetics/AppTornado.o: processingPlane/apps/synthetics/AppTornado.cc \
	statistics/statistics.h \
	statistics/StatGroup.h \
	electronicComponents/orion/ORION_1_Params/SIM_power.h \
	simCore/messages_m.h \
	processingPlane/apps/synthetics/AppTornado.h \
	electronicComponents/orion/ORION_1_Params/SIM_power_test.h \
	statistics/LogFile.h \
	processingPlane/apps/SizeDistribution/SizeDistribution.h \
	statistics/StatObject.h \
	simCore/sim_includes.h \
	electronicComponents/orion/ORION_Config.h \
	processingPlane/DRAM_Cfg/DRAM_Cfg.h \
	electronicComponents/orion/ORION_1_Params/SIM_port.h \
	simCore/NetworkAddress.h \
	processingPlane/apps/Application.h \
	photonic/phylayer.h
$O/processingPlane/apps/synthetics/AppNeighbor.o: processingPlane/apps/synthetics/AppNeighbor.cc \
	statistics/statistics.h \
	processingPlane/apps/synthetics/AppNeighbor.h \
	statistics/StatGroup.h \
	electronicComponents/orion/ORION_1_Params/SIM_power.h \
	simCore/messages_m.h \
	electronicComponents/orion/ORION_1_Params/SIM_power_test.h \
	statistics/LogFile.h \
	processingPlane/apps/SizeDistribution/SizeDistribution.h \
	statistics/StatObject.h \
	simCore/sim_includes.h \
	electronicComponents/orion/ORION_Config.h \
	processingPlane/DRAM_Cfg/DRAM_Cfg.h \
	electronicComponents/orion/ORION_1_Params/SIM_port.h \
	simCore/NetworkAddress.h \
	processingPlane/apps/Application.h \
	photonic/phylayer.h
$O/processingPlane/apps/synthetics/AppHotspot.o: processingPlane/apps/synthetics/AppHotspot.cc \
	statistics/statistics.h \
	statistics/StatGroup.h \
	electronicComponents/orion/ORION_1_Params/SIM_power.h \
	simCore/messages_m.h \
	processingPlane/apps/synthetics/AppHotspot.h \
	electronicComponents/orion/ORION_1_Params/SIM_power_test.h \
	statistics/LogFile.h \
	processingPlane/apps/SizeDistribution/SizeDistribution.h \
	statistics/StatObject.h \
	simCore/sim_includes.h \
	electronicComponents/orion/ORION_Config.h \
	processingPlane/DRAM_Cfg/DRAM_Cfg.h \
	electronicComponents/orion/ORION_1_Params/SIM_port.h \
	simCore/NetworkAddress.h \
	processingPlane/apps/Application.h \
	photonic/phylayer.h
$O/processingPlane/apps/synthetics/AppBitreverse.o: processingPlane/apps/synthetics/AppBitreverse.cc \
	statistics/statistics.h \
	statistics/StatGroup.h \
	electronicComponents/orion/ORION_1_Params/SIM_power.h \
	simCore/messages_m.h \
	processingPlane/apps/synthetics/AppBitreverse.h \
	electronicComponents/orion/ORION_1_Params/SIM_power_test.h \
	statistics/LogFile.h \
	processingPlane/apps/SizeDistribution/SizeDistribution.h \
	statistics/StatObject.h \
	simCore/sim_includes.h \
	electronicComponents/orion/ORION_Config.h \
	processingPlane/DRAM_Cfg/DRAM_Cfg.h \
	electronicComponents/orion/ORION_1_Params/SIM_port.h \
	simCore/NetworkAddress.h \
	processingPlane/apps/Application.h \
	photonic/phylayer.h
$O/processingPlane/apps/test/AppBcastTest.o: processingPlane/apps/test/AppBcastTest.cc \
	statistics/statistics.h \
	statistics/StatGroup.h \
	electronicComponents/orion/ORION_1_Params/SIM_power.h \
	simCore/messages_m.h \
	processingPlane/apps/test/AppBcastTest.h \
	electronicComponents/orion/ORION_1_Params/SIM_power_test.h \
	statistics/LogFile.h \
	processingPlane/apps/SizeDistribution/SizeDistribution.h \
	statistics/StatObject.h \
	simCore/sim_includes.h \
	electronicComponents/orion/ORION_Config.h \
	processingPlane/DRAM_Cfg/DRAM_Cfg.h \
	electronicComponents/orion/ORION_1_Params/SIM_port.h \
	simCore/NetworkAddress.h \
	processingPlane/apps/Application.h \
	photonic/phylayer.h
$O/processingPlane/apps/test/AppOne2One.o: processingPlane/apps/test/AppOne2One.cc \
	statistics/statistics.h \
	statistics/StatGroup.h \
	electronicComponents/orion/ORION_1_Params/SIM_power.h \
	simCore/messages_m.h \
	electronicComponents/orion/ORION_1_Params/SIM_power_test.h \
	statistics/LogFile.h \
	processingPlane/apps/SizeDistribution/SizeDistribution.h \
	statistics/StatObject.h \
	simCore/sim_includes.h \
	electronicComponents/orion/ORION_Config.h \
	processingPlane/DRAM_Cfg/DRAM_Cfg.h \
	electronicComponents/orion/ORION_1_Params/SIM_port.h \
	simCore/NetworkAddress.h \
	processingPlane/apps/test/AppOne2One.h \
	processingPlane/apps/Application.h \
	photonic/phylayer.h
$O/processingPlane/apps/test/AppAll2All.o: processingPlane/apps/test/AppAll2All.cc \
	statistics/statistics.h \
	statistics/StatGroup.h \
	electronicComponents/orion/ORION_1_Params/SIM_power.h \
	simCore/messages_m.h \
	electronicComponents/orion/ORION_1_Params/SIM_power_test.h \
	statistics/LogFile.h \
	processingPlane/apps/SizeDistribution/SizeDistribution.h \
	statistics/StatObject.h \
	simCore/sim_includes.h \
	electronicComponents/orion/ORION_Config.h \
	processingPlane/apps/test/AppAll2All.h \
	processingPlane/DRAM_Cfg/DRAM_Cfg.h \
	electronicComponents/orion/ORION_1_Params/SIM_port.h \
	simCore/NetworkAddress.h \
	processingPlane/apps/Application.h \
	photonic/phylayer.h
$O/processingPlane/apps/trace/App_LL_DepGraph.o: processingPlane/apps/trace/App_LL_DepGraph.cc \
	statistics/statistics.h \
	statistics/StatGroup.h \
	electronicComponents/orion/ORION_1_Params/SIM_power.h \
	simCore/messages_m.h \
	processingPlane/apps/trace/App_LL_DepGraph.h \
	electronicComponents/orion/ORION_1_Params/SIM_power_test.h \
	statistics/LogFile.h \
	processingPlane/apps/SizeDistribution/SizeDistribution.h \
	statistics/StatObject.h \
	simCore/sim_includes.h \
	electronicComponents/orion/ORION_Config.h \
	processingPlane/DRAM_Cfg/DRAM_Cfg.h \
	electronicComponents/orion/ORION_1_Params/SIM_port.h \
	simCore/NetworkAddress.h \
	processingPlane/apps/Application.h \
	photonic/phylayer.h
$O/processingPlane/apps/trace/AppTrace_LBL.o: processingPlane/apps/trace/AppTrace_LBL.cc \
	statistics/statistics.h \
	statistics/StatGroup.h \
	electronicComponents/orion/ORION_1_Params/SIM_power.h \
	simCore/messages_m.h \
	electronicComponents/orion/ORION_1_Params/SIM_power_test.h \
	statistics/LogFile.h \
	processingPlane/apps/SizeDistribution/SizeDistribution.h \
	statistics/StatObject.h \
	simCore/sim_includes.h \
	electronicComponents/orion/ORION_Config.h \
	processingPlane/DRAM_Cfg/DRAM_Cfg.h \
	processingPlane/apps/trace/AppTrace_LBL.h \
	simCore/NetworkAddress.h \
	electronicComponents/orion/ORION_1_Params/SIM_port.h \
	processingPlane/apps/Application.h \
	photonic/phylayer.h
$O/processingPlane/interfaces/NIF_Packet_Credit.o: processingPlane/interfaces/NIF_Packet_Credit.cc \
	statistics/statistics.h \
	electronicComponents/orion/ORION_Array_Internal.h \
	simCore/messages_m.h \
	electronicComponents/VirtualChannelControl.h \
	processingPlane/interfaces/NIF_Packet_Credit.h \
	addon/HotSpot-5.0/util.h \
	statistics/LogFile.h \
	electronicComponents/orion/ORION_1_Params/SIM_power_test.h \
	processingPlane/apps/SizeDistribution/SizeDistribution.h \
	electronicComponents/Arbiters/Arbiter.h \
	electronicComponents/orion/ORION_Config.h \
	processingPlane/DRAM_Cfg/DRAM_Cfg.h \
	simCore/NetworkAddress.h \
	electronicComponents/orion/ORION_1_Params/SIM_port.h \
	addon/HotSpot-5.0/flp.h \
	photonic/phylayer.h \
	addon/thermalmodel.h \
	electronicComponents/orion/ORION_Util.h \
	electronicComponents/orion/ORION_Arbiter.h \
	statistics/StatGroup.h \
	electronicComponents/orion/ORION_1_Params/SIM_power.h \
	electronicComponents/orion/ORION_Array_Info.h \
	processingPlane/interfaces/NIF.h \
	addon/HotSpot-5.0/temperature.h \
	statistics/StatObject.h \
	simCore/sim_includes.h \
	electronicComponents/Arbiters/ArbiterCanRoute.h \
	processingPlane/addressTranslation/AddressTranslator.h \
	processingPlane/apps/Application.h \
	electronicComponents/orion/ORION_Array.h \
	processingPlane/Processor.h \
	addon/HotSpot-5.0/temperature_grid.h \
	electronicComponents/VC_Control.h
$O/processingPlane/interfaces/NIF_Electronic.o: processingPlane/interfaces/NIF_Electronic.cc \
	electronicComponents/orion/ORION_Array_Internal.h \
	statistics/statistics.h \
	simCore/messages_m.h \
	electronicComponents/VirtualChannelControl.h \
	processingPlane/interfaces/NIF_Packet_Credit.h \
	addon/HotSpot-5.0/util.h \
	electronicComponents/orion/ORION_1_Params/SIM_power_test.h \
	statistics/LogFile.h \
	processingPlane/apps/SizeDistribution/SizeDistribution.h \
	electronicComponents/orion/ORION_Config.h \
	electronicComponents/Arbiters/Arbiter.h \
	processingPlane/interfaces/NIF_Electronic.h \
	processingPlane/DRAM_Cfg/DRAM_Cfg.h \
	simCore/NetworkAddress.h \
	electronicComponents/orion/ORION_1_Params/SIM_port.h \
	addon/HotSpot-5.0/flp.h \
	photonic/phylayer.h \
	addon/thermalmodel.h \
	electronicComponents/orion/ORION_Util.h \
	electronicComponents/orion/ORION_Arbiter.h \
	statistics/StatGroup.h \
	electronicComponents/orion/ORION_1_Params/SIM_power.h \
	electronicComponents/orion/ORION_Array_Info.h \
	processingPlane/interfaces/NIF.h \
	statistics/StatObject.h \
	addon/HotSpot-5.0/temperature.h \
	simCore/sim_includes.h \
	electronicComponents/Arbiters/ArbiterCanRoute.h \
	processingPlane/addressTranslation/AddressTranslator.h \
	processingPlane/apps/Application.h \
	processingPlane/Processor.h \
	electronicComponents/orion/ORION_Array.h \
	electronicComponents/VC_Control.h \
	addon/HotSpot-5.0/temperature_grid.h
$O/processingPlane/interfaces/NIF_Photonic.o: processingPlane/interfaces/NIF_Photonic.cc \
	electronicComponents/orion/ORION_Array_Internal.h \
	statistics/statistics.h \
	simCore/messages_m.h \
	electronicComponents/VirtualChannelControl.h \
	processingPlane/interfaces/NIF_Packet_Credit.h \
	addon/HotSpot-5.0/util.h \
	electronicComponents/orion/ORION_1_Params/SIM_power_test.h \
	statistics/LogFile.h \
	processingPlane/apps/SizeDistribution/SizeDistribution.h \
	electronicComponents/Arbiters/Arbiter.h \
	electronicComponents/orion/ORION_Config.h \
	simCore/packetstat.h \
	processingPlane/DRAM_Cfg/DRAM_Cfg.h \
	simCore/NetworkAddress.h \
	electronicComponents/orion/ORION_1_Params/SIM_port.h \
	addon/HotSpot-5.0/flp.h \
	photonic/phylayer.h \
	addon/thermalmodel.h \
	electronicComponents/orion/ORION_Util.h \
	electronicComponents/orion/ORION_Arbiter.h \
	statistics/StatGroup.h \
	electronicComponents/orion/ORION_1_Params/SIM_power.h \
	electronicComponents/orion/ORION_Array_Info.h \
	processingPlane/interfaces/NIF.h \
	statistics/StatObject.h \
	addon/HotSpot-5.0/temperature.h \
	simCore/sim_includes.h \
	processingPlane/interfaces/NIF_Circuit.h \
	electronicComponents/Arbiters/ArbiterCanRoute.h \
	processingPlane/addressTranslation/AddressTranslator.h \
	processingPlane/apps/Application.h \
	processingPlane/interfaces/NIF_Photonic.h \
	processingPlane/Processor.h \
	electronicComponents/orion/ORION_Array.h \
	electronicComponents/VC_Control.h \
	addon/HotSpot-5.0/temperature_grid.h
$O/processingPlane/interfaces/NIF_ElectronicCC.o: processingPlane/interfaces/NIF_ElectronicCC.cc \
	electronicComponents/orion/ORION_Array_Internal.h \
	statistics/statistics.h \
	simCore/messages_m.h \
	electronicComponents/VirtualChannelControl.h \
	processingPlane/interfaces/NIF_Packet_Credit.h \
	addon/HotSpot-5.0/util.h \
	electronicComponents/orion/ORION_1_Params/SIM_power_test.h \
	statistics/LogFile.h \
	processingPlane/apps/SizeDistribution/SizeDistribution.h \
	electronicComponents/Arbiters/Arbiter.h \
	electronicComponents/orion/ORION_Config.h \
	processingPlane/DRAM_Cfg/DRAM_Cfg.h \
	simCore/NetworkAddress.h \
	electronicComponents/orion/ORION_1_Params/SIM_port.h \
	addon/HotSpot-5.0/flp.h \
	photonic/phylayer.h \
	addon/thermalmodel.h \
	electronicComponents/orion/ORION_Util.h \
	electronicComponents/orion/ORION_Arbiter.h \
	statistics/StatGroup.h \
	electronicComponents/orion/ORION_1_Params/SIM_power.h \
	electronicComponents/orion/ORION_Array_Info.h \
	processingPlane/interfaces/NIF.h \
	statistics/StatObject.h \
	addon/HotSpot-5.0/temperature.h \
	simCore/sim_includes.h \
	processingPlane/interfaces/NIF_Circuit.h \
	electronicComponents/Arbiters/ArbiterCanRoute.h \
	processingPlane/interfaces/NIF_ElectronicCC.h \
	processingPlane/addressTranslation/AddressTranslator.h \
	processingPlane/apps/Application.h \
	processingPlane/interfaces/NIF_Photonic.h \
	processingPlane/Processor.h \
	electronicComponents/orion/ORION_Array.h \
	electronicComponents/VC_Control.h \
	addon/HotSpot-5.0/temperature_grid.h
$O/processingPlane/interfaces/NIF.o: processingPlane/interfaces/NIF.cc \
	statistics/statistics.h \
	electronicComponents/orion/ORION_Array_Internal.h \
	simCore/messages_m.h \
	electronicComponents/VirtualChannelControl.h \
	addon/HotSpot-5.0/util.h \
	statistics/LogFile.h \
	electronicComponents/orion/ORION_1_Params/SIM_power_test.h \
	processingPlane/apps/SizeDistribution/SizeDistribution.h \
	electronicComponents/Arbiters/Arbiter.h \
	electronicComponents/orion/ORION_Config.h \
	processingPlane/DRAM_Cfg/DRAM_Cfg.h \
	simCore/packetstat.h \
	simCore/NetworkAddress.h \
	electronicComponents/orion/ORION_1_Params/SIM_port.h \
	addon/HotSpot-5.0/flp.h \
	photonic/phylayer.h \
	addon/thermalmodel.h \
	electronicComponents/orion/ORION_Util.h \
	electronicComponents/orion/ORION_Arbiter.h \
	statistics/StatGroup.h \
	electronicComponents/orion/ORION_1_Params/SIM_power.h \
	electronicComponents/orion/ORION_Array_Info.h \
	processingPlane/interfaces/NIF.h \
	addon/HotSpot-5.0/temperature.h \
	statistics/StatObject.h \
	simCore/sim_includes.h \
	electronicComponents/Arbiters/ArbiterCanRoute.h \
	processingPlane/addressTranslation/AddressTranslator.h \
	processingPlane/apps/Application.h \
	electronicComponents/orion/ORION_Array.h \
	processingPlane/Processor.h \
	addon/HotSpot-5.0/temperature_grid.h \
	electronicComponents/VC_Control.h
$O/processingPlane/interfaces/NIF_Circuit.o: processingPlane/interfaces/NIF_Circuit.cc \
	statistics/statistics.h \
	electronicComponents/orion/ORION_Array_Internal.h \
	simCore/messages_m.h \
	electronicComponents/VirtualChannelControl.h \
	processingPlane/interfaces/NIF_Packet_Credit.h \
	addon/HotSpot-5.0/util.h \
	statistics/LogFile.h \
	electronicComponents/orion/ORION_1_Params/SIM_power_test.h \
	processingPlane/apps/SizeDistribution/SizeDistribution.h \
	electronicComponents/Arbiters/Arbiter.h \
	electronicComponents/orion/ORION_Config.h \
	processingPlane/DRAM_Cfg/DRAM_Cfg.h \
	simCore/NetworkAddress.h \
	electronicComponents/orion/ORION_1_Params/SIM_port.h \
	addon/HotSpot-5.0/flp.h \
	photonic/phylayer.h \
	addon/thermalmodel.h \
	electronicComponents/orion/ORION_Arbiter.h \
	electronicComponents/orion/ORION_Util.h \
	statistics/StatGroup.h \
	electronicComponents/orion/ORION_1_Params/SIM_power.h \
	electronicComponents/orion/ORION_Array_Info.h \
	processingPlane/interfaces/NIF.h \
	addon/HotSpot-5.0/temperature.h \
	statistics/StatObject.h \
	simCore/sim_includes.h \
	processingPlane/interfaces/NIF_Circuit.h \
	electronicComponents/Arbiters/ArbiterCanRoute.h \
	processingPlane/addressTranslation/AddressTranslator.h \
	processingPlane/apps/Application.h \
	electronicComponents/orion/ORION_Array.h \
	processingPlane/Processor.h \
	addon/HotSpot-5.0/temperature_grid.h \
	electronicComponents/VC_Control.h
$O/simCore/packetstat.o: simCore/packetstat.cc \
	simCore/packetstat.h
$O/simCore/NetworkAddress.o: simCore/NetworkAddress.cc \
	simCore/NetworkAddress.h \
	simCore/messages_m.h
$O/simCore/convert.o: simCore/convert.cc \
	simCore/convert.h \
	simCore/messages_m.h
$O/simCore/messages_m.o: simCore/messages_m.cc \
	simCore/messages_m.h
$O/simCore/sim_includes.o: simCore/sim_includes.cc \
	statistics/StatObject.h \
	simCore/debug.h \
	simCore/sim_includes.h
$O/statistics/StatObject.o: statistics/StatObject.cc \
	statistics/StatObject.h
$O/statistics/statistics.o: statistics/statistics.cc \
	statistics/statistics.h \
	statistics/statObjects/energy/StatObject_EnergyEvent.h \
	statistics/statObjects/StatObject_Count.h \
	statistics/statObjects/StatObject_Total.h \
	statistics/StatGroup.h \
	statistics/statGroups/StatGroup_Sum.h \
	statistics/statObjects/StatObject_MMA.h \
	electronicComponents/orion/ORION_1_Params/SIM_power.h \
	statistics/statObjects/energy/StatObject_EnergyOn.h \
	electronicComponents/orion/ORION_1_Params/SIM_power_test.h \
	statistics/LogFile.h \
	statistics/StatObject.h \
	simCore/sim_includes.h \
	statistics/statObjects/StatObject_TimeAvg.h \
	electronicComponents/orion/ORION_Config.h \
	electronicComponents/orion/ORION_1_Params/SIM_port.h \
	photonic/phylayer.h \
	statistics/statObjects/energy/StatObject_EnergyStatic.h \
	statistics/statGroups/StatGroup_ListAll.h
$O/statistics/LogFile.o: statistics/LogFile.cc \
	statistics/LogFile.h \
	statistics/StatObject.h \
	statistics/StatGroup.h
$O/statistics/StatGroup.o: statistics/StatGroup.cc \
	statistics/StatObject.h \
	statistics/StatGroup.h
$O/statistics/statGroups/StatGroup_Sum.o: statistics/statGroups/StatGroup_Sum.cc \
	statistics/StatObject.h \
	statistics/StatGroup.h \
	statistics/statGroups/StatGroup_Sum.h
$O/statistics/statGroups/StatGroup_ListAll.o: statistics/statGroups/StatGroup_ListAll.cc \
	statistics/StatObject.h \
	statistics/StatGroup.h \
	statistics/statGroups/StatGroup_ListAll.h
$O/statistics/statObjects/StatObject_Total.o: statistics/statObjects/StatObject_Total.cc \
	statistics/StatObject.h \
	statistics/statObjects/StatObject_Total.h
$O/statistics/statObjects/StatObject_Count.o: statistics/statObjects/StatObject_Count.cc \
	statistics/StatObject.h \
	statistics/statObjects/StatObject_Count.h
$O/statistics/statObjects/StatObject_MMA.o: statistics/statObjects/StatObject_MMA.cc \
	statistics/StatObject.h \
	statistics/statObjects/StatObject_MMA.h
$O/statistics/statObjects/StatObject_TimeAvg.o: statistics/statObjects/StatObject_TimeAvg.cc \
	statistics/StatObject.h \
	statistics/statObjects/StatObject_TimeAvg.h
$O/statistics/statObjects/energy/StatObject_EnergyOn.o: statistics/statObjects/energy/StatObject_EnergyOn.cc \
	statistics/StatObject.h \
	statistics/statObjects/energy/StatObject_EnergyOn.h
$O/statistics/statObjects/energy/StatObject_EnergyEvent.o: statistics/statObjects/energy/StatObject_EnergyEvent.cc \
	statistics/StatObject.h \
	statistics/statObjects/energy/StatObject_EnergyEvent.h
$O/statistics/statObjects/energy/StatObject_EnergyStatic.o: statistics/statObjects/energy/StatObject_EnergyStatic.cc \
	statistics/StatObject.h \
	statistics/statObjects/energy/StatObject_EnergyStatic.h
$O/topologies/photonicTDM/ScheduleItem.o: topologies/photonicTDM/ScheduleItem.cc \
	simCore/messages_m.h \
	topologies/photonicTDM/ScheduleItem.h
$O/topologies/photonicTDM/ETDM/NIF_PhotonicETDM.o: topologies/photonicTDM/ETDM/NIF_PhotonicETDM.cc \
	electronicComponents/orion/ORION_Array_Internal.h \
	statistics/statistics.h \
	simCore/messages_m.h \
	electronicComponents/VirtualChannelControl.h \
	addon/HotSpot-5.0/util.h \
	topologies/photonicTDM/ETDM/NIF_PhotonicETDM.h \
	electronicComponents/orion/ORION_1_Params/SIM_power_test.h \
	statistics/LogFile.h \
	processingPlane/apps/SizeDistribution/SizeDistribution.h \
	electronicComponents/orion/ORION_Config.h \
	electronicComponents/Arbiters/Arbiter.h \
	processingPlane/DRAM_Cfg/DRAM_Cfg.h \
	electronicComponents/orion/ORION_1_Params/SIM_port.h \
	simCore/NetworkAddress.h \
	addon/HotSpot-5.0/flp.h \
	photonic/phylayer.h \
	addon/thermalmodel.h \
	electronicComponents/orion/ORION_Arbiter.h \
	electronicComponents/orion/ORION_Util.h \
	statistics/StatGroup.h \
	electronicComponents/orion/ORION_1_Params/SIM_power.h \
	electronicComponents/orion/ORION_Array_Info.h \
	processingPlane/interfaces/NIF.h \
	addon/HotSpot-5.0/temperature.h \
	statistics/StatObject.h \
	simCore/sim_includes.h \
	electronicComponents/Arbiters/ArbiterCanRoute.h \
	processingPlane/addressTranslation/AddressTranslator.h \
	processingPlane/apps/Application.h \
	processingPlane/Processor.h \
	electronicComponents/orion/ORION_Array.h \
	electronicComponents/VC_Control.h \
	addon/HotSpot-5.0/temperature_grid.h \
	topologies/photonicTDM/ScheduleItem.h
$O/topologies/photonicTDM/ETDM/ETDM_Switch_Controller.o: topologies/photonicTDM/ETDM/ETDM_Switch_Controller.cc \
	electronicComponents/orion/ORION_Array_Internal.h \
	statistics/statistics.h \
	electronicComponents/orion/ORION_Util.h \
	statistics/StatGroup.h \
	electronicComponents/orion/ORION_1_Params/SIM_power.h \
	simCore/messages_m.h \
	electronicComponents/orion/ORION_Array_Info.h \
	electronicComponents/orion/ORION_1_Params/SIM_power_test.h \
	statistics/LogFile.h \
	statistics/StatObject.h \
	simCore/sim_includes.h \
	electronicComponents/orion/ORION_Config.h \
	electronicComponents/orion/ORION_1_Params/SIM_port.h \
	topologies/photonicTDM/ETDM/ETDM_Switch_Controller.h \
	electronicComponents/orion/ORION_Link.h \
	electronicComponents/orion/ORION_Array.h \
	photonic/phylayer.h
$O/topologies/photonicTDM/TDM/NIF_PhotonicTDM.o: topologies/photonicTDM/TDM/NIF_PhotonicTDM.cc \
	statistics/statistics.h \
	electronicComponents/orion/ORION_Array_Internal.h \
	simCore/messages_m.h \
	electronicComponents/VirtualChannelControl.h \
	addon/HotSpot-5.0/util.h \
	statistics/LogFile.h \
	electronicComponents/orion/ORION_1_Params/SIM_power_test.h \
	processingPlane/apps/SizeDistribution/SizeDistribution.h \
	electronicComponents/orion/ORION_Config.h \
	electronicComponents/Arbiters/Arbiter.h \
	processingPlane/DRAM_Cfg/DRAM_Cfg.h \
	electronicComponents/orion/ORION_1_Params/SIM_port.h \
	simCore/NetworkAddress.h \
	addon/HotSpot-5.0/flp.h \
	photonic/phylayer.h \
	addon/thermalmodel.h \
	electronicComponents/orion/ORION_Arbiter.h \
	electronicComponents/orion/ORION_Util.h \
	statistics/StatGroup.h \
	electronicComponents/orion/ORION_1_Params/SIM_power.h \
	electronicComponents/orion/ORION_Array_Info.h \
	processingPlane/interfaces/NIF.h \
	topologies/photonicTDM/TDM/NIF_PhotonicTDM.h \
	addon/HotSpot-5.0/temperature.h \
	statistics/StatObject.h \
	simCore/sim_includes.h \
	electronicComponents/Arbiters/ArbiterCanRoute.h \
	processingPlane/addressTranslation/AddressTranslator.h \
	processingPlane/apps/Application.h \
	processingPlane/Processor.h \
	electronicComponents/orion/ORION_Array.h \
	addon/HotSpot-5.0/temperature_grid.h \
	electronicComponents/VC_Control.h \
	topologies/photonicTDM/ScheduleItem.h
$O/topologies/photonicTDM/TDM/TDM_Switch_Controller.o: topologies/photonicTDM/TDM/TDM_Switch_Controller.cc \
	electronicComponents/orion/ORION_Array_Internal.h \
	statistics/statistics.h \
	electronicComponents/orion/ORION_Util.h \
	statistics/StatGroup.h \
	electronicComponents/orion/ORION_1_Params/SIM_power.h \
	simCore/messages_m.h \
	electronicComponents/orion/ORION_Array_Info.h \
	electronicComponents/orion/ORION_1_Params/SIM_power_test.h \
	statistics/LogFile.h \
	statistics/StatObject.h \
	simCore/sim_includes.h \
	electronicComponents/orion/ORION_Config.h \
	electronicComponents/orion/ORION_1_Params/SIM_port.h \
	topologies/photonicTDM/TDM/TDM_Switch_Controller.h \
	electronicComponents/orion/ORION_Link.h \
	electronicComponents/orion/ORION_Array.h \
	photonic/phylayer.h

