import sst

# Test custom commands (StandardMem::CustomReq/CustomResp) through memHierarchy
# without requiring an external backend.
#
# cpu (Miranda, custom read/write opcodes) -> L1 -> bus -> memory (simpleMem, default custom command handler)
#
# The L1/bus and bus/memory links may be cut so that custom events are serialized
# across ranks or passed across threads.

sst.setStatisticLoadLevel(1)
sst.setStatisticOutput("sst.statOutputConsole")

cpu = sst.Component("cpu", "miranda.BaseCPU")
cpu.addParams({
    "verbose" : 0,
    "clock" : "2GHz",
})

gen = cpu.setSubComponent("generator", "miranda.STREAMBenchGeneratorCustomCmd")
gen.addParams({
    "verbose" : 0,
    "n" : 1000,
    "operandwidth" : 8,
    "write_cmd" : 10,
    "read_cmd" : 20,
})
cpu.enableStatistics(["custom_reqs"])

l1 = sst.Component("l1cache", "memHierarchy.Cache")
l1.addParams({
    "access_latency_cycles" : "2",
    "cache_frequency" : "2GHz",
    "replacement_policy" : "lru",
    "coherence_protocol" : "MESI",
    "associativity" : "4",
    "cache_line_size" : "64",
    "L1" : "1",
    "cache_size" : "8KiB",
})

bus = sst.Component("bus", "memHierarchy.Bus")
bus.addParams({
    "bus_frequency" : "2GHz",
})

memctrl = sst.Component("memory", "memHierarchy.MemController")
memctrl.addParams({
    "clock" : "1GHz",
    "verbose" : 0, # defCustomCmdHandler requests shootdowns, which MemController warns about for every event
    "addr_range_end" : 512*1024*1024-1,
    "customCmdHandler" : "memHierarchy.defCustomCmdHandler",
})
memory = memctrl.setSubComponent("backend", "memHierarchy.simpleMem")
memory.addParams({
    "access_time" : "100ns",
    "mem_size" : "512MiB",
})

link_cpu_l1 = sst.Link("link_cpu_l1")
link_cpu_l1.connect( (cpu, "cache_link", "1000ps"), (l1, "highlink", "1000ps") )
link_cpu_l1.setNoCut()

link_l1_bus = sst.Link("link_l1_bus")
link_l1_bus.connect( (l1, "lowlink", "50ps"), (bus, "highlink0", "50ps") )

link_bus_mem = sst.Link("link_bus_mem")
link_bus_mem.connect( (bus, "lowlink0", "50ps"), (memctrl, "highlink", "50ps") )
