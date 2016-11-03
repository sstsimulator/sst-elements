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
    "numTxnDrvBufferQEntries":"""50""",
    "relCommandWidth":"""1""",
    "numCmdDrvBufferQEntries":"""50""",
    "readWriteRatio":"""0.5"""
}

# Define the simulation components

# txn gen
comp_txnGen0 = sst.Component("TxnGen0", "CramSim.c_TxnGenSeq")
comp_txnGen0.addParams(g_txnParams)

# txn driver
comp_txnDriver0 = sst.Component("TxnDriver0", "CramSim.c_TxnDriver")
comp_txnDriver0.addParams(g_txnParams)


# Define simulation links


# TXNGEN / TXNDRIVER LINKS
# TxnGen -> TxnDriver (Req)(Txn)
txnReqLink_0 = sst.Link("txnReqLink_0")
txnReqLink_0.connect( (comp_txnGen0, "outTxnGenReqPtr", g_clockCycle), (comp_txnDriver0, "inTxnGenReqPtr", g_clockCycle) )

# TxnGen <- TxnDriver (Req)(Token)
txnTokenLink_0 = sst.Link("txnTokenLink_0")
txnTokenLink_0.connect( (comp_txnGen0, "inTxnUnitReqQTokenChg", g_clockCycle), (comp_txnDriver0, "outTxnDrvReqQTokenChg", g_clockCycle) )

# TxnGen <- TxnDriver (Res)(Txn)
txnResLink_0 = sst.Link("txnResLink_0")
txnResLink_0.connect( (comp_txnGen0, "inTxnUnitResPtr", g_clockCycle), (comp_txnDriver0, "outTxnDrvResPtr", g_clockCycle) )

# TxnGen -> TxnDriver (Res)(Token)
txnTokenLink_1 = sst.Link("txnTokenLink_1")
txnTokenLink_1.connect( (comp_txnGen0, "outTxnGenResQTokenChg", g_clockCycle), (comp_txnDriver0, "inTxnGenResQTokenChg", g_clockCycle) )





# End of generated output.
