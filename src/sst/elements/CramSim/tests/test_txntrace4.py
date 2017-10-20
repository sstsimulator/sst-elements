#This test file is intended to be run from the verimem python suite within ./Verimem/

# Automatically generated SST Python input
import sst
import sys

def read_arguments():
    boolUseRandomTrace = True
    boolUseDefaultConfig = True
    trace_file = ""
    config_file_list = list()
    override_list = list()

    for arg in sys.argv:
        if arg.find("--tracefile=") != -1:
            substrIndex = arg.find("=")+1
            trace_file = arg[substrIndex:]
            boolUseRandomTrace = False
            print "Trace file:", trace_file

        elif arg.find("--configfile=") != -1:
            substrIndex = arg.find("=")+1
            config_file_list.append(arg[substrIndex:])
            boolUseDefaultConfig = False
            print "Config file list:", config_file_list

        elif arg != sys.argv[0]:
            if arg.find("=") == -1:
                print "Malformed config override found!: ", arg
                exit(-1)
            override_list.append(arg)
            print "Override: ", override_list[-1]
            
    return [boolUseRandomTrace, trace_file, boolUseDefaultConfig, config_file_list, override_list]

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
        for g_config_file in g_config_file_list:
            l_configFile = open(g_config_file, 'r')
            for l_line in l_configFile:
                l_tokens = l_line.split()
                #print l_tokens[0], ": ", l_tokens[1]
                l_params[l_tokens[0]] = l_tokens[1]

    for override in g_override_list:
        l_tokens = override.split("=")
        print "Override cfg", l_tokens[0], l_tokens[1]
        l_params[l_tokens[0]] = l_tokens[1]
        
    if not g_boolUseRandomTrace:
        l_params["traceFile"] = g_trace_file
    else:
        print "Trace file not found... using random address generator"

    return l_params

def setup_txn_generator(params):
    l_txnGenStr = ""
    if g_boolUseRandomTrace:
        l_txnGen = "CramSim.c_TxnGen"
    else:
        l_txnGen = "CramSim.c_TraceFileReader"
    l_txnGen = sst.Component("TxnGen0", l_txnGen)
    l_txnGen.addParams(params)
    return l_txnGen

# Command line arguments
g_boolUseRandomTrace = True
g_boolUseDefaultConfig = True
g_trace_file = ""
g_config_file_list = list()
g_override_list = list()

# Setup global parameters
[g_boolUseRandomTrace, g_trace_file, g_boolUseDefaultConfig, g_config_file_list, g_override_list] = read_arguments()
g_params = setup_config_params()


# Define SST core options
sst.setProgramOption("timebase", g_params["clockCycle"])
sst.setProgramOption("stopAtCycle", g_params["stopAtCycle"])


# Define the simulation components

# txn gen
comp_txnGen0 = setup_txn_generator(g_params)

# controller
comp_controller0 = sst.Component("MemController0", "CramSim.c_Controller")
comp_controller0.addParams(g_params)
comp_controller0.addParams({
		"TxnScheduler" : "CramSim.c_TxnScheduler",
		"TxnConverter" : "CramSim.c_TxnConverter",
		"AddrMapper" : "CramSim.c_AddressHasher",
		"CmdScheduler" : "CramSim.c_CmdScheduler" ,
		"DeviceDriver" : "CramSim.c_DeviceDriver"
		})



# bank receiver
comp_dimm0 = sst.Component("Dimm0", "CramSim.c_Dimm")
comp_dimm0.addParams(g_params)



# Define simulation links

# TXNGEN / Controller LINKS
# TxnGen <-> Controller
txnReqLink_0 = sst.Link("txnReqLink_0")
txnReqLink_0.connect( (comp_txnGen0, "memLink", g_params["clockCycle"]), (comp_controller0, "txngenLink", g_params["clockCycle"]) )

# Controller <-> Dimm
cmdReqLink_1 = sst.Link("cmdReqLink_1")
cmdReqLink_1.connect( (comp_controller0, "memLink", g_params["clockCycle"]), (comp_dimm0, "ctrlLink", g_params["clockCycle"]) )





# End of generated output.
