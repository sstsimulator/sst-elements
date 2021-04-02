# Import the SST module
import sst

# In Example0, two components send each other a number of events 
# The simulation ends when the components have sent and
# received all expected events. While the eventSize is parameterized,
# it has no effect on simulation time because the components don't limit
# their link bandwidth
#
# Relevant code:
#   simpleElementExample/example0.h
#   simpleElementExample/example0.cc
#   simpleElementExample/basicEvent.h
# Output:
#   simpleElementExample/tests/refFiles/example0.out
#

### Create the components
component0 = sst.Component("c0", "simpleElementExample.example0")
component1 = sst.Component("c1", "simpleElementExample.example0")

### Parameterize the components.
# Run 'sst-info simpleElementExample.example0' at the command line 
# to see parameter documentation
params = {
        "eventsToSend" : 50,    # Required parameter, error if not provided
        "eventSize" : 32        # Optional parameter, defaults to 16 if not provided
}
component0.addParams(params)
component1.addParams(params)

# Link the components via their 'port' ports
link = sst.Link("component_link")
link.connect( (component0, "port", "1ns"), (component1, "port", "1ns") )

# Because the link latency is ~1ns and the components send one event
# per cycle on a 1GHz clock, the simulation time should be just over eventsToSend ns
