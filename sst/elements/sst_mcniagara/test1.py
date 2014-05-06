# Automatically generated SST Python input
import sst

# Define SST core options
sst.setProgramOption("timebase", "1 ps")
sst.setProgramOption("stopAtCycle", "0 ns")

# Define the simulation components
McNiagara = sst.Component("McNiagara", "sst_mcniagara.SSTMcNiagara")
McNiagara.addParams({
      "Debug" : """1""",
      "seed" : """100""",
      "cycles" : """100000""",
      "converge" : """0""",
      "appDirectory" : """mcniagara/""",
      "inputHistogram" : """INPUT""",
      "instructionProbabilityFile" : """inst_prob.data""",
      "performanceCounterFile" : """perf_cnt.data"""
})


# Define the simulation links
# End of generated output.
