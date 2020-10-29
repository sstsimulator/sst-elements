import sst

# Define SST core options
sst.setProgramOption("timebase", "1ps")
sst.setProgramOption("stopAtCycle", "0 ns")

# Tell SST what statistics handling we want
sst.setStatisticLoadLevel(4)

verbosity = 16

v_cpu_0 = sst.Component("v0", "vanadis.VanadisCPU")
v_cpu_0.addParams({
       "clock" : "2.0GHz",
#       "executable" : "./tests/hello-gem5",
#       "executable" : "./tests/hello-mips",
#       "executable" : "./tests/hello-musl",
#       "executable" : "./tests/core-perf-musl",
       "executable" : "./tests/stream-musl",
#       "executable" : "./tests/inst-test/and",
       "app.env_count" : 1,
       "app.env0" : "HOME=/home/sdhammo",
       "max_cycle" : 1000000,
       "verbose" : verbosity,
       "physical_fp_registers" : 96,
       "print_int_reg" : 1,
#      "pipeline_trace_file" : "pipe-lsq2.trace",
       "reorder_slots" : 128,
      "decodes_per_cycle" : 2,
      "issues_per_cycle" :  2,
      "retires_per_cycle" : 2
})

decode0   = v_cpu_0.setSubComponent( "decoder0", "vanadis.VanadisMIPSDecoder" )
os_hdlr   = decode0.setSubComponent( "os_handler", "vanadis.VanadisMIPSOSHandler" )

decode0.addParams({

})

os_hdlr.addParams({
	"verbose" : verbosity,
	"brk_zero_memory" : "yes"
})

icache_if = v_cpu_0.setSubComponent( "mem_interface_inst", "memHierarchy.memInterface" )

#v_cpu_0_lsq = v_cpu_0.setSubComponent( "lsq", "vanadis.VanadisStandardLoadStoreQueue" )
v_cpu_0_lsq = v_cpu_0.setSubComponent( "lsq", "vanadis.VanadisSequentialLoadStoreQueue" )
v_cpu_0_lsq.addParams({
	"verbose" : verbosity,
	"address_mask" : 0xFFFFFFFF,
#	"address_trace" : "address-lsq2.trace",
#	"allow_speculated_operations" : 0,
	"load_store_entries" : 8
})

dcache_if = v_cpu_0_lsq.setSubComponent( "memory_interface", "memHierarchy.memInterface" )

node_os = sst.Component("os", "vanadis.VanadisNodeOS")
node_os.addParams({
	"verbose" : verbosity,
	"cores" : 1
})

node_os_mem_if = node_os.setSubComponent( "mem_interface", "memHierarchy.memInterface" )

os_l1dcache = sst.Component("node_os.l1dcache", "memHierarchy.Cache")
os_l1dcache.addParams({
      "access_latency_cycles" : "1",
      "cache_frequency" : "2 GHz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MESI",
      "associativity" : "2",
      "cache_line_size" : "64",
      "cache_size" : "1 KB",
      "L1" : "1",
      "debug" : 0,
      "debug_level" : 0
})

cpu0_l1dcache = sst.Component("cpu0.l1dcache", "memHierarchy.Cache")
cpu0_l1dcache.addParams({
      "access_latency_cycles" : "1",
      "cache_frequency" : "2 GHz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MESI",
      "associativity" : "2",
      "cache_line_size" : "64",
      "cache_size" : "8 KB",
      "L1" : "1",
      "debug" : 0,
      "debug_level" : 0
})

cpu0_l1icache = sst.Component("cpu0.l1icache", "memHierarchy.Cache")
cpu0_l1icache.addParams({
      "access_latency_cycles" : "1",
      "cache_frequency" : "2 GHz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MESI",
      "associativity" : "2",
      "cache_line_size" : "64",
      "cache_size" : "8 KB",
      "prefetcher" : "cassini.NextBlockPrefetcher",
      "prefetcher.reach" : 1,
      "L1" : "1",
})

cpu0_l2cache = sst.Component("l2cache", "memHierarchy.Cache")
cpu0_l2cache.addParams({
      "access_latency_cycles" : "1",
      "cache_frequency" : "2 GHz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MESI",
      "associativity" : "8",
      "cache_line_size" : "64",
      "cache_size" : "256 KB",
})

cache_bus = sst.Component("bus", "memHierarchy.Bus")
cache_bus.addParams({
      "bus_frequency" : "2 GHz",
})

memctrl = sst.Component("memory", "memHierarchy.MemController")
memctrl.addParams({
      "clock" : "1GHz",
      "backend.mem_size" : "4GiB",
      "backing" : "malloc"
})

memory = memctrl.setSubComponent("backend", "memHierarchy.simpleMem")
memory.addParams({
      "mem_size" : "4GiB",
      "access_time" : "1 ns"
})

sst.setStatisticOutput("sst.statOutputConsole")
v_cpu_0.enableAllStatistics()

link_cpu0_l1dcache_link = sst.Link("link_cpu0_l1dcache_link")
link_cpu0_l1dcache_link.connect( (dcache_if, "port", "1ns"), (cpu0_l1dcache, "high_network_0", "1ns") )

link_cpu0_l1icache_link = sst.Link("link_cpu0_l1icache_link")
link_cpu0_l1icache_link.connect( (icache_if, "port", "1ns"), (cpu0_l1icache, "high_network_0", "1ns") )

link_os_l1dcache_link = sst.Link("link_os_l1dcache_link")
link_os_l1dcache_link.connect( (node_os_mem_if, "port", "1ns"), (os_l1dcache, "high_network_0", "1ns") )

link_l1dcache_l2cache_link = sst.Link("link_l1dcache_l2cache_link")
link_l1dcache_l2cache_link.connect( (cpu0_l1dcache, "low_network_0", "1ns"), (cache_bus, "high_network_0", "1ns") )

link_l1icache_l2cache_link = sst.Link("link_l1icache_l2cache_link")
link_l1icache_l2cache_link.connect( (cpu0_l1icache, "low_network_0", "1ns"), (cache_bus, "high_network_1", "1ns") )

link_os_l1dcache_l2cache_link = sst.Link("link_os_l1dcache_l2cache_link")
link_os_l1dcache_l2cache_link.connect( (os_l1dcache, "low_network_0", "1ns"), (cache_bus, "high_network_2", "1ns") )

link_bus_l2cache_link = sst.Link("link_bus_l2cache_link")
link_bus_l2cache_link.connect( (cache_bus, "low_network_0", "1ns"), (cpu0_l2cache, "high_network_0", "1ns") )

link_l2cache_mem_link = sst.Link("link_l2cache_mem_link")
link_l2cache_mem_link.connect( (cpu0_l2cache, "low_network_0", "1ns"), (memctrl, "direct_link", "1ns") )

link_core0_os_link = sst.Link("link_core0_os_link")
link_core0_os_link.connect( (os_hdlr, "os_link", "5ns"), (node_os, "core0", "5ns") )
