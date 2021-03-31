import os
import sst

# Define SST core options
sst.setProgramOption("timebase", "1ps")
sst.setProgramOption("stopAtCycle", "0 ns")

# Tell SST what statistics handling we want
sst.setStatisticLoadLevel(4)

verbosity = int(os.getenv("VANADIS_VERBOSE", 0))
os_verbosity = os.getenv("VANADIS_OS_VERBOSE", verbosity)
pipe_trace_file = os.getenv("VANADIS_PIPE_TRACE", "")
lsq_entries = os.getenv("VANADIS_LSQ_ENTRIES", 32)

rob_slots = os.getenv("VANADIS_ROB_SLOTS", 64)
retires_per_cycle = os.getenv("VANADIS_RETIRES_PER_CYCLE", 4)
issues_per_cycle = os.getenv("VANADIS_ISSUES_PER_CYCLE", 4)
decodes_per_cycle = os.getenv("VANADIS_DECODES_PER_CYCLE", 4)

integer_arith_cycles = int(os.getenv("VANADIS_INTEGER_ARITH_CYCLES", 2))
integer_arith_units = int(os.getenv("VANADIS_INTEGER_ARITH_UNITS", 2))
fp_arith_cycles = int(os.getenv("VANADIS_FP_ARITH_CYCLES", 8))
fp_arith_units = int(os.getenv("VANADIS_FP_ARITH_UNITS", 2))
branch_arith_cycles = int(os.getenv("VANADIS_BRANCH_ARITH_CYCLES", 2))

auto_clock_sys = os.getenv("VANADIS_AUTO_CLOCK_SYSCALLS", "no")

cpu_clock = os.getenv("VANADIS_CPU_CLOCK", "2.3GHz")

vanadis_cpu_type = "vanadisdbg.VanadisCPU"

#if (verbosity > 0):
#	print("Verbosity (" + str(verbosity) + ") is non-zero, using debug version of Vanadis.")
#	vanadis_cpu_type = "vanadisdbg.VanadisCPU"

print("Verbosity: " + str(verbosity) + " -> loading Vanadis CPU type: " + vanadis_cpu_type)
print("Auto-clock syscalls: " + str(auto_clock_sys))

v_cpu_0 = sst.Component("v0", vanadis_cpu_type)
v_cpu_0.addParams({
       "clock" : cpu_clock,
#       "executable" : "./tests/hello-mips",
#       "executable" : "./tests/hello-musl",
#       "executable" : "./tests/core-perf-musl",
#       "executable" : "./tests/stream-musl",
#       "executable" : "./tests/stream-mini-musl",
#       "executable" : "./tests/test-printf",
#       "executable" : "./tests/test-env",
#       "executable" : "./tests/lulesh2.0",
#       "executable" : "./tests/luxtxt",
#       "executable" : "./tests/stream-fortran",
#       "executable" : "./tests/test-fp",
#       "executable" : os.getenv("VANADIS_EXE", "./tests/stream-mini-musl"),
       "executable" : os.getenv("VANADIS_EXE", "./tests/small/basic-io/hello-world"),
       "app.env_count" : 2,
       "app.env0" : "HOME=/home/sdhammo",
       "app.env1" : "NEWHOME=/home/sdhammo2",
#       "app.argc" : 4,
#       "app.arg1" : "16",
#       "app.arg2" : "8",
#       "app.arg3" : "8",
#       "max_cycle" : 100000000,
       "verbose" : verbosity,
       "physical_fp_registers" : 168,
       "physical_int_registers" : 180,
       "integer_arith_cycles" : integer_arith_cycles,
       "integer_arith_units" : integer_arith_units,
       "fp_arith_cycles" : fp_arith_cycles,
       "fp_arith_units" : fp_arith_units,
       "branch_unit_cycles" : branch_arith_cycles,
       "print_int_reg" : 1,
       "pipeline_trace_file" : pipe_trace_file,
       "reorder_slots" : rob_slots,
       "decodes_per_cycle" : decodes_per_cycle,
       "issues_per_cycle" :  issues_per_cycle,
       "retires_per_cycle" : retires_per_cycle,
       "auto_clock_syscall" : auto_clock_sys,
       "pause_when_retire_address" : os.getenv("VANADIS_HALT_AT_ADDRESS", 0)
#       "reorder_slots" : 32,
#       "decodes_per_cycle" : 2,
#       "issues_per_cycle" :  1,
#       "retires_per_cycle" : 1
})

app_args = os.getenv("VANADIS_EXE_ARGS", "")

if app_args != "":
	app_args_list = app_args.split(" ")
	# We have a plus 1 because the executable name is arg0
	app_args_count = len( app_args_list ) + 1
	v_cpu_0.addParams({ "app.argc" : app_args_count })
	print("Identified " + str(app_args_count) + " application arguments, adding to input parameters.")
	arg_start = 1
	for next_arg in app_args_list:
		print("arg" + str(arg_start) + " = " + next_arg)
		v_cpu_0.addParams({ "app.arg" + str(arg_start) : next_arg })
		arg_start = arg_start + 1
else:
	print("No application arguments found, continuing with argc=0")

decode0     = v_cpu_0.setSubComponent( "decoder0", "vanadis.VanadisMIPSDecoder" )
os_hdlr     = decode0.setSubComponent( "os_handler", "vanadis.VanadisMIPSOSHandler" )
branch_pred = decode0.setSubComponent( "branch_unit", "vanadis.VanadisBasicBranchUnit")

decode0.addParams({
	"uop_cache_entries" : 1536,
	"predecode_cache_entries" : 4
})

os_hdlr.addParams({
	"verbose" : os_verbosity,
	"brk_zero_memory" : "yes"
})

branch_pred.addParams({
	"branch_entries" : 32
})

icache_if = v_cpu_0.setSubComponent( "mem_interface_inst", "memHierarchy.memInterface" )

#v_cpu_0_lsq = v_cpu_0.setSubComponent( "lsq", "vanadis.VanadisStandardLoadStoreQueue" )
v_cpu_0_lsq = v_cpu_0.setSubComponent( "lsq", "vanadis.VanadisSequentialLoadStoreQueue" )
v_cpu_0_lsq.addParams({
	"verbose" : verbosity,
	"address_mask" : 0xFFFFFFFF,
#	"address_trace" : "address-lsq2.trace",
#	"allow_speculated_operations" : 0,
#	"load_store_entries" : 56,
	"load_store_entries" : lsq_entries,
	"fault_non_written_loads_after" : 0,
	"check_memory_loads" : "no"
})

dcache_if = v_cpu_0_lsq.setSubComponent( "memory_interface", "memHierarchy.memInterface" )

node_os = sst.Component("os", "vanadis.VanadisNodeOS")
node_os.addParams({
	"verbose" : os_verbosity,
	"cores" : 1,
	"heap_start" : 512 * 1024 * 1024,
	"heap_end"   : (2 * 1024 * 1024 * 1024) - 4096,
	"page_size"  : 4096,
	"heap_verbose" : 0 #verbosity
})

node_os_mem_if = node_os.setSubComponent( "mem_interface", "memHierarchy.memInterface" )

os_l1dcache = sst.Component("node_os.l1dcache", "memHierarchy.Cache")
os_l1dcache.addParams({
      "access_latency_cycles" : "2",
      "cache_frequency" : cpu_clock,
      "replacement_policy" : "lru",
      "coherence_protocol" : "MESI",
      "associativity" : "8",
      "cache_line_size" : "64",
      "cache_size" : "32 KB",
      "L1" : "1",
      "debug" : 0,
      "debug_level" : 0
})

cpu0_l1dcache = sst.Component("cpu0.l1dcache", "memHierarchy.Cache")
cpu0_l1dcache.addParams({
      "access_latency_cycles" : "2",
      "cache_frequency" : cpu_clock,
      "replacement_policy" : "lru",
      "coherence_protocol" : "MESI",
      "associativity" : "8",
      "cache_line_size" : "64",
      "cache_size" : "32 KB",
      "L1" : "1",
      "debug" : 0,
      "debug_level" : 0
})
l1dcache_2_cpu     = cpu0_l1dcache.setSubComponent("cpulink", "memHierarchy.MemLink")
l1dcache_2_l2cache = cpu0_l1dcache.setSubComponent("memlink", "memHierarchy.MemLink")

cpu0_l1icache = sst.Component("cpu0.l1icache", "memHierarchy.Cache")
cpu0_l1icache.addParams({
      "access_latency_cycles" : "2",
      "cache_frequency" : cpu_clock,
      "replacement_policy" : "lru",
      "coherence_protocol" : "MESI",
      "associativity" : "8",
      "cache_line_size" : "64",
      "cache_size" : "32 KB",
      "prefetcher" : "cassini.NextBlockPrefetcher",
      "prefetcher.reach" : 1,
      "L1" : "1",
})
l1icache_2_cpu     = cpu0_l1icache.setSubComponent("cpulink", "memHierarchy.MemLink")
l1icache_2_l2cache = cpu0_l1icache.setSubComponent("memlink", "memHierarchy.MemLink")

cpu0_l2cache = sst.Component("l2cache", "memHierarchy.Cache")
cpu0_l2cache.addParams({
      "access_latency_cycles" : "14",
      "cache_frequency" : cpu_clock,
      "replacement_policy" : "lru",
      "coherence_protocol" : "MESI",
      "associativity" : "16",
      "cache_line_size" : "64",
      "cache_size" : "1MB",
})
l2cache_2_l1caches = cpu0_l2cache.setSubComponent("cpulink", "memHierarchy.MemLink")
l2cache_2_mem = cpu0_l2cache.setSubComponent("memlink", "memHierarchy.MemNIC")

l2cache_2_mem.addParams({
	"group" : 1,
	"network_bw" : "25GB/s"
})

cache_bus = sst.Component("bus", "memHierarchy.Bus")
cache_bus.addParams({
      "bus_frequency" : cpu_clock,
})

comp_chiprtr = sst.Component("chiprtr", "merlin.hr_router")
comp_chiprtr.addParams({
      "xbar_bw" : "1GB/s",
      "link_bw" : "1GB/s",
      "input_buf_size" : "2KB",
      "num_ports" : "3",
      "flit_size" : "72B",
      "output_buf_size" : "2KB",
      "id" : "0",
      "topology" : "merlin.singlerouter"
})
comp_chiprtr.setSubComponent("topology","merlin.singlerouter")

dirctrl = sst.Component("dirctrl", "memHierarchy.DirectoryController")
dirctrl.addParams({
      "coherence_protocol" : "MSI",
      "entry_cache_size" : "1024",
      "debug" : 0,
      "debug_level" : 0,
      "addr_range_start" : "0x0",
      "addr_range_end" : "0xFFFFFFFF"
})
dirtoM = dirctrl.setSubComponent("memlink", "memHierarchy.MemLink")
dirNIC = dirctrl.setSubComponent("cpulink", "memHierarchy.MemNIC")
dirNIC.addParams({
      "network_bw" : "25GB/s",
      "group" : 2,
})

memctrl = sst.Component("memory", "memHierarchy.MemController")
memctrl.addParams({
      "clock" : cpu_clock,
      "backend.mem_size" : "4GiB",
      "backing" : "malloc"
})
memToDir = memctrl.setSubComponent("cpulink", "memHierarchy.MemLink")

memory = memctrl.setSubComponent("backend", "memHierarchy.simpleMem")
memory.addParams({
      "mem_size" : "4GiB",
      "access_time" : "1 ns"
})

sst.setStatisticOutput("sst.statOutputConsole")
v_cpu_0.enableAllStatistics()
decode0.enableAllStatistics()
v_cpu_0_lsq.enableAllStatistics()
branch_pred.enableAllStatistics()

link_cpu0_l1dcache_link = sst.Link("link_cpu0_l1dcache_link")
link_cpu0_l1dcache_link.connect( (dcache_if, "port", "1ns"), (l1dcache_2_cpu, "port", "1ns") )

link_cpu0_l1icache_link = sst.Link("link_cpu0_l1icache_link")
link_cpu0_l1icache_link.connect( (icache_if, "port", "1ns"), (l1icache_2_cpu, "port", "1ns") )

link_os_l1dcache_link = sst.Link("link_os_l1dcache_link")
link_os_l1dcache_link.connect( (node_os_mem_if, "port", "1ns"), (os_l1dcache, "high_network_0", "1ns") )

link_l1dcache_l2cache_link = sst.Link("link_l1dcache_l2cache_link")
link_l1dcache_l2cache_link.connect( (l1dcache_2_l2cache, "port", "1ns"), (cache_bus, "high_network_0", "1ns") )

link_l1icache_l2cache_link = sst.Link("link_l1icache_l2cache_link")
link_l1icache_l2cache_link.connect( (l1icache_2_l2cache, "port", "1ns"), (cache_bus, "high_network_1", "1ns") )

link_os_l1dcache_l2cache_link = sst.Link("link_os_l1dcache_l2cache_link")
link_os_l1dcache_l2cache_link.connect( (os_l1dcache, "low_network_0", "1ns"), (cache_bus, "high_network_2", "1ns") )

link_bus_l2cache_link = sst.Link("link_bus_l2cache_link")
link_bus_l2cache_link.connect( (cache_bus, "low_network_0", "1ns"), (l2cache_2_l1caches, "port", "1ns") )

link_l2cache_2_rtr = sst.Link("link_l2cache_2_rtr")
link_l2cache_2_rtr.connect( (l2cache_2_mem, "port", "1ns"), (comp_chiprtr, "port0", "1ns") )

link_dir_2_rtr = sst.Link("link_dir_2_rtr")
link_dir_2_rtr.connect( (comp_chiprtr, "port1", "1ns"), (dirNIC, "port", "1ns") )

link_dir_2_mem = sst.Link("link_dir_2_mem")
link_dir_2_mem.connect( (dirtoM, "port", "1ns"), (memToDir, "port", "1ns") )

link_core0_os_link = sst.Link("link_core0_os_link")
link_core0_os_link.connect( (os_hdlr, "os_link", "5ns"), (node_os, "core0", "5ns") )


