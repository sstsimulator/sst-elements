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


addressMapStr = "_rrrrrrrrrrrrrrr_BB_bb_lllllll_C_xx_hhhhhh_"
numlanes=4
laneIdxPos = "7:6"
readWriteRatio = 0.5
channelPerLane = 2
totalChannel = channelPerLane*numlanes
maxOutstandingReqs = totalChannel*64
numTxnPerCycle = totalChannel
maxTxns = 1000000

# Setup global parameters
[g_trace_file, g_config_file, g_trace_gen, g_stopAtCycle] = read_arguments()
g_params = setup_config_params(g_config_file)
#g_params["traceFile"] = g_trace_file
g_trace_file = "/home/seokin/local/packages/ramulator/cramsim-random.trace.4M"

if g_stopAtCycle != "" :
	g_params["stopAtCycle"]=g_stopAtCycle

if channelPerLane != "" :
	g_params["numChannels"]=channelPerLane

if addressMapStr != "":
	g_params["strAddressMapStr"] = addressMapStr


# Define SST core options
sst.setProgramOption("timebase", g_params["clockCycle"])
sst.setProgramOption("stopAtCycle", g_params["stopAtCycle"])
sst.setStatisticLoadLevel(7)
sst.setStatisticOutput("sst.statOutputConsole")



#comp_txnGen = sst.Component("TxnGen", "CramSim.c_TxnGen")
comp_txnGen = sst.Component("TxnGen", "CramSim.c_TraceReader")
comp_txnGen.addParams({
	"maxTxns" : maxTxns,
	"numTxnPerCycle" : totalChannel,
	"maxOutstandingReqs" : maxOutstandingReqs,
	"readWriteRatio" : readWriteRatio,
	"traceFile" : g_trace_file
	})

comp_txnDispatcher = sst.Component("txnDispatcher", "CramSim.c_TxnDispatcher");
comp_txnDispatcher.addParams({
	"numLanes" : numlanes,
	"laneIdxPos" : laneIdxPos,
	"debug" : 0,
	"debug_level" : 0
	})

txnGenLink = sst.Link("txnGenLink")
txnGenLink.connect((comp_txnGen, "lowLink", g_params["clockCycle"]),(comp_txnDispatcher,"txnGen",g_params["clockCycle"]))

# enable all statistics
comp_txnGen.enableAllStatistics()

# controller+device gen
for chid in range(numlanes):

	# controller
	comp_controller = sst.Component("MemController"+str(chid), "CramSim.c_Controller")
	comp_controller.addParams(g_params)
	comp_controller.addParams({
			"TxnScheduler" : "CramSim.c_TxnScheduler",
			"TxnConverter" : "CramSim.c_TxnConverter",
			"AddrHasher" : "CramSim.c_AddressHasher",
			"CmdScheduler" : "CramSim.c_CmdScheduler" ,
			"DeviceDriver" : "CramSim.c_DeviceDriver"
			})

	# device
	comp_dimm = sst.Component("Dimm"+str(chid), "CramSim.c_Dimm")
	comp_dimm.addParams(g_params)

	# TXNGEN / Controller LINKS
	# TxnGen -> Controller (Req)(Txn)
	txnReqLink_0 = sst.Link("txnReqLink_0_"+(str(chid)))
	txnReqLink_0.connect((comp_txnDispatcher, "lane_"+(str(chid)), g_params["clockCycle"]), (comp_controller, "inTxnGenReqPtr", g_params["clockCycle"]) )


	# Controller -> Dimm (Req)
	cmdReqLink_1 = sst.Link("cmdReqLink_1_"+(str(chid)))
	cmdReqLink_1.connect( (comp_controller, "outDeviceReqPtr", g_params["clockCycle"]), (comp_dimm, "inCtrlReqPtr", g_params["clockCycle"]) )

	# Controller <- Dimm (Res) (Cmd)
	cmdResLink_1 = sst.Link("cmdResLink_1_"+(str(chid)))
	cmdResLink_1.connect( (comp_controller, "inDeviceResPtr", g_params["clockCycle"]), (comp_dimm, "outCtrlResPtr", g_params["clockCycle"]) )
	

	# enable all statistics
	comp_controller.enableAllStatistics()
	#comp_txnUnit0.enableAllStatistics({ "type":"sst.AccumulatorStatistic",
	#                                    "rate":"1 us"})
	#comp_dimm.enableAllStatistics()
