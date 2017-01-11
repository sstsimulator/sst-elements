# Automatically generated SST Python input
import sst


#global system params
g_clockCycle  = "1ps"

# Define SST core options
sst.setProgramOption("timebase", g_clockCycle)
sst.setProgramOption("stopAtCycle", "10us")


# Define technology parameters
g_txnParams = {
    "numTxnGenReqQEntries":"""50""",
    "numTxnGenResQEntries":"""50""",
    "numTxnUnitReqQEntries":"""50""",
    "numTxnUnitResQEntries":"""50""",
    "numCmdReqQEntries":"""400""",
    "numCmdResQEntries":"""400""",
    "numChannelsPerDimm":"""1""",
    "numRanksPerChannel":"""1""",
    "numBankGroupsPerRank":"""1""",
    "numBanksPerBankGroup":"""2""",
    "relCommandWidth":"""1""",
    "readWriteRatio":"""0.5""",
    "boolUseReadA":"""0""",
    "boolUseWriteA":"""0"""
}

# Define the simulation components

# txn gen
comp_txnGen0 = sst.Component("TxnGen0", "CramSim.c_TxnGenSeq")
comp_txnGen0.addParams(g_txnParams)

# txn unit
comp_txnUnit0 = sst.Component("TxnUnit0", "CramSim.c_TxnUnit")
comp_txnUnit0.addParams(g_txnParams)

# cmd unit
comp_cmdUnit0 = sst.Component("CmdUnit0", "CramSim.c_CmdUnit")
comp_cmdUnit0.addParams(g_txnParams)

# bank receiver
comp_dimm0 = sst.Component("Dimm0", "CramSim.c_Dimm")
comp_dimm0.addParams(g_txnParams)

# Define simulation links


# TXNGEN / TXNUNIT LINKS
# TxnGen -> TxnUnit (Req)(Txn)
txnReqLink_0 = sst.Link("txnReqLink_0")
txnReqLink_0.connect( (comp_txnGen0, "outTxnGenReqPtr", g_clockCycle), (comp_txnUnit0, "inTxnGenReqPtr", g_clockCycle) )

# TxnGen <- TxnUnit (Req)(Token)
txnTokenLink_0 = sst.Link("txnTokenLink_0")
txnTokenLink_0.connect( (comp_txnGen0, "inTxnUnitReqQTokenChg", g_clockCycle), (comp_txnUnit0, "outTxnGenReqQTokenChg", g_clockCycle) )

# TxnGen <- TxnUnit (Res)(Txn)
txnResLink_0 = sst.Link("txnResLink_0")
txnResLink_0.connect( (comp_txnGen0, "inTxnUnitResPtr", g_clockCycle), (comp_txnUnit0, "outTxnGenResPtr", g_clockCycle) )

# TxnGen -> TxnUnit (Res)(Token)
txnTokenLink_1 = sst.Link("txnTokenLink_1")
txnTokenLink_1.connect( (comp_txnGen0, "outTxnGenResQTokenChg", g_clockCycle), (comp_txnUnit0, "inTxnGenResQTokenChg", g_clockCycle) )




# TXNUNIT / CMDUNIT LINKS
# TxnUnit -> CmdUnit (Req) (Cmd)
cmdReqLink_0 = sst.Link("cmdReqLink_0")
cmdReqLink_0.connect( (comp_txnUnit0, "outCmdUnitReqPtrPkg", g_clockCycle), (comp_cmdUnit0, "inTxnUnitReqPtr", g_clockCycle) )

# TxnUnit <- CmdUnit (Req) (Token)
cmdTokenLink_0 = sst.Link("cmdTokenLink_0")
cmdTokenLink_0.connect( (comp_txnUnit0, "inCmdUnitReqQTokenChg", g_clockCycle), (comp_cmdUnit0, "outTxnUnitReqQTokenChg", g_clockCycle) )

# TxnUnit <- CmdUnit (Res) (Cmd)
cmdResLink_0 = sst.Link("cmdResLink_0")
cmdResLink_0.connect( (comp_txnUnit0, "inCmdUnitResPtr", g_clockCycle), (comp_cmdUnit0, "outTxnUnitResPtr", g_clockCycle) )




# CMDUNIT / DIMM LINKS
# CmdUnit -> Dimm (Req) (Cmd)
cmdReqLink_1 = sst.Link("cmdReqLink_1")
cmdReqLink_1.connect( (comp_cmdUnit0, "outBankReqPtr", g_clockCycle), (comp_dimm0, "inCmdUnitReqPtr", g_clockCycle) )

# CmdUnit <- Dimm (Res) (Cmd)
cmdResLink_1 = sst.Link("cmdResLink_1")
cmdResLink_1.connect( (comp_cmdUnit0, "inBankResPtr", g_clockCycle), (comp_dimm0, "outCmdUnitResPtr", g_clockCycle) )




# End of generated output.
