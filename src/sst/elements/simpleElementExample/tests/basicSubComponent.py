# Import the SST module
import sst

# The basicSubComponent.py script runs a simulation that demonstrates the basic use of subcomponents
#
#   The simulation consists of N components connected in a ring. Each component has a left link, a right link,
#   and a "compute_unit" subcomponent. The subcomponent controls what kind of computation the component does.
#   At the beginning of the simulation, each component sends an event with a number (the 'value' parameter) to its left.
#   When a component receives a message from a different component, it passes the number in the component to its compute_unit
#   subcomponent which updates the number. The event also has a string that describes the computation that has been done. 
#   After updating both the number and the string, the component passes the event to its left.
#   When a component receives its event back, it prints the computation that was done and the result and then tells SST that
#   the simulation can end. When all components have done this, the simulation ends.
#
#   Expected output:
#       Each component should print a line saying "<component_name> computed: <computation_string> = <result>"
#       SST will then print the end time of the simulation and exit
#       The end time is equal to the link latency multiplied by the number of components which is 100ns 
#       (i.e., the time it takes an event to make it around the ring)
#
# Relevant code:
#   simpleElementExample/basicSubComponent_component.h
#   simpleElementExample/basicSubComponent_component.cc
#   simpleElementExample/basicSubComponent_subcomponent.h
#   simpleElementExample/basicSubComponent_subcomponent.cc
#   simpleElementExample/basicSubComponentEvent.h
#
# Output:
#   simpleElementExample/tests/refFiles/basicSubComponent.out
#

# Some things we'll use when creating the simulation below
num_components = 10 # Change to change the number of components that get simulated
units = ("simpleElementExample.basicSubComponentIncrement",
         "simpleElementExample.basicSubComponentMultiply",
         "simpleElementExample.basicSubComponentDecrement",
         "simpleElementExample.basicSubComponentDivide")

### Create our links first so we can connect them as we create components
links = [] # List of link handlers
for x in range(num_components):
    link = sst.Link("link_" + str(x))
    links.append(link)


### Create the components and link
for x in range(num_components):
    component = sst.Component("component_" + str(x), "simpleElementExample.basicSubComponent_comp")
   
    # Have all components start with some number between 0 and 5
    component.addParam("value", (x + 1) % 6)
    
    # Connect the components to each other in a ring
    component.addLink(links[x-1], "left", "10ns")
    component.addLink(links[x], "right", "10ns")

    ## Load a SubComponent into the component - this is a "user" subcomponent since we load it here
    # To use an anonymous subcomponent, comment out these three lines
    
    # For variety, assign subcomponent types in a round-robin fashion to each component
    unit_selector = x % len(units)

    # Load a subcomponent into a slot by giving both the slot name and the name of the subcomponent to load
    sub = component.setSubComponent("compute_unit", units[unit_selector])
    
    # Since we have a handle to the subcomponent, we can pass it parameters
    sub.addParam("amount", (x % 4 + 2) % 7) 
