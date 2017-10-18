import sst
import sys
import time
from util import *

#######################################################################################################

# Command line arguments
g_config_file = ""
g_override_list = ""

# Setup global parameters
[g_config_file, g_overrided_list] = read_arguments()
g_params = setup_config_params(g_config_file, g_overrided_list)

numLanes = 4
g_params["strAddressMapStr"] = "_rrrrrrrrrrrrrrr_R_BB_bb_lllllllllll_CCC_xx_hhhhhh_"
laneIdxPos = "6:6"
readWriteRatio = 0.5
channelsPerLane = 8
g_params["numChannels"] = channelsPerLane
totalChannel = channelsPerLane*numLanes
maxOutstandingReqs = totalChannel*64
numTxnPerCycle = totalChannel
maxTxns = 100000 * totalChannel


# Define SST core options
sst.setProgramOption("timebase", g_params["clockCycle"])
sst.setProgramOption("stopAtCycle", g_params["stopAtCycle"])
sst.setStatisticLoadLevel(7)
sst.setStatisticOutput("sst.statOutputConsole")


#########################################################################################################

## Configure transaction generator and transaction dispatcher
comp_txnGen = sst.Component("TxnGen", "CramSim.c_TxnGen")
comp_txnGen.addParams(g_params)
comp_txnGen.addParams({
	"mode" : "rand",
	"maxTxns" : maxTxns,
	"numTxnPerCycle" : totalChannel,
	"maxOutstandingReqs" : maxOutstandingReqs,
	"readWriteRatio" : readWriteRatio
	})
comp_txnGen.enableAllStatistics()

comp_txnDispatcher = sst.Component("txnDispatcher", "CramSim.c_TxnDispatcher");
comp_txnDispatcher.addParams({
	"numLanes" : numLanes,
	"laneIdxPos" : laneIdxPos,
	"debug" : 0,
	"debug_level" : 0
	})

txnGenLink = sst.Link("txnGenLink")
txnGenLink.connect((comp_txnGen, "memLink", g_params["clockCycle"]),(comp_txnDispatcher,"txnGen",g_params["clockCycle"]))


# Configure controller and device
for chid in range(numLanes):

	# controller
	comp_controller = sst.Component("MemController"+str(chid), "CramSim.c_Controller")
	comp_controller.addParams(g_params)
	comp_controller.addParams({
			"TxnScheduler" : "CramSim.c_TxnScheduler",
			"TxnConverter" : "CramSim.c_TxnConverter",
			"AddrMapper" : "CramSim.c_AddressHasher",
			"CmdScheduler" : "CramSim.c_CmdScheduler" ,
			"DeviceDriver" : "CramSim.c_DeviceDriver"
			})

	# device
	comp_dimm = sst.Component("Dimm"+str(chid), "CramSim.c_Dimm")
	comp_dimm.addParams(g_params)

	# TXNGEN / Controller LINKS
	# TxnGen -> Controller (Req)(Txn)
	txnReqLink_0 = sst.Link("txnReqLink_0_"+(str(chid)))
	txnReqLink_0.connect((comp_txnDispatcher, "lane_"+(str(chid)), g_params["clockCycle"]), (comp_controller, "txngenLink", g_params["clockCycle"]) )


	# Controller -> Dimm (Req)
	cmdReqLink_1 = sst.Link("cmdReqLink_1_"+(str(chid)))
	cmdReqLink_1.connect( (comp_controller, "memLink", g_params["clockCycle"]), (comp_dimm, "ctrlLink", g_params["clockCycle"]) )

	

	# enable all statistics
	comp_controller.enableAllStatistics()
	#comp_txnUnit0.enableAllStatistics({ "type":"sst.AccumulatorStatistic",
	#                                    "rate":"1 us"})
	#comp_dimm.enableAllStatistics()
