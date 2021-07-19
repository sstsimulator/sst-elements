# Import the SST module
import sst

# The BasicLinks component demonstrates ways of handling events on a link
# It is similar to example0 and example1 except that the component has many links and
# it randomly selects a link to send each event on. The RNG is seeded by a parameter
# so that each component has unique behavior.
# It also provides a port_vector%d port that can have an arbitrary number of connections.
#
# To make things more interesting, we have four components in this example and
# we use different parameters for them.
#
# Relevant code:
#   simpleElementExample/basicLinks.h
#   simpleElementExample/basicLinks.cc
#   simpleElementExample/basicEvent.h
# Output:
#   simpleElementExample/tests/refFiles/basicLinks.out
#

### Create the components
component0 = sst.Component("c0", "simpleElementExample.basicLinks")
component1 = sst.Component("c1", "simpleElementExample.basicLinks")
component2 = sst.Component("c2", "simpleElementExample.basicLinks")
component3 = sst.Component("c3", "simpleElementExample.basicLinks")

### Parameterize the components.
# Run 'sst-info simpleElementExample.basicLinks' at the command line 
# to see parameter documentation
component0.addParams({
    "eventsToSend" : 50,
    "eventSize" : 16,
    "rngSeedZ" : 12,
    "rngSeedW" : 438949
    })

component1.addParams({
    "eventsToSend" : 45,
    "eventSize" : 16,
    "rngSeedZ" : 1589,
    "rngSeedW" : 4385949
    })

component2.addParams({
    "eventsToSend" : 50,
    "eventSize" : 11,
    "rngSeedZ" : 459,
    "rngSeedW" : 4389
    })

component3.addParams({
    "eventsToSend" : 25,
    "eventSize" : 24,
    "rngSeedZ" : 6,
    "rngSeedW" : 54869
    })

### Link the components via their 'port' ports
# Each 'basicLinks' components has three ports:
#   port_vector%d
#   port_handler
#   port_polled

# For port_polled, connect: c0 <-> c1 and c2 <-> c3
poll_link0 = sst.Link("link_poll0")
poll_link1 = sst.Link("link_poll1")
poll_link0.connect( (component0, "port_polled", "1us"), (component1, "port_polled", "1us") )
poll_link1.connect( (component2, "port_polled", "1us"), (component3, "port_polled", "1us") )

# For port_handler, connect c0 <-> c2 and c1 <-> c3
handle_link0 = sst.Link("link_handler0")
handle_link1 = sst.Link("link_handler1")
handle_link0.connect( (component0, "port_handler", "10ns"), (component2, "port_handler", "10ns") )
handle_link1.connect( (component1, "port_handler", "10ns"), (component3, "port_handler", "10ns") )

# For port_vector%d, connect each component to each other component
# There is no limit on the number of port_vectorX links per component
# The Component is written to look for contiguous port numbers so we must 
# ensure the numbering here is contiguous or the Component won't discover
# the links correctly

# Connect (component0, component1) and (component2, component3) via port_vector0
port_vector0 = sst.Link("link_vector0")
port_vector1 = sst.Link("link_vector1")
port_vector0.connect( (component0, "port_vector0", "1ns"), (component1, "port_vector0", "1ns") )
port_vector1.connect( (component2, "port_vector0", "1ns"), (component3, "port_vector0", "1ns") )

# Connect components (0, 2) and (1, 3) via port_vector1
port_vector2 = sst.Link("link_vector2")
port_vector3 = sst.Link("link_vector3")
port_vector2.connect( (component0, "port_vector1", "1ns"), (component2, "port_vector1", "1ns") )
port_vector3.connect( (component1, "port_vector1", "1ns"), (component3, "port_vector1", "1ns") )

# Connect components (0, 3) and (1, 2) via port_vector2
port_vector4 = sst.Link("link_vector4")
port_vector5 = sst.Link("link_vector5")
port_vector4.connect( (component0, "port_vector2", "1ns"), (component2, "port_vector2", "1ns") )
port_vector5.connect( (component1, "port_vector2", "1ns"), (component3, "port_vector2", "1ns") )


### Enable statistics
# Limit the verbosity of statistics to any with a load level from 0-7
sst.setStatisticLoadLevel(7)

# Determine where statistics should be sent
sst.setStatisticOutput("sst.statOutputConsole") 

# Enable statistics on both components
sst.enableAllStatisticsForComponentType("simpleElementExample.basicLinks")

