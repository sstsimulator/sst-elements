# Automatically generated SST Python input
import sst

# Define SST core options
sst.setProgramOption("timebase", "1 ps")
sst.setProgramOption("stopAtCycle", "10000s")

# Define the simulation components
msgGen0 = sst.Component("msgGen0", "messageGeneratorComponent.messageGeneratorComponent")
msgGen0.addParams({
      "clock" : """1MHz""",
      "sendcount" : """100000""",
      "outputinfo" : """0"""
})
msgGen1 = sst.Component("msgGen1", "messageGeneratorComponent.messageGeneratorComponent")
msgGen1.addParams({
      "clock" : """1MHz""",
      "sendcount" : """100000""",
      "outputinfo" : """0"""
})


# Define the simulation links
s_0_1 = sst.Link("s_0_1")
s_0_1.connect( (msgGen0, "remoteComponent", "1000000ps"), (msgGen1, "remoteComponent", "1000000ps") )
# End of generated output.
