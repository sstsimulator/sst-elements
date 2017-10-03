from __future__ import division
import subprocess
import sys

# GLOBAL PARAMS
config_file = "ddr4_verimem.cfg"
config_file_openbank = "ddr4_verimem_openbank.cfg"
DEBUG = True if sys.argv[1] == "1" else False

def run_verimem(config_file, trace_file):
    # set the command
    sstCmd = "sst --lib-path=.libs/ tests/test_txntrace.py --model-options=\""
    sstParams = "--configfile=" + config_file + " traceFile=" + trace_file + "\""

    osCmd = sstCmd + sstParams

    sstParams_openbank = "--configfile=" + config_file_openbank + " traceFile=" + trace_file + "\""
    osCmd_openbank = sstCmd + sstParams_openbank
    
    print osCmd

    # run SST
    p = subprocess.Popen(osCmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=True)
    output, err = p.communicate()

    if DEBUG:
        print "My output: ", output
        if err != "":
            print "My error: ", err

    # extract total Txns processed
    outputLines = output.split("\n")
    totalTxns = 0
    for line in outputLines:
        if line.find("Total Txns Received:") != -1:
            substrIndex = line.find("Total Txns Received: ")+20
            receivedStr = line[substrIndex:]
            totalTxns += int(receivedStr)


    return totalTxns


def run_verimem_openbank(config_file, trace_file):
    # set the command
    sstCmd = "sst --lib-path=.libs/ tests/test_txntrace.py --model-options=\""
    sstParams_openbank = "--configfile=" + config_file_openbank + " traceFile=" + trace_file + "\""
    osCmd_openbank = sstCmd + sstParams_openbank
    
    print osCmd_openbank

    # run SST
    p = subprocess.Popen(osCmd_openbank, stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=True)
    output, err = p.communicate()

    if DEBUG:
        print "My output: ", output
        if err != "":
            print "My error: ", err

    # extract total Txns processed
    outputLines = output.split("\n")
    totalTxns = 0
    for line in outputLines:
        if line.find("Total Txns Received:") != -1:
            substrIndex = line.find("Total Txns Received: ")+20
            receivedStr = line[substrIndex:]
            totalTxns += int(receivedStr)


    return totalTxns

#Trace Suite 1
def run_suite1(params):
    results = ""

    #*** READS ONLY ***
    totalTxns = run_verimem(params["config_file"], "traces/sst-CramSim-trace_verimem_1_R.trc")

    expected_Timing = max(params["nRC"],
        (params["nRCD"] + params["nRTP"] + params["nRP"]),
        (params["nFAW"] / 4),
        params["nRRD_L"],
        params["nCCD_L"])
    timing = params["stopAtCycle"] / totalTxns

    if DEBUG:
        print "Expected Timing: ", expected_Timing
        print "Actual Timing: ", timing, "\n\n\n"


    if ((abs(expected_Timing - timing) / timing) <= 0.10):
        results += "[v] Suite 1 - Reads only\n"
    else:
        results += "[x] Suite 1 - Reads only\n"

    #*** WRITES ONLY ***
    totalTxns = run_verimem(params["config_file"], "traces/sst-CramSim-trace_verimem_1_W.trc")

    expected_Timing = max((params["nRCD"] + params["nCWL"] + (params["nBL"]) + params["nWR"] + params["nRP"]),
        params["nRC"],
        (params["nFAW"] / 4),
        params["nRRD_L"],
        params["nCCD_L"])
    timing = params["stopAtCycle"] / totalTxns

    if DEBUG:
        print "Expected Timing: ", expected_Timing
        print "Actual Timing: ", timing, "\n\n\n"


    if ((abs(expected_Timing - timing) / timing) <= 0.10):
        results += "[v] Suite 1 - Writes only\n"
    else:
        results += "[x] Suite 1 - Writes only\n"

    #*** READS & WRITES ***
    totalTxns = run_verimem(params["config_file"], "traces/sst-CramSim-trace_verimem_1_RW.trc")

    first_timing_option = (params["nRAS"] + params["nRP"] + params["nRCD"] + params["nCWL"] + (params["nBL"] / 2) + params["nWR"] + params["nRP"]) / 2
    expected_Timing = max(first_timing_option,
        params["nRC"], (params["nFAW"] / 4),
        params["nRRD_L"],
        params["nCCD_L"])
    timing = params["stopAtCycle"] / totalTxns

    if DEBUG:
        print "Expected Timing: ", expected_Timing
        print "Actual Timing: ", timing, "\n\n\n"


    if ((abs(expected_Timing - timing) / timing) <= 0.10):
        results += "[v] Suite 1 - Reads & Writes\n"
    else:
        results += "[x] Suite 1 - Reads & Writes\n"

    return results


#Trace Suites 2 and 3 do not offer different insights for this sim from Suite 1
# sst --lib-path=.libs/ tests/test_txntrace.py --model-options="--configfile=ddr4_verimem_openbank.cfg traceFile=traces/sst-CramSim-trace_verimem_2_W.trc"

#Trace Suite 2
def run_suite2(params):
    results = ""

    #*** READS ONLY ***
    totalTxns = run_verimem_openbank(params["config_file"], "traces/sst-CramSim-trace_verimem_2_R.trc")

    expected_Timing = max(params["nRC"],
        (params["nRCD"] + params["nRTP"] + params["nRP"]),
        (params["nFAW"] / 4),
        params["nRRD_L"],
        params["nCCD_L"])
    timing = params["stopAtCycle"] / totalTxns

    if DEBUG:
        print "Expected Timing: ", expected_Timing
        print "Actual Timing: ", timing, "\n\n\n"


    if ((abs(expected_Timing - timing) / timing) <= 0.10):
        results += "[v] Suite 2 - Reads only\n"
    else:
        results += "[x] Suite 2 - Reads only\n"

    #*** WRITES ONLY ***
    totalTxns = run_verimem(params["config_file"], "traces/sst-CramSim-trace_verimem_2_W.trc")

    expected_Timing = max((params["nRCD"] + params["nCWL"] + (params["nBL"]) + params["nWR"] + params["nRP"]),
        params["nRC"],
        (params["nFAW"] / 4),
        params["nRRD_L"],
        params["nCCD_L"])
    timing = params["stopAtCycle"] / totalTxns

    if DEBUG:
        print "Expected Timing: ", expected_Timing
        print "Actual Timing: ", timing, "\n\n\n"


    if ((abs(expected_Timing - timing) / timing) <= 0.10):
        results += "[v] Suite 2 - Writes only\n"
    else:
        results += "[x] Suite 2 - Writes only\n"


    return results


#Trace Suite 4
def run_suite4(params):
    results = ""

    #*** READS ONLY ***
    totalTxns = run_verimem(params["config_file"], "traces/sst-CramSim-trace_verimem_4_R.trc")

    expected_Timing = max((params["nRC"] / 4),
        ((params["nRCD"] + params["nRTP"] + params["nRP"]) / 4),
        (params["nFAW"] /4),
        params["nRRD_L"],
        params["nCCD_L"])
    timing = params["stopAtCycle"] / totalTxns

    if DEBUG:
        print "Expected Timing: ", expected_Timing
        print "Actual Timing: ", timing, "\n\n\n"


    if ((abs(expected_Timing - timing) / timing) <= 0.10):
        results += "[v] Suite 4 - Reads only\n"
    else:
        results += "[x] Suite 4 - Reads only\n"

    #*** WRITES ONLY ***
    totalTxns = run_verimem(params["config_file"], "traces/sst-CramSim-trace_verimem_4_W.trc")

    expected_Timing = max(( (params["nRCD"] + params["nCWL"] + (params["nBL"] / 2) + params["nWR"] + params["nRP"]) / 4),
     (params["nRC"] / 4),
     (params["nFAW"] / 4),
     params["nRRD_L"],
     params["nCCD_L"])
    timing = params["stopAtCycle"] / totalTxns

    if DEBUG:
        print "Expected Timing: ", expected_Timing
        print "Actual Timing: ", timing, "\n\n\n"


    if ((abs(expected_Timing - timing) / timing) <= 0.10):
        results += "[v] Suite 4 - Writes only\n"
    else:
        results += "[x] Suite 4 - Writes only\n"

    results += "[v] Suite 4 - Reads & Writes (Analysis yet to be done)\n"

    return results

#Trace Suite 5
def run_suite5(params):
    results = ""

    #*** READS ONLY ***
    totalTxns = run_verimem(params["config_file"], "traces/sst-CramSim-trace_verimem_5_R.trc")

    expected_Timing = max((params["nRC"] / params["num_banks"]), ((params["nRCD"] + params["nRTP"] + params["nRP"]) / params["num_banks"]), (params["nFAW"] /4), params["nRRD_S"], params["nCCD_S"])
    timing = params["stopAtCycle"] / totalTxns

    if DEBUG:
        print "Expected Timing: ", expected_Timing
        print "Actual Timing: ", timing, "\n\n\n"


    if ((abs(expected_Timing - timing)) <= 1):
        results += "[v] Suite 5 - Reads only\n"
    else:
        results += "[x] Suite 5 - Reads only\n"

    #*** WRITES ONLY ***
    totalTxns = run_verimem(params["config_file"], "traces/sst-CramSim-trace_verimem_5_W.trc")

    expected_Timing = max( ( (params["nRCD"] + params["nCWL"] + (params["nBL"] / 2) + params["nWR"] + params["nRP"]) / params["num_banks"]),
                            (params["nRC"] / params["num_banks"]),
                            (params["nFAW"] / 4),
                             params["nRRD_S"],
                             params["nCCD_S"])
    timing = params["stopAtCycle"] / totalTxns

    if DEBUG:
        print "Expected Timing: ", expected_Timing
        print "Actual Timing: ", timing, "\n\n\n"


    if ((abs(expected_Timing - timing)) <= 1):
        results += "[v] Suite 5 - Writes only\n"
    else:
        results += "[x] Suite 5 - Writes only\n"

    results += "[v] Suite 5 - Reads & Writes (Analysis yet to be done)\n"

    return results

#Trace Suite 6
def run_suite6(params):
    results = ""

    #*** READS ONLY ***
    totalTxns = run_verimem(params["config_file"], "traces/sst-CramSim-trace_verimem_6_R.trc")

    expected_Timing = max((params["nFAW"] /4),
        params["nRRD_S"],
        params["nCCD_S"],
        (params["nRC"] / 16),
        ((params["nRCD"] + params["nRTP"] + params["nRP"]) / 16) )
    timing = params["stopAtCycle"] / totalTxns

    if DEBUG:
        print "Expected Timing: ", expected_Timing
        print "Actual Timing: ", timing, "\n\n\n"


    if (((expected_Timing - timing)) <= 1):
        results += "[v] Suite 6 - Reads only\n"
    else:
        results += "[x] Suite 6 - Reads only\n"

    #*** WRITES ONLY ***
    totalTxns = run_verimem(params["config_file"], "traces/sst-CramSim-trace_verimem_6_W.trc")

    expected_Timing = max((params["nFAW"] /4),
        params["nRRD_S"],
        params["nCCD_S"],
        ((params["nRCD"] + params["nCWL"] + (params["nBL"] / 2) + params["nWR"] + params["nRP"]) / 16) ,
        (params["nRC"] / 16) )
    timing = params["stopAtCycle"] / totalTxns

    if DEBUG:
        print "Expected Timing: ", expected_Timing
        print "Actual Timing: ", timing, "\n\n\n"


    if ((abs(expected_Timing - timing)) <= 1):
        results += "[v] Suite 6 - Writes only\n"
    else:
        results += "[x] Suite 6 - Writes only\n"

    results += "[v] Suite 6 - Reads & Writes (Analysis yet to be done)\n"

    return results


def santize_params(params):
    return_params = {}

    return_params["stopAtCycle"] = int(params["stopAtCycle"].replace("ns\n", ""))

    channels = int(params["numChannels"].replace("\n", ""))
    ranksPerChannel = int(params["numRanksPerChannel"].replace("\n", ""))
    bankGroupsPerRank = int(params["numBankGroupsPerRank"].replace("\n", ""))
    banksPerBankGroup = int(params["numBanksPerBankGroup"].replace("\n", ""))
    return_params["num_banks"] = channels * ranksPerChannel * bankGroupsPerRank * banksPerBankGroup

    return_params["nRC"] = int(params["nRC"].replace("\n", ""))
    return_params["nRRD"] = int(params["nRRD"].replace("\n", ""))
    return_params["nRRD_L"] = int(params["nRRD_L"].replace("\n", ""))
    return_params["nRRD_S"] = int(params["nRRD_S"].replace("\n", ""))
    return_params["nRCD"] = int(params["nRCD"].replace("\n", ""))
    return_params["nCCD"] = int(params["nCCD"].replace("\n", ""))
    return_params["nCCD_L"] = int(params["nCCD_L"].replace("\n", ""))
    return_params["nCCD_L_WR"] = int(params["nCCD_L_WR"].replace("\n", ""))
    return_params["nCCD_S"] = int(params["nCCD_S"].replace("\n", ""))
    return_params["nAL"] = int(params["nAL"].replace("\n", ""))
    return_params["nCL"] = int(params["nCL"].replace("\n", ""))
    return_params["nCWL"] = int(params["nCWL"].replace("\n", ""))
    return_params["nWR"] = int(params["nWR"].replace("\n", ""))
    return_params["nWTR"] = int(params["nWTR"].replace("\n", ""))
    return_params["nWTR_L"] = int(params["nWTR_L"].replace("\n", ""))
    return_params["nWTR_S"] = int(params["nWTR_S"].replace("\n", ""))
    return_params["nRTW"] = int(params["nRTW"].replace("\n", ""))
    return_params["nEWTR"] = int(params["nEWTR"].replace("\n", ""))
    return_params["nERTW"] = int(params["nERTW"].replace("\n", ""))
    return_params["nEWTW"] = int(params["nEWTW"].replace("\n", ""))
    return_params["nERTR"] = int(params["nERTR"].replace("\n", ""))
    return_params["nRAS"] = int(params["nRAS"].replace("\n", ""))
    return_params["nRTP"] = int(params["nRTP"].replace("\n", ""))
    return_params["nRP"] = int(params["nRP"].replace("\n", ""))
    return_params["nRFC"] = int(params["nRFC"].replace("\n", ""))
    return_params["nREFI"] = int(params["nREFI"].replace("\n", ""))
    return_params["nFAW"] = int(params["nFAW"].replace("\n", ""))
    return_params["nBL"] = int(params["nBL"].replace("\n", ""))

    return return_params

# get config params
g_params = {}
configFileInstance = open(config_file, 'r')
for line in configFileInstance:
    tokens = line.split(' ')
    g_params[tokens[0]] = tokens[1]

g_params = santize_params(g_params)
g_params["config_file"] = config_file

print "-----RUNNING VERIMEM-----"
print(run_suite1(g_params))
print(run_suite2(g_params))
print(run_suite4(g_params))
print(run_suite5(g_params))
print(run_suite6(g_params))
print("done.\n")
