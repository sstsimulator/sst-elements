# -*- Makefile -*-
#
#
# Pull in OMNeT++ configuration (Makefile.inc or configuser.vc)

OMNETPP_LIB_DIR = /home/scratch/omnetpp-4.1/lib
TOOLCHAIN_NAME = gcc
OMNETPP_LIB_SUBDIR = $(OMNETPP_LIB_DIR)/$(TOOLCHAIN_NAME)
KERNEL_LIBS = -loppsim
SYS_LIBS = -ldl -lstdc++
OMNETPP_LDFLAGS = -Wl,-rpath,$(OMNETPP_LIB_DIR) -Wl,-rpath,.
OMNETPP_CFLAGS = -O2 -DNDEBUG=1 -fno-stack-protector -fPIC -DXMLPARSER=libxml -DWITH_PARSIM -DWITH_NETBUILDER
OMNETPP_INCLUDE_DIR = /home/scratch/omnetpp-4.1/include

# -------
# list project-specific .h and .cc sources here
#    these will just be copied below
PROJECT_SOURCES = 

# list project-specific include paths here
#    these will just be copied below
#    note:  put -I before each path
PROJECT_PATHS =

# C++ include paths (with -I)
PHOENIXSIM_INCLUDE_PATH = \
    -I. \
    -IPhoenixSim/addon \
    -IPhoenixSim/addon/HotSpot-5.0 \
    -IPhoenixSim/chipComponents \
    -IPhoenixSim/data \
    -IPhoenixSim/electronicComponents \
    -IPhoenixSim/electronicComponents/Arbiters \
    -IPhoenixSim/electronicComponents/orion \
    -IPhoenixSim/electronicComponents/orion/ORION_1_Params \
    -IPhoenixSim/electronicComponents/orion/ORION_2_Params \
    -IPhoenixSim/images \
    -IPhoenixSim/images/components \
    -IPhoenixSim/images/components/old \
    -IPhoenixSim/images/mitucb \
    -IPhoenixSim/images/planes \
    -IPhoenixSim/ioComponents \
    -IPhoenixSim/ioComponents/DRAM_LRL \
    -IPhoenixSim/ioComponents/DRAMsim \
    -IPhoenixSim/ioComponents/DRAMsim/mem_system_def \
    -IPhoenixSim/parameters \
    -IPhoenixSim/parameters/Hendry-Thesis \
    -IPhoenixSim/parameters/default \
    -IPhoenixSim/photonic \
    -IPhoenixSim/photonic/PhotoCAD \
    -IPhoenixSim/photonic/devices \
    -IPhoenixSim/photonic/devices/machzehnder \
    -IPhoenixSim/photonic/devices/multiring \
    -IPhoenixSim/photonic/switches \
    -IPhoenixSim/photonic/switches/machzehnder \
    -IPhoenixSim/photonic/switches/multiring \
    -IPhoenixSim/processingPlane \
    -IPhoenixSim/processingPlane/DRAM_Cfg \
    -IPhoenixSim/processingPlane/addressTranslation \
    -IPhoenixSim/processingPlane/apps \
    -IPhoenixSim/processingPlane/apps/DRAM \
    -IPhoenixSim/processingPlane/apps/MITUCB \
    -IPhoenixSim/processingPlane/apps/SizeDistribution \
    -IPhoenixSim/processingPlane/apps/model \
    -IPhoenixSim/processingPlane/apps/synthetics \
    -IPhoenixSim/processingPlane/apps/test \
    -IPhoenixSim/processingPlane/apps/trace \
    -IPhoenixSim/processingPlane/interfaces \
    -IPhoenixSim/results \
    -IPhoenixSim/simCore \
    -IPhoenixSim/statistics \
    -IPhoenixSim/statistics/statGroups \
    -IPhoenixSim/statistics/statObjects \
    -IPhoenixSim/statistics/statObjects/energy \
    -IPhoenixSim/topologies \
    -IPhoenixSim/topologies/CMesh \
    -IPhoenixSim/topologies/blockingTorus \
    -IPhoenixSim/topologies/electronicMesh \
    -IPhoenixSim/topologies/electronicMesh_Circuit \
    -IPhoenixSim/topologies/electronicTorus \
    -IPhoenixSim/topologies/flattenedButterfly \
    -IPhoenixSim/topologies/nonblockingSiliconNitride \
    -IPhoenixSim/topologies/nonblockingTorus \
    -IPhoenixSim/topologies/photonicMesh \
    -IPhoenixSim/topologies/photonicTDM \
    -IPhoenixSim/topologies/photonicTDM/ETDM \
    -IPhoenixSim/topologies/photonicTDM/TDM \
    -IPhoenixSim/topologies/squareRoot \
    -IPhoenixSim/topologies/switches \
    -IPhoenixSim/topologies/switches/crossbar \
    -IPhoenixSim/topologies/switches/machzehndercrossbar \
    -IPhoenixSim/topologies/torusNX

AM_CPPFLAGS = \
	$(BOOST_CPPFLAGS) \
	$(MPI_CPPFLAGS) \
	$(OMNETPP_CFLAGS) \
	-I$(OMNETPP_INCLUDE_DIR) \
	$(PHOENIXSIM_INCLUDE_PATH) \
	$(PROJECT_PATHS)

compdir = $(pkglibdir)
comp_LTLIBRARIES = libPhoenixSim.la
libPhoenixSim_la_SOURCES = \
	PhoenixSim.h \
	PhoenixSim.cc \
	omnetconfiguration.h omnetconfiguration.cc \
	omnetEvent.h \
	cconfigreader.h \
	platdefs.h \
	timeutil.h \
	envir/inifilereader.h envir/inifilereader.cc \
	common/stringpool.h common/stringpool.cc \
	common/patternmatcher.h common/patternmatcher.cc \
	common/commondefs.h \
	common/opp_ctype.h \
	common/stringutil.h common/stringutil.cc \
	common/exception.h common/exception.cc \
	common/commonutil.h common/commonutil.cc \
	common/stringtokenizer.h common/stringtokenizer.cc \
	common/fileglobber.h common/fileglobber.cc \
	envir/valueiterator.h envir/valueiterator.cc \
	envir/scenario.h envir/scenario.cc \
	envir/envirdefs.h \
	common/expression.h common/expression.cc \
	common/unitconversion.h common/unitconversion.cc \
	common/fileutil.h common/fileutil.cc \
	envir/fsutils.h envir/fsutils.cc \
	common/enumstr.h common/enumstr.cc \
	nullenvir.h \
	simEnv.h simEnv.cc \
	envir/eventlogfilemgr.h envir/eventlogfilemgr.cc \
	envir/objectprinter.h envir/objectprinter.cc \
	envir/intervals.h envir/intervals.cc \
	envir/eventlogwriter.h envir/eventlogwriter.cc \
	common/matchexpression.h common/matchexpression.cc \
	envir/matchablefield.h envir/matchablefield.cc \
	envir/matchableobject.h envir/matchableobject.cc \
	envir/resultfilters.h envir/resultfilters.cc \
	envir/resultfilter.h envir/resultfilter.cc \
	envir/resultlistener.h envir/resultlistener.cc \
	envir/resultrecorder.h envir/resultrecorder.cc \
	common/ver.h \
	envir/indexedfileoutvectormgr.h envir/indexedfileoutvectormgr.cc \
	envir/fileoutvectormgr.h envir/fileoutvectormgr.cc \
	envir/runattributes.h envir/runattributes.cc \
	envir/fileoutscalarmgr.h envir/fileoutscalarmgr.cc \
	envir/filesnapshotmgr.h envir/filesnapshotmgr.cc \
	envir/cxmldoccache.h envir/cxmldoccache.cc \
	envir/speedometer.h envir/speedometer.cc \
	nedxml/saxparser.h \
	nedxml/saxparser_none.cc \
	nedxml/nederror.h nedxml/nederror.cc \
	nedxml/nedxmldefs.h \
	nedxml/nedelement.h nedxml/nedelement.cc \
	nedxml/nedexception.h nedxml/nedexception.cc \
	nedxml/nedparser.h nedxml/nedparser.cc \
	nedxml/nedyydefs.h \
	nedxml/nedfilebuffer.h nedxml/nedfilebuffer.cc \
	nedxml/nedyylib.h nedxml/nedyylib.cc \
	parsim/ccommbufferbase.h parsim/ccommbufferbase.cc \
	parsim/cfilecommbuffer.h parsim/cfilecommbuffer.cc \
	parsim/cfilecomm.h parsim/cfilecomm.cc \
	parsim/cidealsimulationprot.h parsim/cidealsimulationprot.cc \
	parsim/cispeventlogger.h parsim/cispeventlogger.cc \
	parsim/clinkdelaylookahead.h parsim/clinkdelaylookahead.cc \
	parsim/cmemcommbuffer.h parsim/cmemcommbuffer.cc \
	parsim/cmpicommbuffer.h parsim/cmpicommbuffer.cc \
	parsim/cmpicomm.h parsim/cmpicomm.cc \
	parsim/cnamedpipecomm.h parsim/cnamedpipecomm.cc \
	parsim/cnamedpipecomm-win.cc \
	parsim/cnmplookahead.h \
	parsim/cnosynchronization.h parsim/cnosynchronization.cc \
	parsim/cnullmessageprot.h parsim/cnullmessageprot.cc \
	parsim/cparsimpartition.h parsim/cparsimpartition.cc \
	parsim/cparsimprotocolbase.h parsim/cparsimprotocolbase.cc \
	parsim/cparsimsynchr.h \
	parsim/cplaceholdermod.h parsim/cplaceholdermod.cc \
	parsim/cproxygate.h parsim/cproxygate.cc \
	parsim/creceivedexception.h parsim/creceivedexception.cc \
	parsim/messagetags.h \
	parsim/parsimindex.h \
	parsim/parsimutil.h parsim/parsimutil.cc\
	PhoenixSim/processingPlane/interfaces/NIF.h\
	PhoenixSim/processingPlane/interfaces/NIF_Packet_Credit.h\
	PhoenixSim/processingPlane/interfaces/NIF_Photonic.h\
	PhoenixSim/processingPlane/interfaces/NIF_Electronic.h\
	PhoenixSim/processingPlane/interfaces/NIF_ElectronicCC.h\
	PhoenixSim/processingPlane/interfaces/NIF_Circuit.h\
	PhoenixSim/processingPlane/Processor.h\
	PhoenixSim/processingPlane/addressTranslation/AddressTranslator_Standard.h\
	PhoenixSim/processingPlane/addressTranslation/AddressTranslator.h\
	PhoenixSim/processingPlane/addressTranslation/AddressTranslator_SquareRoot.h\
	PhoenixSim/processingPlane/addressTranslation/AddressTranslator_FatTree.h\
	PhoenixSim/processingPlane/apps/synthetics/AppRandom.h\
	PhoenixSim/processingPlane/apps/synthetics/AppTornado.h\
	PhoenixSim/processingPlane/apps/synthetics/AppNeighbor.h\
	PhoenixSim/processingPlane/apps/synthetics/AppRandom_FiniteQueue.h\
	PhoenixSim/processingPlane/apps/synthetics/AppBitreverse.h\
	PhoenixSim/processingPlane/apps/synthetics/AppHotspot.h\
	PhoenixSim/processingPlane/apps/test/AppOne2One.h\
	PhoenixSim/processingPlane/apps/test/AppBcastTest.h\
	PhoenixSim/processingPlane/apps/test/AppAll2All.h\
	PhoenixSim/processingPlane/apps/MITUCB/App_MITUCB_All2All.h\
	PhoenixSim/processingPlane/apps/MITUCB/App_MITUCB_Random.h\
	PhoenixSim/processingPlane/apps/DRAM/AppDRAMAll2One.h\
	PhoenixSim/processingPlane/apps/DRAM/AppDRAMRandom.h\
	PhoenixSim/processingPlane/apps/DRAM/AppDRAMOne2One.h\
	PhoenixSim/processingPlane/apps/DRAM/AppDRAMOne2All.h\
	PhoenixSim/processingPlane/apps/trace/App_LL_DepGraph.h\
	PhoenixSim/processingPlane/apps/trace/AppTrace_LBL.h\
	PhoenixSim/processingPlane/apps/model/AppFFT.h\
	PhoenixSim/processingPlane/apps/model/AppDataFlow.h\
	PhoenixSim/processingPlane/apps/model/AppFFTstream.h\
	PhoenixSim/processingPlane/apps/SizeDistribution/SizeDistribution_Constant.h\
	PhoenixSim/processingPlane/apps/SizeDistribution/SizeDistribution_InvGamma.h\
	PhoenixSim/processingPlane/apps/SizeDistribution/SizeDistribution_Gamma.h\
	PhoenixSim/processingPlane/apps/SizeDistribution/SizeDistribution_Normal.h\
	PhoenixSim/processingPlane/apps/SizeDistribution/SizeDistribution_Control.h\
	PhoenixSim/processingPlane/apps/SizeDistribution/SizeDistribution.h\
	PhoenixSim/processingPlane/apps/Application.h\
	PhoenixSim/processingPlane/ProcessorRouterArbiter.h\
	PhoenixSim/processingPlane/DRAM_Cfg/DRAM_Cfg_AllNodes.h\
	PhoenixSim/processingPlane/DRAM_Cfg/DRAM_Cfg_Periphery.h\
	PhoenixSim/processingPlane/DRAM_Cfg/DRAM_Cfg_Periphery_SR16x16.h\
	PhoenixSim/processingPlane/DRAM_Cfg/DRAM_Cfg.h\
	PhoenixSim/processingPlane/DRAM_Cfg/DRAM_Cfg_Periphery_SR8x8.h\
	PhoenixSim/processingPlane/DRAM_Cfg/DRAM_Cfg_Periphery_SR4x4.h\
	PhoenixSim/processingPlane/GatewaySignalControl.h\
	PhoenixSim/ioComponents/DRAMsim/BIU.h\
	PhoenixSim/ioComponents/DRAMsim/TransactionQueue.h\
	PhoenixSim/ioComponents/DRAMsim/Aux_Stat.h\
	PhoenixSim/ioComponents/DRAMsim/Bank.h\
	PhoenixSim/ioComponents/DRAMsim/PowerMeasure.h\
	PhoenixSim/ioComponents/DRAMsim/DRAM_Sim.h\
	PhoenixSim/ioComponents/DRAMsim/PendingInfo.h\
	PhoenixSim/ioComponents/DRAMsim/Command.h\
	PhoenixSim/ioComponents/DRAMsim/Memory_Controller.h\
	PhoenixSim/ioComponents/DRAMsim/Transaction.h\
	PhoenixSim/ioComponents/DRAMsim/utilities.h\
	PhoenixSim/ioComponents/DRAMsim/RefreshQueue.h\
	PhoenixSim/ioComponents/DRAMsim/RBRR.h\
	PhoenixSim/ioComponents/DRAMsim/DRAM_config.h\
	PhoenixSim/ioComponents/DRAMsim/BIU_slot.h\
	PhoenixSim/ioComponents/DRAMsim/RowBuffer.h\
	PhoenixSim/ioComponents/DRAMsim/PowerConfig.h\
	PhoenixSim/ioComponents/DRAMsim/Rank.h\
	PhoenixSim/ioComponents/DRAMsim/DRAMaddress.h\
	PhoenixSim/ioComponents/DRAMsim/BusEvent.h\
	PhoenixSim/ioComponents/DRAMsim/PendingQueue.h\
	PhoenixSim/ioComponents/DRAMsim/DRAM.h\
	PhoenixSim/ioComponents/DRAMsim/UpdateInterface.h\
	PhoenixSim/ioComponents/DRAMsim/constants.h\
	PhoenixSim/ioComponents/DRAMsim/Bus.h\
	PhoenixSim/ioComponents/DRAMsim/PowerCounter.h\
	PhoenixSim/ioComponents/DRAMsim/Bundle.h\
	PhoenixSim/ioComponents/DRAMsim/DIMM.h\
	PhoenixSim/ioComponents/DRAMsim/Global_TQ_info.h\
	PhoenixSim/ioComponents/DRAMsim/PrefetchCache.h\
	PhoenixSim/ioComponents/DRAM_LRL/OCM_Control.h\
	PhoenixSim/ioComponents/DRAM_LRL/MemoryControl.h\
	PhoenixSim/ioComponents/DRAM_LRL/DRAM_Bank.h\
	PhoenixSim/ioComponents/DRAM_LRL/NIF_MemGW.h\
	PhoenixSim/ioComponents/DRAM_LRL/ProcessorDataStripper.h\
	PhoenixSim/ioComponents/cDramModule.h\
	PhoenixSim/simCore/packetstat.h\
	PhoenixSim/simCore/debug.h\
	PhoenixSim/simCore/sim_includes.h\
	PhoenixSim/simCore/NetworkAddress.h\
	PhoenixSim/simCore/messages_m.h\
	PhoenixSim/simCore/convert.h\
	PhoenixSim/addon/thermalmodel.h\
	PhoenixSim/addon/HotSpot-5.0/wire.h\
	PhoenixSim/addon/HotSpot-5.0/hotspot-iface.h\
	PhoenixSim/addon/HotSpot-5.0/package.h\
	PhoenixSim/addon/HotSpot-5.0/temperature.h\
	PhoenixSim/addon/HotSpot-5.0/util.h\
	PhoenixSim/addon/HotSpot-5.0/hotspot.h\
	PhoenixSim/addon/HotSpot-5.0/flp.h\
	PhoenixSim/addon/HotSpot-5.0/hotfloorplan.h\
	PhoenixSim/addon/HotSpot-5.0/npe.h\
	PhoenixSim/addon/HotSpot-5.0/shape.h\
	PhoenixSim/addon/HotSpot-5.0/temperature_block.h\
	PhoenixSim/addon/HotSpot-5.0/temperature_grid.h\
	PhoenixSim/topologies/photonicTDM/ScheduleItem.h\
	PhoenixSim/topologies/photonicTDM/TDM/TDM_Switch_Controller.h\
	PhoenixSim/topologies/photonicTDM/TDM/NIF_PhotonicTDM.h\
	PhoenixSim/topologies/photonicTDM/ETDM/NIF_PhotonicETDM.h\
	PhoenixSim/topologies/photonicTDM/ETDM/ETDM_Switch_Controller.h\
	PhoenixSim/electronicComponents/RouterStat.h\
	PhoenixSim/electronicComponents/mux.h\
	PhoenixSim/electronicComponents/RouterArbiter.h\
	PhoenixSim/electronicComponents/electronicIOpad.h\
	PhoenixSim/electronicComponents/electronic_channel.h\
	PhoenixSim/electronicComponents/orion/ORION_1_Params/SIM_power.h\
	PhoenixSim/electronicComponents/orion/ORION_1_Params/SIM_power_test.h\
	PhoenixSim/electronicComponents/orion/ORION_1_Params/SIM_port.h\
	PhoenixSim/electronicComponents/orion/ORION_Array.h\
	PhoenixSim/electronicComponents/orion/ORION_Crossbar.h\
	PhoenixSim/electronicComponents/orion/ORION_2_Params/ORION_port.h\
	PhoenixSim/electronicComponents/orion/ORION_2_Params/ORION_Technology_v1.h\
	PhoenixSim/electronicComponents/orion/ORION_2_Params/ORION_Technology.h\
	PhoenixSim/electronicComponents/orion/ORION_2_Params/ORION_Technology_v2.h\
	PhoenixSim/electronicComponents/orion/ORION_Link.h\
	PhoenixSim/electronicComponents/orion/ORION_Config.h\
	PhoenixSim/electronicComponents/orion/ORION_Array_Info.h\
	PhoenixSim/electronicComponents/orion/ORION_Arbiter.h\
	PhoenixSim/electronicComponents/orion/ORION_Util.h\
	PhoenixSim/electronicComponents/orion/ORION_Array_Internal.h\
	PhoenixSim/electronicComponents/VC_Control.h\
	PhoenixSim/electronicComponents/VirtualChannelControl.h\
	PhoenixSim/electronicComponents/RouterCrossbar.h\
	PhoenixSim/electronicComponents/Arbiters/Arbiter_TNX_Gateway.h\
	PhoenixSim/electronicComponents/Arbiters/Arbiter_SR.h\
	PhoenixSim/electronicComponents/Arbiters/Photonic4x4Arbiter.h\
	PhoenixSim/electronicComponents/Arbiters/Arbiter_PR.h\
	PhoenixSim/electronicComponents/Arbiters/PhotonicNoUturnArbiter.h\
	PhoenixSim/electronicComponents/Arbiters/Arbiter_FT_Root.h\
	PhoenixSim/electronicComponents/Arbiters/Arbiter_NBT_Gateway.h\
	PhoenixSim/electronicComponents/Arbiters/Arbiter_SR8x8.h\
	PhoenixSim/electronicComponents/Arbiters/Arbiter_BT_EJ.h\
	PhoenixSim/electronicComponents/Arbiters/Arbiter_SR_Middle.h\
	PhoenixSim/electronicComponents/Arbiters/Arbiter_OM4x4.h\
	PhoenixSim/electronicComponents/Arbiters/Arbiter_FB.h\
	PhoenixSim/electronicComponents/Arbiters/Arbiter_MITUCB_MeshNode.h\
	PhoenixSim/electronicComponents/Arbiters/Arbiter_SR4x4.h\
	PhoenixSim/electronicComponents/Arbiters/Arbiter_BT_4x4.h\
	PhoenixSim/electronicComponents/Arbiters/Arbiter_BT_INJ.h\
	PhoenixSim/electronicComponents/Arbiters/Arbiter_EHS.h\
	PhoenixSim/electronicComponents/Arbiters/Arbiter_FT.h\
	PhoenixSim/electronicComponents/Arbiters/Arbiter_SR_Quad.h\
	PhoenixSim/electronicComponents/Arbiters/Arbiter_TNX_Node.h\
	PhoenixSim/electronicComponents/Arbiters/Arbiter_CM.h\
	PhoenixSim/electronicComponents/Arbiters/ElectronicArbiter.h\
	PhoenixSim/electronicComponents/Arbiters/Arbiter_NBT_Node_SP.h\
	PhoenixSim/electronicComponents/Arbiters/Arbiter_MITUCB_MemoryNode.h\
	PhoenixSim/electronicComponents/Arbiters/Arbiter_EM_circuit.h\
	PhoenixSim/electronicComponents/Arbiters/Arbiter_GWC.h\
	PhoenixSim/electronicComponents/Arbiters/Arbiter_PC.h\
	PhoenixSim/electronicComponents/Arbiters/Arbiter_SR_Top.h\
	PhoenixSim/electronicComponents/Arbiters/Arbiter_SR16x16.h\
	PhoenixSim/electronicComponents/Arbiters/Arbiter_EM.h\
	PhoenixSim/electronicComponents/Arbiters/Arbiter_ET.h\
	PhoenixSim/electronicComponents/Arbiters/Arbiter_BT_GWS.h\
	PhoenixSim/electronicComponents/Arbiters/PhotonicArbiter.h\
	PhoenixSim/electronicComponents/Arbiters/ArbiterCanRoute.h\
	PhoenixSim/electronicComponents/Arbiters/Arbiter_PM_Node.h\
	PhoenixSim/electronicComponents/Arbiters/Arbiter_Crossbar.h\
	PhoenixSim/electronicComponents/Arbiters/Arbiter_XB_Gateway.h\
	PhoenixSim/electronicComponents/Arbiters/Arbiter.h\
	PhoenixSim/electronicComponents/Arbiters/Arbiter_FT_GWC.h\
	PhoenixSim/electronicComponents/Arbiters/Arbiter_NBT_Node.h\
	PhoenixSim/electronicComponents/Arbiters/Arbiter_XB_Node.h\
	PhoenixSim/electronicComponents/Arbiters/Arbiter_NBSiN.h\
	PhoenixSim/electronicComponents/serializer.h\
	PhoenixSim/electronicComponents/RouterInport.h\
	PhoenixSim/chipComponents/InoutConverter.h\
	PhoenixSim/statistics/StatObject.h\
	PhoenixSim/statistics/statGroups/StatGroup_Sum.h\
	PhoenixSim/statistics/statGroups/StatGroup_ListAll.h\
	PhoenixSim/statistics/statObjects/energy/StatObject_EnergyOn.h\
	PhoenixSim/statistics/statObjects/energy/StatObject_EnergyStatic.h\
	PhoenixSim/statistics/statObjects/energy/StatObject_EnergyEvent.h\
	PhoenixSim/statistics/statObjects/StatObject_Total.h\
	PhoenixSim/statistics/statObjects/StatObject_TimeAvg.h\
	PhoenixSim/statistics/statObjects/StatObject_Count.h\
	PhoenixSim/statistics/statObjects/StatObject_MMA.h\
	PhoenixSim/statistics/StatGroup.h\
	PhoenixSim/statistics/LogFile.h\
	PhoenixSim/statistics/statistics.h\
	PhoenixSim/photonic/activeelement.h\
	PhoenixSim/photonic/domainchanger.h\
	PhoenixSim/photonic/resonanceprofile.h\
	PhoenixSim/photonic/activeringelement.h\
	PhoenixSim/photonic/modulatorelement.h\
	PhoenixSim/photonic/machzehnderelement.h\
	PhoenixSim/photonic/activetworingelement.h\
	PhoenixSim/photonic/detectorelement.h\
	PhoenixSim/photonic/basicelement.h\
	PhoenixSim/photonic/phylayer.h\
	PhoenixSim/photonic/ringelement.h\
	PhoenixSim/photonic/devices/pse1x2nx.h\
	PhoenixSim/photonic/devices/facet.h\
	PhoenixSim/photonic/devices/detector.h\
	PhoenixSim/photonic/devices/laser.h\
	PhoenixSim/photonic/devices/SwitchingFilter.h\
	PhoenixSim/photonic/devices/filter1x2.h\
	PhoenixSim/photonic/devices/pse1x2.h\
	PhoenixSim/photonic/devices/StarCoupler.h\
	PhoenixSim/photonic/devices/cross.h\
	PhoenixSim/photonic/devices/modulator.h\
	PhoenixSim/photonic/devices/pse2x2.h\
	PhoenixSim/photonic/devices/line.h\
	PhoenixSim/photonic/devices/activedirectionalcoupler.h\
	PhoenixSim/photonic/devices/attenuator.h\
	PhoenixSim/photonic/devices/multiring/pse2x2r2.h\
	PhoenixSim/photonic/devices/multiring/pse2x2r3.h\
	PhoenixSim/photonic/devices/multiring/pse1x2nxr2.h\
	PhoenixSim/photonic/devices/multiring/pse1x2r3.h\
	PhoenixSim/photonic/devices/multiring/pse1x2multiring.h\
	PhoenixSim/photonic/devices/multiring/pse1x2r2.h\
	PhoenixSim/photonic/devices/multiring/pse2x2multiring.h\
	PhoenixSim/photonic/devices/multiring/pse1x2nxr3.h\
	PhoenixSim/photonic/devices/multiring/pse1x2nxmultiring.h\
	PhoenixSim/photonic/devices/join.h\
	PhoenixSim/photonic/devices/bend.h\
	PhoenixSim/photonic/devices/end.h\
	PhoenixSim/photonic/devices/machzehnder/mzswitch1x2.h\
	PhoenixSim/photonic/devices/machzehnder/mzswitch2x2.h\
	PhoenixSim/photonic/activemultiringelement.h\
	PhoenixSim/photonic/tworingelement.h\
	PhoenixSim/photonic/multiringelement.h\
	PhoenixSim/processingPlane/ProcessorRouterArbiter.cc\
	PhoenixSim/processingPlane/interfaces/NIF.cc\
	PhoenixSim/processingPlane/interfaces/NIF_Packet_Credit.cc\
	PhoenixSim/processingPlane/interfaces/NIF_Circuit.cc\
	PhoenixSim/processingPlane/interfaces/NIF_Electronic.cc\
	PhoenixSim/processingPlane/interfaces/NIF_Photonic.cc\
	PhoenixSim/processingPlane/interfaces/NIF_ElectronicCC.cc\
	PhoenixSim/processingPlane/Processor.cc\
	PhoenixSim/processingPlane/addressTranslation/AddressTranslator_SquareRoot.cc\
	PhoenixSim/processingPlane/addressTranslation/AddressTranslator.cc\
	PhoenixSim/processingPlane/addressTranslation/AddressTranslator_FatTree.cc\
	PhoenixSim/processingPlane/addressTranslation/AddressTranslator_Standard.cc\
	PhoenixSim/processingPlane/apps/synthetics/AppBitreverse.cc\
	PhoenixSim/processingPlane/apps/synthetics/AppHotspot.cc\
	PhoenixSim/processingPlane/apps/synthetics/AppRandom_FiniteQueue.cc\
	PhoenixSim/processingPlane/apps/synthetics/AppNeighbor.cc\
	PhoenixSim/processingPlane/apps/synthetics/AppTornado.cc\
	PhoenixSim/processingPlane/apps/synthetics/AppRandom.cc\
	PhoenixSim/processingPlane/apps/Application.cc\
	PhoenixSim/processingPlane/apps/test/AppAll2All.cc\
	PhoenixSim/processingPlane/apps/test/AppOne2One.cc\
	PhoenixSim/processingPlane/apps/test/AppBcastTest.cc\
	PhoenixSim/processingPlane/apps/MITUCB/App_MITUCB_Random.cc\
	PhoenixSim/processingPlane/apps/MITUCB/App_MITUCB_All2All.cc\
	PhoenixSim/processingPlane/apps/DRAM/AppDRAMOne2One.cc\
	PhoenixSim/processingPlane/apps/DRAM/AppDRAMOne2All.cc\
	PhoenixSim/processingPlane/apps/DRAM/AppDRAMRandom.cc\
	PhoenixSim/processingPlane/apps/DRAM/AppDRAMAll2One.cc\
	PhoenixSim/processingPlane/apps/trace/App_LL_DepGraph.cc\
	PhoenixSim/processingPlane/apps/trace/AppTrace_LBL.cc\
	PhoenixSim/processingPlane/apps/model/AppFFTstream.cc\
	PhoenixSim/processingPlane/apps/model/AppFFT.cc\
	PhoenixSim/processingPlane/apps/model/AppDataFlow.cc\
	PhoenixSim/processingPlane/apps/SizeDistribution/SizeDistribution_Control.cc\
	PhoenixSim/processingPlane/apps/SizeDistribution/SizeDistribution.cc\
	PhoenixSim/processingPlane/apps/SizeDistribution/SizeDistribution_InvGamma.cc\
	PhoenixSim/processingPlane/apps/SizeDistribution/SizeDistribution_Constant.cc\
	PhoenixSim/processingPlane/apps/SizeDistribution/SizeDistribution_Gamma.cc\
	PhoenixSim/processingPlane/apps/SizeDistribution/SizeDistribution_Normal.cc\
	PhoenixSim/processingPlane/GatewaySignalControl.cc\
	PhoenixSim/processingPlane/DRAM_Cfg/DRAM_Cfg_AllNodes.cc\
	PhoenixSim/processingPlane/DRAM_Cfg/DRAM_Cfg_Periphery.cc\
	PhoenixSim/processingPlane/DRAM_Cfg/DRAM_Cfg_Periphery_SR16x16.cc\
	PhoenixSim/processingPlane/DRAM_Cfg/DRAM_Cfg_Periphery_SR8x8.cc\
	PhoenixSim/processingPlane/DRAM_Cfg/DRAM_Cfg.cc\
	PhoenixSim/processingPlane/DRAM_Cfg/DRAM_Cfg_Periphery_SR4x4.cc\
	PhoenixSim/ioComponents/DRAMsim/Global_TQ_info.cc\
	PhoenixSim/ioComponents/DRAMsim/utilities.cc\
	PhoenixSim/ioComponents/DRAMsim/DRAMaddress.cc\
	PhoenixSim/ioComponents/DRAMsim/PrefetchCache.cc\
	PhoenixSim/ioComponents/DRAMsim/Command.cc\
	PhoenixSim/ioComponents/DRAMsim/Transaction.cc\
	PhoenixSim/ioComponents/DRAMsim/PowerConfig.cc\
	PhoenixSim/ioComponents/DRAMsim/PendingQueue.cc\
	PhoenixSim/ioComponents/DRAMsim/DIMM.cc\
	PhoenixSim/ioComponents/DRAMsim/RefreshQueue.cc\
	PhoenixSim/ioComponents/DRAMsim/DRAM_config.cc\
	PhoenixSim/ioComponents/DRAMsim/RBRR.cc\
	PhoenixSim/ioComponents/DRAMsim/Aux_Stat.cc\
	PhoenixSim/ioComponents/DRAMsim/BIU.cc\
	PhoenixSim/ioComponents/DRAMsim/TransactionQueue.cc\
	PhoenixSim/ioComponents/DRAMsim/DRAM_Sim.cc\
	PhoenixSim/ioComponents/DRAMsim/Memory_Controller.cc\
	PhoenixSim/ioComponents/DRAMsim/DRAM.cc\
	PhoenixSim/ioComponents/DRAM_LRL/OCM_Control.cc\
	PhoenixSim/ioComponents/DRAM_LRL/NIF_MemGW.cc\
	PhoenixSim/ioComponents/DRAM_LRL/ProcessorDataStripper.cc\
	PhoenixSim/ioComponents/DRAM_LRL/DRAM_Bank.cc\
	PhoenixSim/ioComponents/DRAM_LRL/MemoryControl.cc\
	PhoenixSim/ioComponents/cDramModule.cc\
	PhoenixSim/simCore/NetworkAddress.cc\
	PhoenixSim/simCore/convert.cc\
	PhoenixSim/simCore/packetstat.cc\
	PhoenixSim/simCore/messages_m.cc\
	PhoenixSim/simCore/sim_includes.cc\
	PhoenixSim/addon/thermalmodel.cc\
	PhoenixSim/topologies/photonicTDM/ScheduleItem.cc\
	PhoenixSim/topologies/photonicTDM/TDM/NIF_PhotonicTDM.cc\
	PhoenixSim/topologies/photonicTDM/TDM/TDM_Switch_Controller.cc\
	PhoenixSim/topologies/photonicTDM/ETDM/ETDM_Switch_Controller.cc\
	PhoenixSim/topologies/photonicTDM/ETDM/NIF_PhotonicETDM.cc\
	PhoenixSim/electronicComponents/RouterInport.cc\
	PhoenixSim/electronicComponents/mux.cc\
	PhoenixSim/electronicComponents/VirtualChannelControl.cc\
	PhoenixSim/electronicComponents/RouterCrossbar.cc\
	PhoenixSim/electronicComponents/serializer.cc\
	PhoenixSim/electronicComponents/electronicIOpad.cc\
	PhoenixSim/electronicComponents/RouterArbiter.cc\
	PhoenixSim/electronicComponents/orion/ORION_Array_Info.cc\
	PhoenixSim/electronicComponents/orion/ORION_Config.cc\
	PhoenixSim/electronicComponents/orion/ORION_Arbiter.cc\
	PhoenixSim/electronicComponents/orion/ORION_Util.cc\
	PhoenixSim/electronicComponents/orion/ORION_Array_Internal.cc\
	PhoenixSim/electronicComponents/orion/ORION_Link.cc\
	PhoenixSim/electronicComponents/orion/ORION_Crossbar.cc\
	PhoenixSim/electronicComponents/orion/ORION_Array.cc\
	PhoenixSim/electronicComponents/RouterStat.cc\
	PhoenixSim/electronicComponents/electronic_channel.cc\
	PhoenixSim/electronicComponents/VC_Control.cc\
	PhoenixSim/electronicComponents/Arbiters/Arbiter_FB.cc\
	PhoenixSim/electronicComponents/Arbiters/Arbiter_BT_EJ.cc\
	PhoenixSim/electronicComponents/Arbiters/Arbiter_EHS.cc\
	PhoenixSim/electronicComponents/Arbiters/Arbiter_SR.cc\
	PhoenixSim/electronicComponents/Arbiters/Arbiter_PR.cc\
	PhoenixSim/electronicComponents/Arbiters/Arbiter_FT_Root.cc\
	PhoenixSim/electronicComponents/Arbiters/Arbiter_MITUCB_MeshNode.cc\
	PhoenixSim/electronicComponents/Arbiters/ArbiterCanRoute.cc\
	PhoenixSim/electronicComponents/Arbiters/Arbiter_BT_INJ.cc\
	PhoenixSim/electronicComponents/Arbiters/Arbiter_TNX_Node.cc\
	PhoenixSim/electronicComponents/Arbiters/ElectronicArbiter.cc\
	PhoenixSim/electronicComponents/Arbiters/Arbiter_XB_Node.cc\
	PhoenixSim/electronicComponents/Arbiters/Arbiter_NBT_Node_SP.cc\
	PhoenixSim/electronicComponents/Arbiters/Arbiter_NBSiN.cc\
	PhoenixSim/electronicComponents/Arbiters/PhotonicNoUturnArbiter.cc\
	PhoenixSim/electronicComponents/Arbiters/Arbiter_OM4x4.cc\
	PhoenixSim/electronicComponents/Arbiters/Photonic4x4Arbiter.cc\
	PhoenixSim/electronicComponents/Arbiters/Arbiter_FT_GWC.cc\
	PhoenixSim/electronicComponents/Arbiters/Arbiter_PM_Node.cc\
	PhoenixSim/electronicComponents/Arbiters/Arbiter_SR_Quad.cc\
	PhoenixSim/electronicComponents/Arbiters/Arbiter_TNX_Gateway.cc\
	PhoenixSim/electronicComponents/Arbiters/Arbiter_FT.cc\
	PhoenixSim/electronicComponents/Arbiters/Arbiter_SR_Middle.cc\
	PhoenixSim/electronicComponents/Arbiters/Arbiter_SR16x16.cc\
	PhoenixSim/electronicComponents/Arbiters/Arbiter_BT_GWS.cc\
	PhoenixSim/electronicComponents/Arbiters/PhotonicArbiter.cc\
	PhoenixSim/electronicComponents/Arbiters/Arbiter_MITUCB_MemoryNode.cc\
	PhoenixSim/electronicComponents/Arbiters/Arbiter_Crossbar.cc\
	PhoenixSim/electronicComponents/Arbiters/Arbiter_EM.cc\
	PhoenixSim/electronicComponents/Arbiters/Arbiter_PC.cc\
	PhoenixSim/electronicComponents/Arbiters/Arbiter.cc\
	PhoenixSim/electronicComponents/Arbiters/Arbiter_ET.cc\
	PhoenixSim/electronicComponents/Arbiters/Arbiter_SR4x4.cc\
	PhoenixSim/electronicComponents/Arbiters/Arbiter_BT_4x4.cc\
	PhoenixSim/electronicComponents/Arbiters/Arbiter_EM_circuit.cc\
	PhoenixSim/electronicComponents/Arbiters/Arbiter_CM.cc\
	PhoenixSim/electronicComponents/Arbiters/Arbiter_GWC.cc\
	PhoenixSim/electronicComponents/Arbiters/Arbiter_SR8x8.cc\
	PhoenixSim/electronicComponents/Arbiters/Arbiter_XB_Gateway.cc\
	PhoenixSim/electronicComponents/Arbiters/Arbiter_NBT_Gateway.cc\
	PhoenixSim/electronicComponents/Arbiters/Arbiter_SR_Top.cc\
	PhoenixSim/electronicComponents/Arbiters/Arbiter_NBT_Node.cc\
	PhoenixSim/chipComponents/InoutConverter.cc\
	PhoenixSim/statistics/statistics.cc\
	PhoenixSim/statistics/statGroups/StatGroup_Sum.cc\
	PhoenixSim/statistics/statGroups/StatGroup_ListAll.cc\
	PhoenixSim/statistics/StatObject.cc\
	PhoenixSim/statistics/statObjects/energy/StatObject_EnergyEvent.cc\
	PhoenixSim/statistics/statObjects/energy/StatObject_EnergyOn.cc\
	PhoenixSim/statistics/statObjects/energy/StatObject_EnergyStatic.cc\
	PhoenixSim/statistics/statObjects/StatObject_TimeAvg.cc\
	PhoenixSim/statistics/statObjects/StatObject_MMA.cc\
	PhoenixSim/statistics/statObjects/StatObject_Count.cc\
	PhoenixSim/statistics/statObjects/StatObject_Total.cc\
	PhoenixSim/statistics/StatGroup.cc\
	PhoenixSim/statistics/LogFile.cc\
	PhoenixSim/photonic/activeringelement.cc\
	PhoenixSim/photonic/modulatorelement.cc\
	PhoenixSim/photonic/phylayer.cc\
	PhoenixSim/photonic/resonanceprofile.cc\
	PhoenixSim/photonic/tworingelement.cc\
	PhoenixSim/photonic/activetworingelement.cc\
	PhoenixSim/photonic/machzehnderelement.cc\
	PhoenixSim/photonic/ringelement.cc\
	PhoenixSim/photonic/activemultiringelement.cc\
	PhoenixSim/photonic/devices/StarCoupler.cc\
	PhoenixSim/photonic/devices/detector.cc\
	PhoenixSim/photonic/devices/join.cc\
	PhoenixSim/photonic/devices/bend.cc\
	PhoenixSim/photonic/devices/end.cc\
	PhoenixSim/photonic/devices/cross.cc\
	PhoenixSim/photonic/devices/line.cc\
	PhoenixSim/photonic/devices/laser.cc\
	PhoenixSim/photonic/devices/filter1x2.cc\
	PhoenixSim/photonic/devices/SwitchingFilter.cc\
	PhoenixSim/photonic/devices/pse2x2.cc\
	PhoenixSim/photonic/devices/modulator.cc\
	PhoenixSim/photonic/devices/pse1x2.cc\
	PhoenixSim/photonic/devices/activedirectionalcoupler.cc\
	PhoenixSim/photonic/devices/pse1x2nx.cc\
	PhoenixSim/photonic/devices/facet.cc\
	PhoenixSim/photonic/devices/attenuator.cc\
	PhoenixSim/photonic/devices/multiring/pse1x2r2.cc\
	PhoenixSim/photonic/devices/multiring/pse2x2r2.cc\
	PhoenixSim/photonic/devices/multiring/pse1x2nxr3.cc\
	PhoenixSim/photonic/devices/multiring/pse2x2r3.cc\
	PhoenixSim/photonic/devices/multiring/pse1x2nxr2.cc\
	PhoenixSim/photonic/devices/multiring/pse1x2multiring.cc\
	PhoenixSim/photonic/devices/multiring/pse2x2multiring.cc\
	PhoenixSim/photonic/devices/multiring/pse1x2r3.cc\
	PhoenixSim/photonic/devices/multiring/pse1x2nxmultiring.cc\
	PhoenixSim/photonic/devices/machzehnder/mzswitch2x2.cc\
	PhoenixSim/photonic/devices/machzehnder/mzswitch1x2.cc\
	PhoenixSim/photonic/activeelement.cc\
	PhoenixSim/photonic/detectorelement.cc\
	PhoenixSim/photonic/domainchanger.cc\
	PhoenixSim/photonic/basicelement.cc\
	PhoenixSim/photonic/multiringelement.cc \
	$(PROJECT_SOURCES)





EXTRA_DIST = makeSDL.pl

libPhoenixSim_la_LDFLAGS = $(OMNETPP_LDFLAGS) -module -avoid-version 

libPhoenixSim_la_LIBADD = -L"$(OMNETPP_LIB_SUBDIR)" -L"$(OMNETPP_LIB_DIR)" $(KERNEL_LIBS) $(SYS_LIBS)
