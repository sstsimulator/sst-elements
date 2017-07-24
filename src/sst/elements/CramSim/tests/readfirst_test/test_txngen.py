# Automatically generated SST Python input
import sst
import sys
import time
sys.path.append('./tests/')
from util import *


# Command line arguments
g_trace_file = ""
g_config_file = ""
g_trace_gen = ""
g_stopAtCycle = ""
g_params = ""

# Setup global parameters
[g_trace_file, g_config_file, g_trace_gen, g_stopAtCycle] = read_arguments()
g_params = setup_config_params(g_config_file)
g_params["traceFile"] = g_trace_file
if g_stopAtCycle != "" :
	g_params["stopAtCycle"]=g_stopAtCycle




# Define SST core options
sst.setProgramOption("timebase", g_params["clockCycle"])
sst.setProgramOption("stopAtCycle", g_params["stopAtCycle"])
sst.setStatisticLoadLevel(7)
sst.setStatisticOutput("sst.statOutputConsole")


# txn gen
comp_txnGen0 = sst.Component("TxnGen0", g_trace_gen)
comp_txnGen0.addParams(g_params)


# controller
comp_controller0 = sst.Component("MemController0", "CramSim.c_Controller")
comp_controller0.addParams(g_params)
comp_controller0.addParams({
		"TxnScheduler" : "CramSim.c_TxnScheduler",
		"TxnConverter" : "CramSim.c_TxnConverter",
		"AddrHasher" : "CramSim.c_AddressHasher",
		"CmdScheduler" : "CramSim.c_CmdScheduler" ,
		"DeviceDriver" : "CramSim.c_DeviceDriver"
		})



# bank receiver
comp_dimm0 = sst.Component("Dimm0", "CramSim.c_Dimm")
comp_dimm0.addParams(g_params)



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

'''
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
'''


