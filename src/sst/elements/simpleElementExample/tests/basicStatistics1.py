# Import the SST module
import sst

# The basicStatisticsX.py scripts demonstrate user-side configuration of statistics.
#
# This example demonstrates:
#   1. Generating statistics at a regular interval throughout simulation
#   2. Various ways to select which statistics are enabled
#
# This component has no links and SST will produce a warning because that is an unusual configuration
# that often points to a mis-configuration. For this simulation, the warning can be ignored.
#
# Relevant code:
#   simpleElementExample/basicStatistics.h
#   simpleElementExample/basicStatistics.cc
#   simpleElementExample/basicEvent.h
# Output:
#   simpleElementExample/tests/refFiles/basicStatistics1.out
#   simpleElementExample/tests/refFiles/basicStatistics1.csv
#

### Create two components
component0 = sst.Component("StatisticComponent0", "simpleElementExample.basicStatistics")
component1 = sst.Component("StatisticComponent1", "simpleElementExample.basicStatistics")



### Parameterize the component.
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
# Statistics are enabled if:
#   1. The statistic is enabled via an 'enableAll...' call and 
#       its load level is <= the load level setting (global or per-component/type) 
#   2. The statistic is specifically named in an 'enableStatistic...' or 'enableStatistics' call
 
## Limit the verbosity of statistics to any with a load level from 0-4
# This component's statistics range from 1-4 (see sst-info)
# Default: Set global load level
sst.setStatisticLoadLevel(4)

# Option: Set per-component-type load level
#sst.setStatisticLoadLevelForComponentType("simpleElementExample.basicStatistics", 4)

# Option: Set per-component load level
#sst.setStatisticLoadLevelForComponentName("StatisticComponent0", 7)
#sst.setStatisticLoadLevelForComponentName("StatisticComponent1", 7)


## Determine where statistics should be sent
sst.setStatisticOutput("sst.statOutputCSV", { "filepath" : "./basicStatistics1.csv", "seperator" : "," } ) 


## Enable statistics on the components
# There are many ways to enable statistics, allowing users to specify exactly what they want to see and how
# By default, this script is setup to have the simulation print all statistics for the 
# "simpleElementExample.basicStatistics" component types every 50ns
# Other options, besides 'enableAllStatisticsForComponenType' are commented out below
# Parameters are optional in all calls

# This is new compared to basicStatistics0 - we add parameters to the statistics
statParams = {
        "rate" : "50ns" # This is how often the simulation should report statistics
}

# Default: Enable all statistics for components of a certain type
sst.enableAllStatisticsForComponentType("simpleElementExample.basicStatistics", statParams)

# Option: Enable all statistics for all components in the simulation
#sst.enableAllStatisticsForAllComponents(statParams)

# Option: Enable statistics for a specific component given that component's name (uncomment one or both lines)
#sst.enableAllStatisticsForComponentName("StatisticComponent0", statParams)
#sst.enableAllStatisticsForComponentName("StatisticComponent1", statParams)

# Option: Enable a list of statistics for a given component name or type
statSet = ["UINT32_statistic", "UINT32_statistic_duplicate", 
           "UINT64_statistic", "INT32_statistic", "INT64_statistic", "SUBID_statistic"]
#sst.enableStatisticsForComponentType("simpleElementExample.basicStatistics", statSet, statParams)
# or
#sst.enableStatisticsForComponentName("StatisticComponent0", statSet, statParams)
#sst.enableStatisticsForComponentName("StatisticComponent1", statSet, statParams)

# Option: Individually enable a statistic on a certain component type (uncomment one or more of these lines)
#sst.enableStatisticForComponentType("simpleElementExample.basicStatistics", "UINT32_statistic", statParams)
#sst.enableStatisticForComponentType("simpleElementExample.basicStatistics", "UINT32_statistic_duplicate", statParams)
#sst.enableStatisticForComponentType("simpleElementExample.basicStatistics", "UINT64_statistic", statParams)
#sst.enableStatisticForComponentType("simpleElementExample.basicStatistics", "INT32_statistic", statParams)
#sst.enableStatisticForComponentType("simpleElementExample.basicStatistics", "INT64_statistic", statParams)
#sst.enableStatisticForComponentType("simpleElementExample.basicStatistics", "SUBID_statistic", statParams)

# Option: Individually enable a statistic on a given component name (uncomment one or more of these lines)
#sst.enableStatisticForComponentName("StatisticComponent0", "UINT32_statistic", statParams)
#sst.enableStatisticForComponentName("StatisticComponent0", "UINT32_statistic_duplicate", statParams)
#sst.enableStatisticForComponentName("StatisticComponent0", "UINT64_statistic", statParams)
#sst.enableStatisticForComponentName("StatisticComponent0", "INT32_statistic", statParams)
#sst.enableStatisticForComponentName("StatisticComponent0", "INT64_statistic", statParams)
#sst.enableStatisticForComponentName("StatisticComponent0", "SUBID_statistic", statParams)
#sst.enableStatisticForComponentName("StatisticComponent1", "UINT32_statistic", statParams)
#sst.enableStatisticForComponentName("StatisticComponent1", "UINT32_statistic_duplicate", statParams)
#sst.enableStatisticForComponentName("StatisticComponent1", "UINT64_statistic", statParams)
#sst.enableStatisticForComponentName("StatisticComponent1", "INT32_statistic", statParams)
#sst.enableStatisticForComponentName("StatisticComponent1", "INT64_statistic", statParams)
#sst.enableStatisticForComponentName("StatisticComponent1", "SUBID_statistic", statParams)
