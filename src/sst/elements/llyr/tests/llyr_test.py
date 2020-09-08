# Automatically generated SST Python input
import sst

# Define SST core options
sst.setProgramOption("timebase", "1 ps")
sst.setProgramOption("stopAtCycle", "10000s")

max_addr_gb = 1
tile_clk_mhz = 1

# Define the simulation components
df_0 = sst.Component("df_0", "llyr.LlyrDataflow")
df_0.addParams({
      "verbose": 10,
      "clockcount" : """100000000""",
      "clock" : str(tile_clk_mhz) + "GHz",
})

df_l1cache = sst.Component("l1cache", "memHierarchy.Cache")
df_l1cache.addParams({
      "access_latency_cycles" : "2",
      "cache_frequency" : str(tile_clk_mhz) + "GHz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MESI",
      "associativity" : "4",
      "cache_line_size" : "64",
      "debug" : "0",
      "L1" : "1",
      "cache_size" : "512B"
})

df_memory = sst.Component("memory", "memHierarchy.MemController")
df_memory.addParams({
      "clock" : "1GHz"
})

backend = df_memory.setSubComponent("backend", "memHierarchy.simpleMem")
backend.addParams({
    "access_time" : "10 ns",
    "mem_size" : str(max_addr_gb) + "GiB",
})

sst.setStatisticOutput("sst.statOutputCSV")
sst.enableAllStatisticsForAllComponents()

sst.setStatisticOutputOptions( {
        "filepath"  : "output.csv"
} )

# Define the simulation links
link_df_cache_link = sst.Link("link_cpu_cache_link")
link_df_cache_link.connect( (df_0, "cache_link", "1000ps"), (df_l1cache, "high_network_0", "1000ps") )
link_df_cache_link.setNoCut()

link_mem_bus_link = sst.Link("link_mem_bus_link")
link_mem_bus_link.connect( (df_l1cache, "low_network_0", "50ps"), (df_memory, "direct_link", "50ps") )


