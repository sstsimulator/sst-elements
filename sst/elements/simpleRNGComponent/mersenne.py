# Automatically generated SST Python input
import sst

# Define SST core options
sst.setProgramOption("timebase", "1 ps")
sst.setProgramOption("stopAtCycle", "10000s")

# Define the simulation components
clocker0 = sst.Component("clocker0", "simpleRNGComponent.simpleRNGComponent")
clocker0.addParams({
      "rng" : """mersenne""",
      "count" : """100000""",
      "seed" : """1447"""
})


# Define the simulation links
# End of generated output.
