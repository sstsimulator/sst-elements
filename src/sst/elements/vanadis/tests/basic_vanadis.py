import sst

# Define SST core options
sst.setProgramOption("timebase", "1ps")
sst.setProgramOption("stopAtCycle", "0 ns")

# Tell SST what statistics handling we want
sst.setStatisticLoadLevel(4)

v_cpu_0 = sst.Component("v0", "vanadis.VanadisCPU")
v_cpu_0.addParams({
       "clock" : "1.0GHz",
       "executable" : "./tests/hello-mips", 
       "max_cycle" : 1000,
       "verbose" : 16,
       "physical_fp_registers" : 96,
       "print_int_reg" : 1
})

decode0   = v_cpu_0.setSubComponent( "decoder0", "vanadis.VanadisMIPSDecoder" )
os_hdlr   = decode0.setSubComponent( "os_handler", "vanadis.VanadisMIPSOSHandler" )

os_hdlr.addParams({
	"verbose" : 16
})

dcache_if = v_cpu_0.setSubComponent( "mem_interface_data", "memHierarchy.memInterface" )
icache_if = v_cpu_0.setSubComponent( "mem_interface_inst", "memHierarchy.memInterface" )

cpu0_l1dcache = sst.Component("cpu0.l1dcache", "memHierarchy.Cache")
cpu0_l1dcache.addParams({
      "access_latency_cycles" : "1",
      "cache_frequency" : "2 GHz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MSI",
      "associativity" : "2",
      "cache_line_size" : "64",
      "cache_size" : "1 KB",
      "L1" : "1",
})

cpu0_l1icache = sst.Component("cpu0.l1icache", "memHierarchy.Cache")
cpu0_l1icache.addParams({
      "access_latency_cycles" : "1",
      "cache_frequency" : "2 GHz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MSI",
      "associativity" : "2",
      "cache_line_size" : "64",
      "cache_size" : "1 KB",
      "L1" : "1",
})

cpu0_l2cache = sst.Component("l2cache", "memHierarchy.Cache")
cpu0_l2cache.addParams({
      "access_latency_cycles" : "1",
      "cache_frequency" : "2 GHz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MSI",
      "associativity" : "8",
      "cache_line_size" : "64",
      "cache_size" : "2 KB",
})

cache_bus = sst.Component("bus", "memHierarchy.Bus")
cache_bus.addParams({
      "bus_frequency" : "2 GHz",
})

memctrl = sst.Component("memory", "memHierarchy.MemController")
memctrl.addParams({
      "clock" : "1GHz",
})

memory = memctrl.setSubComponent("backend", "memHierarchy.simpleMem")
memory.addParams({
      "mem_size" : "4GiB",
      "access_time" : "1 ns"
})

link_cpu0_l1dcache_link = sst.Link("link_cpu0_l1dcache_link")
link_cpu0_l1dcache_link.connect( (dcache_if, "port", "1ns"), (cpu0_l1dcache, "high_network_0", "1ns") )

link_cpu0_l1icache_link = sst.Link("link_cpu0_l1icache_link")
link_cpu0_l1icache_link.connect( (icache_if, "port", "1ns"), (cpu0_l1icache, "high_network_0", "1ns") )

link_l1dcache_l2cache_link = sst.Link("link_l1dcache_l2cache_link")
link_l1dcache_l2cache_link.connect( (cpu0_l1dcache, "low_network_0", "1ns"), (cache_bus, "high_network_0", "1ns") )

link_l1icache_l2cache_link = sst.Link("link_l1icache_l2cache_link")
link_l1icache_l2cache_link.connect( (cpu0_l1icache, "low_network_0", "1ns"), (cache_bus, "high_network_1", "1ns") )

link_bus_l2cache_link = sst.Link("link_bus_l2cache_link")
link_bus_l2cache_link.connect( (cache_bus, "low_network_0", "1ns"), (cpu0_l2cache, "high_network_0", "1ns") )

link_l2cache_mem_link = sst.Link("link_l2cache_mem_link")
link_l2cache_mem_link.connect( (cpu0_l2cache, "low_network_0", "1ns"), (memctrl, "direct_link", "1ns") )

