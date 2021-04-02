# Import the SST module
import sst

# Example1 builds on Example0, adding some statistic tracking for the event size
# and randomly varying the event size between 1 and 'eventSize'.
#
# Relevant code:
#   simpleElementExample/example1.h
#   simpleElementExample/example1.cc
#   simpleElementExample/basicEvent.h
# Output:
#   simpleElementExample/tests/refFiles/example1.out
#

### Create the components
component0 = sst.Component("c0", "simpleElementExample.example1")
component1 = sst.Component("c1", "simpleElementExample.example1")

### Parameterize the components.
# Run 'sst-info simpleElementExample.example0' at the command line 
# to see parameter documentation
params = {
        "eventsToSend" : 50,    # Required parameter, error if not provided
        "eventSize" : 32        # Optional parameter, defaults to 16 if not provided
}
component0.addParams(params)
component1.addParams(params)

### Link the components via their 'port' ports
link = sst.Link("component_link")
link.connect( (component0, "port", "1ns"), (component1, "port", "1ns") )

### Enable statistics
# Limit the verbosity of statistics to any with a load level from 0-7
sst.setStatisticLoadLevel(7)

# Determine where statistics should be sent
sst.setStatisticOutput("sst.statOutputConsole") 

# Enable statistics on both components
sst.enableAllStatisticsForComponentType("simpleElementExample.example1")

# Because the link latency is ~1ns and the components send one event
# per cycle on a 1GHz clock, the simulation time should be just over eventsToSend ns
# The statistics output will change if eventSize is changed.
