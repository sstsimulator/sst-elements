# Import the SST module
import sst

# The basicStatisticsX.py scripts demonstrate user-side configuration of statistics.
# Each one focuses on a different aspect of user-side configuration
#
# This example demonstrates:
#   1. Default output behavior (reporting statistics at the end of simulation)
#   2. Various output formats for statistics
#
# This component has no links and SST will produce a warning because that is an unusual configuration
# that often points to a mis-configuration. For this simulation, the warning can be ignored.
#
# Relevant code:
#   simpleElementExample/basicStatistics.h
#   simpleElementExample/basicStatistics.cc
#   simpleElementExample/basicEvent.h
#
# Output:
#   simpleElementExample/tests/refFiles/basicStatistics0.out
#   simpleElementExample/tests/refFiles/basicStatistics0.csv 
#

### Create two components (to compare different components' output in the CSV file)
component0 = sst.Component("StatisticComponent0", "simpleElementExample.basicStatistics")
component1 = sst.Component("StatisticComponent1", "simpleElementExample.basicStatistics")



### Parameterize the components.
# Run 'sst-info simpleElementExample.basicStatistics' at the command line 
# to see parameter documentation
params0 = {
        "marsagliaZ" : 438,     # Seed for Marsaglia RNG
        "marsagliaW" : 9375794, # Seed for Marsaglia RNG
        "mersenne" : 102485,    # Seed for Mersenne RNG
        "run_cycles" : 1000,    # Number of cycles to run for
        "subids" : 3            # Number of SUBID_statistic instances
}
component0.addParams(params0)
params1 = {
        "marsagliaZ" : 957537,  # Seed for Marsaglia RNG
        "marsagliaW" : 5857,    # Seed for Marsaglia RNG
        "mersenne" : 860,       # Seed for Mersenne RNG
        "run_cycles" : 1200,    # Number of cycles to run for
        "subids" : 6            # Number of SUBID_statistic instances
}
component1.addParams(params1)



### Enable statistics
## Limit the verbosity of statistics to any with a load level from 0-4
# This component's statistics range from 1-4 (see sst-info)
sst.setStatisticLoadLevel(4)


## Determine where statistics should be sent. By default this script uses CSV, other options are
# commented out below. Output locations are case-insensitive (e.g., statOutputCSV = statoutputcsv).

# Default: Output to CSV. Filename and seperator can be specified
sst.setStatisticOutput("sst.statOutputCSV", { "filepath" : "./basicStatistics0.csv", "seperator" : "," } ) 

# Option: Output to the terminal
#sst.setStatisticOutput("sst.statoutputconsole") 

# Option: Output to a text file
#sst.setStatisticOutput("sst.statOutputTXT", { "filepath" : "./basicStatistics0.txt" } )

# Option: Output to HDF5. Requires sst-core to be configured with HDF5 library.
#sst.setStatisticOutput("sst.statoutputhd5f")

# Option: Output to JSON
#sst.setStatisticOutput("sst.statOutputJSON", { "filepath" : "./basicStatistics0.json" } )


## Enable statistics on the components
sst.enableAllStatisticsForComponentType("simpleElementExample.basicStatistics")

