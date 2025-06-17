# Import the SST module
import sst

#######################################################################
# Define the simulation components
# The distributed carwash simulation has two components
#   1. A car generator
#   2. A carwash

# Name the car generator "generator"
gen = sst.Component("generator", "simpleSimulation.distCarGenerator")

# Name the carwash "carwash"
wash = sst.Component("carwash", "simpleSimulation.distCarWash")

#######################################################################
# Parameterize the components

# Set the random seed to use for generating cars
gen.addParam("random_seed", 151515);

# Try to generate a car every two minutes
gen.addParam("arrival_frequency", 2);

# Set the carwash up to run for 4320m (72h)
wash.addParam("time", 4320)
#######################################################################
# Connect the components to each other with a link

link = sst.Link("link")

# Connect gen's 'car' port to wash's 'car' port. Configure the link to take 60s to send an event.
link.connect( ( gen, "car", "60s"), ( wash, "car", "60s" ) )
