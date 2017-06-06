#This test file is intended to be run from the verimem python suite within ./Verimem/

# Automatically generated SST Python input
import sst
import sys

def read_arguments():
    boolUseRandomTrace = True
    boolUseDefaultConfig = True
    trace_file = ""
    config_file = ""

    for arg in sys.argv:
        if arg.find("--tracefile=") != -1:
            substrIndex = arg.find("=")+1
            trace_file = arg[substrIndex:]
            boolUseRandomTrace = False
            print "Trace file:", trace_file

        if arg.find("--configfile=") != -1:
            substrIndex = arg.find("=")+1
            config_file = arg[substrIndex:]
            boolUseDefaultConfig = False
            print "Config file:", config_file
    return [boolUseRandomTrace, trace_file, boolUseDefaultConfig, config_file]

def setup_config_params():
    l_params = {}
    if g_boolUseDefaultConfig:
        print "Config file not found... using default configuration"
        l_params = {
            "clockCycle": "1ns",
            "stopAtCycle": "10us",
            "numTxnGenReqQEntries":"""50""",
            "numTxnGenResQEntries":"""50""",
            "numTxnUnitReqQEntries":"""50""",
            "numTxnUnitResQEntries":"""50""",
            "numCmdReqQEntries":"""400""",
            "numCmdResQEntries":"""400""",
            "numChannelsPerDimm":"""2""",
            "numRanksPerChannel":"""2""",
            "numBankGroupsPerRank":"""2""",
            "numBanksPerBankGroup":"""2""",
            "relCommandWidth":"""1""",
            "readWriteRatio":"""1""",
            "boolUseReadA":"""0""",
            "boolUseWriteA":"""0""",
            "boolAllocateCmdResACT":"""0""",
            "boolAllocateCmdResREAD":"""0""",
            "boolAllocateCmdResREADA":"""0""",
            "boolAllocateCmdResWRITE":"""0""",
            "boolAllocateCmdResWRITEA":"""0""",
            "boolAllocateCmdResPRE":"""0""",
            "nRC":"""55""",
            "nRRD":"""4""",
            "nRRD_L":"""6""",
            "nRRD_S":"""4""",
            "nRCD":"""16""",
            "nCCD":"""4""",
            "nCCD_L":"""6""",
            "nCCD_L_WR":"""1""",
            "nCCD_S":"""4""",
            "nAL":"""15""",
            "nCL":"""16""",
            "nCWL":"""12""",
            "nWR":"""18""",
            "nWTR":"""3""",
            "nWTR_L":"""9""",
            "nWTR_S":"""3""",
            "nRTW":"""4""",
            "nEWTR":"""6""",
            "nERTW":"""6""",
            "nEWTW":"""6""",
            "nERTR":"""6""",
            "nRAS":"""39""",
            "nRTP":"""9""",
            "nRP":"""16""",
            "nRFC":"""420""",
            "nREFI":"""9360""",
            "nFAW":"""16""",
            "nBL":"""4"""
        }
    else:
        l_configFile = open(g_config_file, 'r')
        for l_line in l_configFile:
            l_tokens = l_line.split()
            #print l_tokens[0], ": ", l_tokens[1]
            l_params[l_tokens[0]] = l_tokens[1]

    if not g_boolUseRandomTrace:
        l_params["traceFile"] = g_trace_file
    else:
        print "Trace file not found... using random address generator"

    return l_params

def setup_txn_generator(params):
    l_txnGenStr = ""
    if g_boolUseRandomTrace:
        l_txnGen = "CramSim.c_TxnGenRand"
    else:
        l_txnGen = "CramSim.c_DramSimTraceReader"
    l_txnGen = sst.Component("TxnGen0", l_txnGen)
    l_txnGen.addParams(params)
    return l_txnGen

# Command line arguments
g_boolUseRandomTrace = True
g_boolUseDefaultConfig = True
g_trace_file = ""
g_config_file = ""

# Setup global parameters
[g_boolUseRandomTrace, g_trace_file, g_boolUseDefaultConfig, g_config_file] = read_arguments()
g_params = setup_config_params()


# Define SST core options
sst.setProgramOption("timebase", g_params["clockCycle"])
sst.setProgramOption("stopAtCycle", g_params["stopAtCycle"])
sst.setStatisticLoadLevel(7)
sst.setStatisticOutput("sst.statOutputConsole")
#sst.setStatisticOutputOption("help", "help")

# Define the simulation components
# txn gen
comp_txnGen0 = setup_txn_generator(g_params)


# controller
comp_controller0 = sst.Component("MemController0", "CramSim.c_Controller")
comp_controller0.addParams(g_params)
comp_controller0.addParams({
		"TxnConverter" : "CramSim.c_TxnConverter",
		"AddrHasher" : "CramSim.c_AddressHasher",
		"CmdScheduler" : "CramSim.c_CmdScheduler" ,
		"DeviceController" : "CramSim.c_DeviceController"
		})



# bank receiver
comp_dimm0 = sst.Component("Dimm0", "CramSim.c_Dimm")
comp_dimm0.addParams(g_params)


# enable all statistics
comp_txnGen0.enableAllStatistics()
comp_controller0.enableAllStatistics()
comp_controller0.enableStatistics(["reqQueueSize"],  # overriding the type of one statistic
                                  {"type":"sst.HistogramStatistic",
                                   "minvalue" : "0",
                                   "binwidth" : "2",
                                   "numbins" : "18",
                                   "IncludeOutOfBounds" : "1"})
comp_controller0.enableStatistics(["resQueueSize"],  # overriding the type of one statistic
                                  {"type":"sst.HistogramStatistic",
                                   "minvalue" : "0",
                                   "binwidth" : "2",
                                   "numbins" : "18",
                                   "IncludeOutOfBounds" : "1"})
#comp_txnUnit0.enableAllStatistics({ "type":"sst.AccumulatorStatistic",
#                                    "rate":"1 us"})
comp_dimm0.enableAllStatistics()

# Define simulation links

# TXNGEN / Controller LINKS
# TxnGen -> Controller (Req)(Txn)
txnReqLink_0 = sst.Link("txnReqLink_0")
txnReqLink_0.connect( (comp_txnGen0, "outTxnGenReqPtr", g_params["clockCycle"]), (comp_controller0, "inTxnGenReqPtr", g_params["clockCycle"]) )

# TxnGen <- Controller (Res)(Txn)
txnResLink_0 = sst.Link("txnResLink_0")
txnResLink_0.connect( (comp_txnGen0, "inCtrlResPtr", g_params["clockCycle"]), (comp_controller0, "outTxnGenResPtr", g_params["clockCycle"]) )


# TxnGen <- Controller (Req)(Token)
txnTokenLink_0 = sst.Link("txnTokenLink_0")
txnTokenLink_0.connect( (comp_txnGen0, "inCtrlReqQTokenChg", g_params["clockCycle"]), (comp_controller0, "outTxnGenReqQTokenChg", g_params["clockCycle"]) )

# TxnGen -> Controller (Res)(Token)
txnTokenLink_1 = sst.Link("txnTokenLink_1")
txnTokenLink_1.connect( (comp_txnGen0, "outTxnGenResQTokenChg", g_params["clockCycle"]), (comp_controller0, "inTxnGenResQTokenChg", g_params["clockCycle"]) )


# Controller -> Dimm (Req)
cmdReqLink_1 = sst.Link("cmdReqLink_1")
cmdReqLink_1.connect( (comp_controller0, "outDeviceReqPtr", g_params["clockCycle"]), (comp_dimm0, "inCtrlReqPtr", g_params["clockCycle"]) )

# Controller <- Dimm (Res) (Cmd)
cmdResLink_1 = sst.Link("cmdResLink_1")
cmdResLink_1.connect( (comp_controller0, "inDeviceResPtr", g_params["clockCycle"]), (comp_dimm0, "outCtrlResPtr", g_params["clockCycle"]) )




# End of generated output.
