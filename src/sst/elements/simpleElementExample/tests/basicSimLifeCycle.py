# Import the SST module
import sst

# The basicSimLifeCycle.py script runs a simulation that demonstrates each phase of the SST lifecycle.
# The phases are:
#   1. Construction - components are constructed
#   2. Init - components can exchange information prior to simulation begin
#   3. Setup - last chance for components to prepare for simulation
#   4. Run - the simulation itself. Time advances and simulation continues until the first of 
#           (1) a predefined time passed in this configuration file, 
#            or (2) until all primary components declare it is OK for simulation to end
#   5. Complete - components can exchange information after simulation has ended
#   6. Finish - last chance for components to do work prior to SST terminating
#   7. Destruction - components are cleaned up
#
#
#   Simulation in detail, including output. 
#   ==========================================
#   In this simulation there are no clocks, everything is event-driven.
#   Components are connected in a ring and send events counter-clockwise.
#   Prior to simulation begin, components agree on the number of events to send each other and send that number of events.
#   When all events are sent, simulation ends.
#   ==========================================
#   1. Construction
#       Each component is constructed.
#       Output (per-component):
#           "Phase: Construction, comp_name"
#
#   2. Init
#       Components exchange their eventsToSend parameters and discover the names of all other components in the ring
#       Output (per-component/per-round):
#           "Phase: Init(<round_number>), <comp_name>" for each component and each round in the init() phase
#
#   3. Setup 
#       Components compute the actual number of eventsToSend using the average of all components' parameters. 
#       Each component sends this number of events to each other component. Components enqueue their first event 
#       for simulation in the queue to prepare for the next phase.
#       Output:
#           "Phase: Setup, <comp_name>"
#           "    <comp_name> will send <num> events to each other component."
#   
#   4. Run
#       Components send the agreed-upon number of events to each other. Events contain the destination name. 
#       Components forward events counter-clockwise until they reach their destination.
#       Simulation ends when there are no more messages in flight
#       Output:
#           "Phase: Run, <comp_name>".
#               Note this is actually printed at the very end of setup() since that is the 
#               function that indicates the beginning of SST's run loop.
#
#   5. Complete
#       Components send their left neighbor a message saying Goodbye and their right neighbor a message saying Farewell
#       Output: "Phase: Complete(<round_number>), <comp_name>" for each component and each round in the complete() phase
#
#   6. Finish
#       Components print the message their neighbors sent during Complete, as well as information about how many events they 
#           handled during the Run phase
#       Output: 
#           "Phase: Finish, <comp_name>"
#           "    My name is <comp_name> and I sent <num> messages. I received <num> messages and forwarded <num> messages."
#           "    My left neighbor said: <message>."
#           "    My right neighbor said: <message>."
#           
#   7. Destruction
#       State is cleaned up.
#       Output: "Phase: Destruction"
#
#   If the 'verbose' parameter is set to true, components will also print each message they receive with this format:
#   "    <sim_cycle> <comp_name> received <message.toString()>" 
#   Expect message output in the init, run, and complete phases.
#
# Relevant code:
#   simpleElementExample/basicSimLifeCycle.h
#   simpleElementExample/basicSimLifeCycle.cc
#   simpleElementExample/basicSimLifeCycleEvent.h
# Output:
#   simpleElementExample/tests/refFiles/basicSimLifeCycle.out
#   simpleElementExample/tests/refFiles/basicSimLifeCycle_verbose.out
#
#
# Additional exercises:
#   Run this example using 'sst basicSimLifeCycle.py'. You will likely need to increase the 'eventsToSend' parameters to
#       make the simulation run longer so that it doesn't end before you can send it a signal. Try 100K or more.
#
#   1. After launching this example, send SST a SIGINT or SIGTERM - this triggers the "emergencyShutdown" function 
#   2. After launching this example, send SST a SIGUSR1 or SIGUSR2, or both.
#       SIGUSR1 will cause SST Core to output some information
#       SIGUSR2 will also call the "printStatus" function on each component
#
#   See basicSimLifeCycle.h and basicSimLifeCycle.cc for the emergencyShutdown and printStatus functions
#

num_components = 8
links = []

### Create our links first so we can connect them as we create components
for x in range(num_components):
    link = sst.Link("link_" + str(x))
    links.append(link)


### Create the components and link
for x in range(num_components):
    component = sst.Component("component_" + str(x), "simpleElementExample.basicSimLifeCycle")
    
    component.addParam("eventsToSend", x + 5) # Change this to make simulation longer by sending more events
    # component.addParam("verbose", 1) # Uncomment for verbose output - compare to refFiles/basicSimLifeCycle_verbose.out
    
    component.addLink(links[x-1], "left", "10ns")
    component.addLink(links[x], "right", "10ns")
