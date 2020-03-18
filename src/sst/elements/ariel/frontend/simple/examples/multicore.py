import sst

# This is an example only, it is not reguarly used in testing
# 8 cores, private L1 and L2, shared L3, one memory controller
# No on-chip network (bus only)

corecount = 8
clock = "2GHz"

# CPU with 'corecount' cores
ariel = sst.Component("ariel0", "ariel.ariel")
ariel.addParams( {
    "verbose" : "0",
    "maxcorequeue" : "256",
    "maxissuepercycle" : "2",
    "pipetimeout" : "0",
    "corecount" : str(corecount),
    "clock" : clock,
    "executable" : "<BINARY PATH GOES HERE>",
#    "appargcount" : ARGUMENT COUNT FOR EXECUTABLE GOES HERE,
#    "apparg0" : <ARG0 GOES HERE>,
#    "apparg1" : <ARG1 GOES HERE>, # Add additional args as needed
    "arielmode" : "1",
    "arielinterceptcalls" : "0", # Do not intercept malloc/free 
    "launchparamcount" : 1,
    "launchparam0" : "-ifeellucky", # For Pin2.14 on newer hardware
} )

# Bus between L2s and L3
membus = sst.Component("membus", "memHierarchy.Bus")
membus.addParams( { "bus_frequency" : clock } )

for x in range(0, corecount):
    # Private L1s
    # 32KB, 2-way set associative, 64B line, 2 cycle access
    # MSI coherence with LRU replacement
    l1d = sst.Component("l1cache_" + str(x), "memHierarchy.Cache")
    l1d.addParams( {
        "cache_frequency" : clock,
        "access_latency_cycles" : 2,
        "cache_size" : "32KB",
        "associativity" : 2,
        "cache_line_size" : 64,
        "replacement_policy" : "lru",
        "coherence_protocol" : "MSI",
        "L1" : 1,
    } )
    ariel_l1d_link = sst.Link("cpu_cache_link_" + str(x))
    ariel_l1d_link.connect( (ariel, "cache_link_" + str(x), "50ps"), (l1d, "high_network_0", "50ps") )

    # Private L2s
    # 128KB, 8-way set associative, 64B line, 5 cycle access
    # MSI coherence with LRU replacement
    l2 = sst.Component("l2cache_" + str(x), "memHierarchy.Cache")
    l2.addParams( {
        "cache_frequency" : clock,
        "access_latency_cycles" : 5,
        "cache_size" : "128KB",
        "associativity" : 8,
        "cache_line_size" : 64,
        "replacement_policy" : "lru",
        "coherence_protocol" : "MSI"
    })
	
    l1d_l2_link = sst.Link("l1_l2_link_" + str(x))
    l1d_l2_link.connect( (l1d, "low_network_0", "50ps"), (l2, "high_network_0", "50ps") )

    l2_bus_link = sst.Link("l2_bus_link_" + str(x))
    l2_bus_link.connect( (l2, "low_network_0", "50ps"), (membus, "high_network_" + str(x), "50ps") )

# Shared L3
# 1MB*cores, 16-way set associative, 64B line, 15 cycle access
# MSI coherence with NMRU (not-most-recently-used) replacement
l3 = sst.Component("l3cache", "memHierarchy.Cache")
l3.addParams( {
    "cache_frequency" : clock,
    "access_latency_cycles" : 15,
    "cache_size" : str(corecount) + "MB",   # 1MB/core
    "associativity" : 16,
    "cache_line_size" : 64,
    "replacement_policy" : "nmru",
    "coherence_protocol" : "MSI"
} )

l3_bus_link = sst.Link("l3_bus_link")
l3_bus_link.connect( (l3, "high_network_0", "50ps"), (membus, "low_network_0", "50ps") )

# Memory/Controller
# Using "simpleMem" memory model
# 1GB with 70ns access time
memctrl = sst.Component("memory", "memHierarchy.MemController")
memctrl.addParam("clock", "1GHz")
memory = memctrl.setSubComponent("backend", "memHierarchy.simpleMem")
memory.addParams({
    "access_time" : "70ns",
    "mem_size" : "1GB"
})

memory_bus_link = sst.Link("memory_bus_link")
memory_bus_link.connect( (l3, "low_network_0", "50ps"), (memctrl, "direct_link", "50ps") )

print("Done configuring SST model")
