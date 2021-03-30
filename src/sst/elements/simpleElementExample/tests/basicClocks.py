# Import the SST module
import sst

# The basicClocks.py script demonstrates the use of clocks in the simpleElementExample.basicClocks component
# The component takes three clock frequencies and one cycle count as parameters.
# Simulation ends when clock0 has executed the specified cycle count
# clock1 and clock2 run for 15 cycles and then stop
#
# The output is:
#   1. For each clock registered, print the frequency/period it was registered with ("Registering clockX at ...")
#       - Clock1 is registerd with a UnitAlgebra type and that print statement shows some strings that can be acquired from 
#         a UnitAlgebra. See basicClocks.cc.
#   2. 10 messages (as long as clockTicks >= 10), that list the current cycle count for each clock and for the simulation, 
#      as well as the current simulation time in nanoseconds
#       - Lines look like: "Clock0 cycles: 150, Clock1 cycles: 30, Clock2 cycles: 10, SimulationCycles: 150000, Simulation ns: 150"
#   3. 10 messages from each of clock1 and clock2 which are printed during their clock handlers
#       - Lines look like: "Clock #1 - TICK num: 1"
#   4. The final simulation time which should be equal to 'clockTicks' cycles for clock0
#
# This component has no links and SST will produce a warning because that is an unusual configuration
# that often points to a mis-configuration. For this simulation, the warning can be ignored.
#
# Relevant code:
#   simpleElementExample/basicClocks.h
#   simpleElementExample/basicClocks.cc
# Output:
#   simpleElementExample/tests/refFiles/basicClocks.out
#

### SST Core configuration
# By default, the SST Core uses picosecond resolution. The cycle count is a uint64_t, giving up to ~2/3 a year of simulation time.
# The resolution can be adjusted if (1) you need more fine-grained clocks or (2) you need a longer simulation period
# The core is event driven, so the resolution doesn't affect performance and only needs to be adjusted if one of the above is true

# Increase simulation period (you'll have to adjust the clocks below!)
# sst.setProgramOption("timebase", "1 ms")

# Decrease simulation period. Won't affect below the parameters below, but will affect the print statements giving SimulationCycles.
# sst.setProgramOption("timebase", "1 fs")

### Create the component
component = sst.Component("ClockComponent", "simpleElementExample.basicClocks")

### Parameterize the component.
# Run 'sst-info simpleElementExample.basicClocks' at the command line 
# to see parameter documentation
params = {
        "clock0" : "1GHz",  # Clocks can be specified as a frequency or a period.
        "clock1" : "5ns",
        "clock2" : "15ns",
        "clockTicks" : 250,
}
component.addParams(params)

### No links
### No statistics
