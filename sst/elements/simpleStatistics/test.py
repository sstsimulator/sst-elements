import sst

# Define SST core options
sst.setProgramOption("timebase", "1 ps")
sst.setProgramOption("stopAtCycle", "200ns")

########################################################################

# Set the Statistic Load Level; Statistics with Enable Levels (set in 
# elementInfoStatistic) lower or equal to the load can be enabled (default = 0)
sst.setStatisticLoadLevel(7)                             ## Stat Load Level 

# Set the desired Statistic Output (sst.statOutputConsole is default)
#sst.setStatisticOutput("sst.statOutputConsole")         
#sst.setStatisticOutput("sst.statOutputTXT")             
sst.setStatisticOutput("sst.statOutputCSV")              

# Set any Statistic Output Options. (The "help" option will display available options)
sst.setStatisticOutputOptions({
			"filepath" : "./TestOutput.csv",    
			"separator" : ", "
})
#sst.setStatisticOutputOption("outputtopheader", "1")
#sst.setStatisticOutputOption("outputinlineheader", "1")
#sst.setStatisticOutputOption("outputsimtime", "1")
#sst.setStatisticOutputOption("outputrank", "1")
#sst.setStatisticOutputOption("help", "help")

########################################################################

######################################
# RANK 0 Component + Statistic Enables

# Define the simulation components
StatExample0 = sst.Component("simpleStatisticsTest0", "simpleStatistics.simpleStatistics")
StatExample0.setRank(0)

# Set Component Parameters
StatExample0.addParams({
      "rng" : """marsaglia""",
      "count" : """100""",   # Change For number of 1ns clocks
      "seed_w" : """1447""",
      "seed_z" : """1053"""
})

# Enable All Statistics for the Component
#StatExample0.enableAllStatistics()
#StatExample0.enableAllStatisticsWithRate("5ns")

## Enable Individual Statistics for the Component
#StatExample0.enableStatistics([
#      "histo_U32",
#      "histo_U64",
#      "histo_I32",
#      "histo_I64", 
#      "accum_U32",
#      "accum_U64"
#])

# Enable Individual Statistics for the Component
StatExample0.enableStatisticsWithRate({
      "histo_U32" : "5 ns",
      "histo_U64" : "10 ns",
      "histo_I32" : "25 events",
      "histo_I64" : "50 ns",
      "accum_U32" : "5 ns",
      "accum_U64" : "10 ns",
})

#### ######################################
#### # RANK 1 Component + Statistic Enables
#### 
#### # Define the simulation components
#### StatExample1 = sst.Component("simpleStatisticsTest1", "simpleStatistics.simpleStatistics")
#### StatExample1.setRank(1)
#### 
#### # Set Component Parameters
#### StatExample1.addParams({
####       "rng" : """marsaglia""",
####       "count" : """100""",   # Change For number of 1ns clocks
####       "seed_w" : """1447""",
####       "seed_z" : """1053"""
#### })
#### 
#### # Enable All Statistics for the Component
#### #StatExample1.enableAllStatistics()
#### #StatExample1.enableAllStatisticsWithRate("5ns")
#### 
#### ## Enable Individual Statistics for the Component
#### #StatExample1.enableStatistics([
#### #      "histo_U32",
#### #      "histo_U64",
#### #      "histo_I32",
#### #      "histo_I64", 
#### #      "accum_U32",
#### #      "accum_U64"
#### #])
#### 
#### # Enable Individual Statistics for the Component
#### StatExample1.enableStatisticsWithRate({
####       "histo_U32" : "5 ns",
####       "histo_U64" : "10 ns",
####       "histo_I32" : "25 events",
####       "histo_I64" : "50 ns",
####       "accum_U32" : "5 ns",
####       "accum_U64" : "10 ns",
#### })

######################################
## Globally Enable Statistics (Must Occur After Components Are Created)

# Enable All Statistics for All Components
#sst.enableAllStatisticsForAllComponents()
#sst.enableAllStatisticsWithRateForAllComponents("10ns")

# Enable All Statistics for Components defined by Name
#sst.enableAllStatisticsForComponentName("simpleStatisticsTest0")
#sst.enableAllStatisticsWithRateForComponentName("simpleStatisticsTest1", "10ns")

# Enable All Statistics for Components defined by Type
#sst.enableAllStatisticsForComponentType("simpleStatistics.simpleStatistics")
#sst.enableAllStatisticsWithRateForComponentType("simpleStatistics.simpleStatistics", "10ns")

# Enable Single Statistic for Components defined by Name
#sst.enableStatisticForComponentName("simpleStatisticsTest0", "histo_U32")
#sst.enableStatisticForComponentName("simpleStatisticsTest1", "histo_U64")
#sst.enableStatisticWithRateForComponentName("simpleStatisticsTest0", "histo_U32", "10ns")
#sst.enableStatisticWithRateForComponentName("simpleStatisticsTest1", "histo_I32", "100 events")
#sst.enableStatisticWithRateForComponentName("simpleStatisticsTest1", "histo_I64", "20ns")
#sst.enableStatisticWithRateForComponentName("simpleStatisticsTest1", "accum_U64", "30ns")

# Enable Single Statistic for Components defined by Type
#sst.enableStatisticForComponentType("simpleStatistics.simpleStatistics", "histo_U32")
#sst.enableStatisticForComponentType("simpleStatistics.simpleStatistics", "histo_U64")
#sst.enableStatisticWithRateForComponentType("simpleStatistics.simpleStatistics", "histo_U32", "5ns")
#sst.enableStatisticWithRateForComponentType("simpleStatistics.simpleStatistics", "histo_I32", "100 events")
#sst.enableStatisticWithRateForComponentType("simpleStatistics.simpleStatistics", "histo_I64", "20ns")
#sst.enableStatisticWithRateForComponentType("simpleStatistics.simpleStatistics", "accum_U64", "30ns")

