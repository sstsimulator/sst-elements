# Import the SST module
import sst

# Define the simulation component
# Name it "carwash"
wash = sst.Component("carwash", "simpleSimulation.simpleCarWash")

# Set the carwash up to run for 4320m (72h)
wash.addParam("time", 4320)

# Set the random seed to use for generating cars
wash.addParam("random_seed", 151515);

# Try to generate a car every two minutes
wash.addParam("arrival_frequency", 2);

