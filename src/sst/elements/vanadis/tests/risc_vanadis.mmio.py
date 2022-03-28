# test configuration for using llyr as an mmio device with vanadis
import os
import sst

DEBUG_L1 = 1
DEBUG_MEM = 1
DEBUG_CORE = 1
DEBUG_DIR = 1
DEBUG_OTHERS = 1
DEBUG_LEVEL = 10

debug_params = { "debug" : DEBUG_OTHERS, "debug_level" : 10 }

# Define SST core options
sst.setProgramOption("timebase", "1 ps")
sst.setProgramOption("stopAtCycle", "10000s")

# Constants shared across components
network_bw = "25GB/s"
cpu_clk = 2.6
tile_clk_mhz = 1
backing_size = 256
mmio_addr = 0xD0000000
statLevel = 16

# Network
core_group = 1
mmio_group = 2
dir_group = 3
mem_group = 4

l1_dst = [dir_group,mmio_group]
mmio_src = [core_group]
mmio_dst = [dir_group]
dir_src = [core_group,mmio_group]
dir_dst = [mem_group]
mem_src = [dir_group]

# CPU
verbosity = int(os.getenv("VANADIS_VERBOSE", 100))
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

# dataflow
app_dir = "/home/hughes/tools/sst/src/sst-elements/src/sst/elements/llyr/tests/"

# Define the simulation components
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
       #"executable" : os.getenv("VANADIS_EXE", "../tests/riscv/basic-io/hello-world"),
       #"executable" : os.getenv("VANADIS_EXE", "../tests/riscv/basic/basic"),
       #"executable" : os.getenv("VANADIS_EXE", "../tests/riscv/tests/rv64ui-p-add"),
       "executable" : os.getenv("VANADIS_EXE", "../tests/riscv/llyr_test/llyr"),
       "app.env_count" : 2,
       "app.env0" : "HOME=/home/hughes",
       "app.env1" : "NEWHOME=/home/hughes2",
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

vanadis_isa = os.getenv("VANADIS_ISA", "RISCV64")
vanadis_decoder = "vanadis.Vanadis" + vanadis_isa + "Decoder"
vanadis_os_hdlr = "vanadis.Vanadis" + vanadis_isa + "OSHandler"

decode0     = v_cpu_0.setSubComponent( "decoder0", vanadis_decoder )
os_hdlr     = decode0.setSubComponent( "os_handler", vanadis_os_hdlr )
#os_hdlr     = decode0.setSubComponent( "os_handler", "vanadis.VanadisMIPSOSHandler" )
branch_pred = decode0.setSubComponent( "branch_unit", "vanadis.VanadisBasicBranchUnit" )

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

icache_if = v_cpu_0.setSubComponent( "mem_interface_inst", "memHierarchy.standardInterface" )

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

dcache_if = v_cpu_0_lsq.setSubComponent( "memory_interface", "memHierarchy.standardInterface" )
dcache_if.addParams({"debug" : 0, "debug_level" : 11 })

node_os = sst.Component("os", "vanadis.VanadisNodeOS")
node_os.addParams({
	"verbose" : os_verbosity,
	"cores" : 1,
	"heap_start" : 512 * 1024 * 1024,
	"heap_end"   : (2 * 1024 * 1024 * 1024) - 4096,
	"page_size"  : 4096,
	"heap_verbose" : 0 #verbosity
})

node_os_mem_if = node_os.setSubComponent( "mem_interface", "memHierarchy.standardInterface" )

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
      "debug_level" : 11
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
      "debug_level" : 11
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
l2cache_2_l1caches.addParams(debug_params)

l2cache_2_mem = cpu0_l2cache.setSubComponent("memlink", "memHierarchy.MemNIC")
l2cache_2_mem.addParams({ "group" : core_group,
                       "destinations" : l1_dst,
                       "network_bw" : network_bw})
l2cache_2_mem.addParams(debug_params)

cache_bus = sst.Component("bus", "memHierarchy.Bus")
cache_bus.addParams({
      "bus_frequency" : cpu_clock,
})

## dataflow
df_0 = sst.Component("df_0", "llyr.LlyrDataflow")
df_0.addParams({
    "verbose": 20,
    "clock" : str(tile_clk_mhz) + "GHz",
    "device_addr"   : mmio_addr,
    "starting_addr" : 0xC0000000,
    "mem_init"      : app_dir + "int-1.mem",
    #"application"   : "conditional.in",
    "application"   : app_dir + "gemm.in",
    #"application"   : app_dir + "llvm_in/cdfg-example-02.ll",
    "hardware_graph": app_dir + "hardware.cfg",
    "mapper"        : "llyr.mapper.simple"

})

df_iface = df_0.setSubComponent("iface", "memHierarchy.standardInterface")
df_iface.addParams(debug_params)

df_link = df_iface.setSubComponent("memlink", "memHierarchy.MemLink")
df_link.addParams(debug_params)

df_l1cache = sst.Component("df_l1", "memHierarchy.Cache")
df_l1cache.addParams({
      "access_latency_cycles" : "2",
      "cache_frequency" : str(tile_clk_mhz) + "GHz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MESI",
      "cache_size" : "512B",
      "associativity" : "1",
      "cache_line_size" : "16",
      "L1" : "1",
      "debug" : DEBUG_L1,
      "debug_level" : DEBUG_LEVEL
})

df_l1_link = df_l1cache.setSubComponent("cpulink", "memHierarchy.MemLink") # Non-network link from device to device's L1
df_l1_link.addParams(debug_params)

df_l1_nic = df_l1cache.setSubComponent("memlink", "memHierarchy.MemNIC") # Network link
df_l1_nic.addParams({"group" : mmio_group,
                    "sources" : mmio_src,
                    "destinations" : mmio_dst,
                    "network_bw" : network_bw })
df_l1_nic.addParams(debug_params)

## network
dir = sst.Component("directory", "memHierarchy.DirectoryController")
dir.addParams({
      "clock" : str(tile_clk_mhz) + "GHz",
      "entry_cache_size" : 16384,
      "mshr_num_entries" : 16,
      "addr_range_start" : 0,
      "addr_range_end" : mmio_addr - 1,
      "debug" : DEBUG_DIR,
      "debug_level" : 10,
})

dir_nic = dir.setSubComponent("cpulink", "memHierarchy.MemNIC")
dir_nic.addParams({
      "group" : dir_group,
      "sources" : dir_src,
      "destinations" : dir_dst,
      "network_bw" : network_bw,
      "network_input_buffer_size" : "2KiB",
      "network_output_buffer_size" : "2KiB",
})
dir_nic.addParams(debug_params)

memctrl = sst.Component("memory", "memHierarchy.MemController")
memctrl.addParams({
    "debug" : DEBUG_MEM,
    "debug_level" : DEBUG_LEVEL,
    "clock" : str(tile_clk_mhz) + "GHz",
    "addr_range_end" : mmio_addr - 1,
})

memory = memctrl.setSubComponent("backend", "memHierarchy.simpleMem")
memory.addParams({
      "access_time" : "100 ns",
      #"mem_size" : str(backing_size) + "B",
      "mem_size" : str(16) + "GiB",
})

mem_nic = memctrl.setSubComponent("cpulink", "memHierarchy.MemNIC")
mem_nic.addParams(debug_params)
mem_nic.addParams({
    "group" : mem_group,
    "sources" : mem_src,
    "network_bw" : network_bw,
    "network_input_buffer_size" : "2KiB",
    "network_output_buffer_size" : "2KiB"
})

chiprtr = sst.Component("chiprtr", "merlin.hr_router")
chiprtr.addParams({
      "xbar_bw" : "1GB/s",
      "id" : "0",
      "input_buf_size" : "1KB",
      "num_ports" : "4",
      "flit_size" : "72B",
      "output_buf_size" : "1KB",
      "link_bw" : "1GB/s",
      "topology" : "merlin.singlerouter"
})
chiprtr.setSubComponent("topology","merlin.singlerouter")


# Enable SST Statistics Outputs for this simulation
sst.setStatisticLoadLevel(statLevel)
sst.enableAllStatisticsForAllComponents({"type":"sst.AccumulatorStatistic"})
sst.setStatisticOutput("sst.statOutputTXT", { "filepath" : "output.csv" })


# Define the simulation links
#                cpu/l1/cpu_l1_nic - os_l1/os_l1_nic
#                     |
#  mmio/l1/mmio_l1_nic - chiprtr - dir_nic/dir/mem
#
# Connect CPU to CPU L1 via the CPU's interface and the L1's cpulink handler
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

# Connect the MMIO L1 to the network via the L1's memlink NIC handler
link_l2_rtr = sst.Link("link_l2")
link_l2_rtr.connect( (l2cache_2_mem, "port", "500ps"), (chiprtr, "port0", "500ps"))

# Connect directory to the network via the directory's NIC handler
link_dir_rtr = sst.Link("link_dir")
link_dir_rtr.connect( (dir_nic, "port", "1000ps"), (chiprtr, "port1", "1000ps"))

# Connect directory to the memory
link_dir_mem = sst.Link("link_mem")
link_dir_mem.connect( (mem_nic, "port", "1000ps"), (chiprtr, "port2", "1000ps") )

# Connect MMIO to MMIO L1 via the MMIO's interface and the L1's cpulink handler
link_df_l1 = sst.Link("link_mmio")
link_df_l1.connect( (df_link, "port", "500ps"), (df_l1_link, "port", "500ps") )

link_device_rtr = sst.Link("link_device")
link_device_rtr.connect( (df_l1_nic, "port", "500ps"), (chiprtr, "port3", "500ps"))

link_core0_os_link = sst.Link("link_core0_os_link")
link_core0_os_link.connect( (os_hdlr, "os_link", "5ns"), (node_os, "core0", "5ns") )




