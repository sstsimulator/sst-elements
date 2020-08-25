# Automatically generated SST Python input
import sst

# Define SST core options
sst.setProgramOption("timebase", "1 ps")
sst.setProgramOption("stopAtCycle", "10000s")

# Define the simulation components
df_0 = sst.Component("df_0", "llyr.LlyrDataflow")
df_0.addParams({
      "verbose": 10,
      "clockcount" : """100000000""",
      "clock" : """1MHz"""
})


# Define the simulation links
# End of generated output.

