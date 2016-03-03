# Automatically generated SST Python input
import sst

# Define SST core options
sst.setProgramOption("timebase", "1 ps")
sst.setProgramOption("stopAtCycle", "0 ns")

# Define the simulation components
McOpteron = sst.Component("McOpteron", "sst_mcopteron.SSTMcOpteron")
McOpteron.addParams({
      "debugLevel" : """1""",
      "cycles" : """100000""",
      "converge" : """0""",
      "defFile" : """mcopteron/agner-insn.txt""",
      "debugCycles" : """0""",
      "printStaticMix" : """0""",
      "printInstructionMix" : """0""",
      "mixFile" : """mcopteron/usedist_new.all""",
      "appDirectory" : """.""",
      "seed" : """100""",
      "traceFile" : """mcopteron/trace.txt""",
      "traceOut" : """0""",
      "seperateSize" : """0""",
      "newMixFile" : """mcopteron/instrMix.txt""",
      "instructionSizeFile" : """mcopteron/instrSizeDistr.txt""",
      "repeatTrace" : """0"""
})


# Define the simulation links
# End of generated output.
