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
    "relCommandWidth":"""1""",
    "numCmdDrvBufferQEntries":"""50""",
    "readWriteRatio":"""0.5"""
}

# Define the simulation components

# txn gen
comp_txnGen0 = sst.Component("TxnGen0", "CramSim.c_TxnGenSeq")
comp_txnGen0.addParams(g_txnParams)

# txn unit
comp_txnUnit0 = sst.Component("TxnUnit0", "CramSim.c_TxnUnit")
comp_txnUnit0.addParams(g_txnParams)

# cmd driver
comp_cmdDriver0 = sst.Component("CmdDriver0", "CramSim.c_CmdDriver")
comp_cmdDriver0.addParams(g_txnParams)

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




# TXNUNIT / CMDDRIVER LINKS
# TxnUnit -> CmdDriver (Req) (Cmd)
cmdReqLink_0 = sst.Link("cmdReqLink_0")
cmdReqLink_0.connect( (comp_txnUnit0, "outCmdUnitReqPtrPkg", g_clockCycle), (comp_cmdDriver0, "inTxnUnitReqPtr", g_clockCycle) )

# TxnUnit <- CmdDriver (Req) (Token)
cmdTokenLink_0 = sst.Link("cmdTokenLink_0")
cmdTokenLink_0.connect( (comp_txnUnit0, "inCmdUnitReqQTokenChg", g_clockCycle), (comp_cmdDriver0, "outCmdDrvReqQTokenChg", g_clockCycle) )

# TxnUnit <- CmdDriver (Res) (Cmd)
txnResLink_1 = sst.Link("txnResLink_1")
txnResLink_1.connect( (comp_txnUnit0, "inCmdUnitResPtr", g_clockCycle), (comp_cmdDriver0, "outCmdDrvResPtr", g_clockCycle) )


# End of generated output.
