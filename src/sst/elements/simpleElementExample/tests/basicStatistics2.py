# Import the SST module
import sst

# The basicStatisticsX.py scripts demonstrate user-side configuration of statistics.
#
# This example demonstrates:
#   1. Different methods for reporting statistics: accumulator, histogram, unique count 
#
# This component has no links and SST will produce a warning because that is an unusual configuration
# that often points to a mis-configuration. For this simulation, the warning can be ignored.
#
# Relevant code:
#   simpleElementExample/basicStatistics.h
#   simpleElementExample/basicStatistics.cc
#   simpleElementExample/basicEvent.h
# Output:
#   simpleElementExample/tests/refFiles/basicStatistics2.out
#   simpleElementExample/tests/refFiles/basicStatistics2.txt
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
 
## Limit the verbosity of statistics to any with a load level from 0-4
# This component's statistics range from 1-4 (see sst-info)
sst.setStatisticLoadLevel(4)


## Determine where statistics should be sent
# CSV is harder to read because the different statistic types all have different fields
# so this breaks with the previous examples and uses the text output
sst.setStatisticOutput("sst.statOutputTXT", { "filepath" : "./basicStatistics2.txt" } ) 


## Enable statistics on the components
# This example demonstrates reporting statistics in different ways
# We will report:
#   - the statistics in "accumStats" as accumulators (default)
#   - the statistics in "histStats" histograms
#   - the statistics in "uniqStats" as a unique count
accumStats = ["UINT32_statistic", "UINT32_statistic_duplicate", "UINT64_statistic"]
histStats = ["INT32_statistic", "INT64_statistic"]
uniqStats = ["SUBID_statistic"]

# Histograms are statically constructed so require some dimension info
histogramParams = {
        "type"     : "sst.HistogramStatistic",  # default = "sst.AccumulatorStatistic"  Statistic type
        "minvalue" : -10000,                    # default = 0                           Minimum value in histogram
        "binwidth" : 2500,                      # default = 5000                        Size of each histogram bin
        "numbins"  : 8,                         # default = 100                         Number of bins to track
        "dumpbinsonoutput" : 1,                 # default = true (1)                    Dump the range (names) of 
                                                #                                           each bin too
        "includeoutofbounds" : 1,               # default = true (1)                    Track values that fall outside 
                                                #                                           the histogram bins
        }

uniqueCountParams = {
        "type" : "sst.UniqueCountStatistic",    # default = "sst.AccumulatorStatistic"  Statistic type
        }

sst.enableStatisticsForComponentType("simpleElementExample.basicStatistics", accumStats)
sst.enableStatisticsForComponentType("simpleElementExample.basicStatistics", histStats, histogramParams)
sst.enableStatisticsForComponentType("simpleElementExample.basicStatistics", uniqStats, uniqueCountParams)

