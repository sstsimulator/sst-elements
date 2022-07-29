# Automatically generated SST Python input
import sst

# Define SST core options
sst.setProgramOption("timebase", "1 ps")
sst.setProgramOption("stopAtCycle", "10000s")

# Constants shared across components
tile_clk_mhz = 1
backing_size = 16384
l1_size = 512
verboseLevel = 0
statLevel = 16
mainDebug = 0
otherDebug = 0
debugLevel = 0

# Define the simulation components
df_0 = sst.Component("df_0", "llyr.LlyrDataflow")
df_0.addParams({
   "verbose" : str(verboseLevel),
   "clock" : str(tile_clk_mhz) + "GHz",
   "mem_init"      : "int-1.mem",
   "application"   : "gemm.in",
   "hardware_graph": "graph_mesh_25.hdw",
   "mapper"        : "llyr.mapper.simple"
})
iface = df_0.setSubComponent("iface", "memHierarchy.standardInterface")

df_l1cache = sst.Component("df_l1", "memHierarchy.Cache")
df_l1cache.addParams({
   "access_latency_cycles" : "2",
   "cache_frequency" : str(tile_clk_mhz) + "GHz",
   "replacement_policy" : "lru",
   "coherence_protocol" : "MESI",
   "cache_size" : str(l1_size) + "B",
   "associativity" : "1",
   "cache_line_size" : "16",
   "verbose" : str(verboseLevel),
   "debug" : str(otherDebug),
   "debug_level" : str(debugLevel),
   "L1" : "1"
})

df_memory = sst.Component("memory", "memHierarchy.MemController")
df_memory.addParams({
   "backing" : "mmap",
   "verbose" : str(verboseLevel),
   "debug" : str(otherDebug),
   "debug_level" : str(debugLevel),
   "addr_range_start" : "0",
   "clock" : str(tile_clk_mhz) + "GHz",
})

backend = df_memory.setSubComponent("backend", "memHierarchy.simpleMem")
backend.addParams({
   "access_time" : "100 ns",
   "mem_size" : str(backing_size) + "B",
})

# Enable SST Statistics Outputs for this simulation
sst.setStatisticLoadLevel(statLevel)
sst.enableAllStatisticsForAllComponents({"type":"sst.AccumulatorStatistic"})
#sst.setStatisticOutput("sst.statOutputTXT", { "filepath" : "output.csv" })

# Define the simulation links
link_df_cache_link = sst.Link("link_cpu_cache_link")
link_df_cache_link.connect( (iface, "port", "1ps"), (df_l1cache, "high_network_0", "1ps") )
link_df_cache_link.setNoCut()

link_mem_bus_link = sst.Link("link_mem_bus_link")
link_mem_bus_link.connect( (df_l1cache, "low_network_0", "5ps"), (df_memory, "direct_link", "5ps") )


