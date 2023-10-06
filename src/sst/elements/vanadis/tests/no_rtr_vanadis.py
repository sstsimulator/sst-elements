import os
import sst


dbg_addr = [0x20019ec0,0xfefefefefefefec0]
#dbg_addr = [536977088,18374403900871474880]
#dbg_addr = "0"
mh_dbg = 0 

#vanadis_isa = os.getenv("VANADIS_ISA", "MIPS")
#isa="mipsel"
vanadis_isa = os.getenv("VANADIS_ISA", "RISCV64")
isa="riscv64"

group = "basic-io"
test = "hello-world"
test = "hello-world-cpp"
#test = "openat"
#test = "printf-check"
#test = "read-write"
#test = "unlink"
#test = "unlinkat"

#group = "basic-math"
#test = "sqrt-double"
#test = "sqrt-float"

#group = "basic-ops"
#test = "test-branch"
#test = "test-shfit"

group = "misc"
#test = "mt-dgemm"
test = "stream-fortran"
#test = "stream"
#test = "gettime"
#test = "splitLoad"
#test = "hpcg"

# Define SST core options
sst.setProgramOption("timebase", "1ps")

# Tell SST what statistics handling we want
sst.setStatisticLoadLevel(4)

full_exe_name = os.getenv("VANADIS_EXE", "./tests/small/" + group + "/" + test +  "/" + isa + "/" + test )
exe_name= full_exe_name.split("/")[-1]

verbosity = int(os.getenv("VANADIS_VERBOSE", 0))
os_verbosity = 0 #os.getenv("VANADIS_OS_VERBOSE", verbosity)
pipe_trace_file = os.getenv("VANADIS_PIPE_TRACE", "")
lsq_ld_entries = os.getenv("VANADIS_LSQ_LD_ENTRIES", 16)
lsq_st_entries = os.getenv("VANADIS_LSQ_ST_ENTRIES", 8)
#lsq_mask = 0xFFFFFFFFFFFFFFFF
lsq_mask = 0xFFFFFFFF


rob_slots = os.getenv("VANADIS_ROB_SLOTS", 64)
retires_per_cycle = os.getenv("VANADIS_RETIRES_PER_CYCLE", 4)
issues_per_cycle = os.getenv("VANADIS_ISSUES_PER_CYCLE", 4)
decodes_per_cycle = os.getenv("VANADIS_DECODES_PER_CYCLE", 4)

integer_arith_cycles = int(os.getenv("VANADIS_INTEGER_ARITH_CYCLES", 2))
integer_arith_units = int(os.getenv("VANADIS_INTEGER_ARITH_UNITS", 2))
fp_arith_cycles = int(os.getenv("VANADIS_FP_ARITH_CYCLES", 8))
fp_arith_units = int(os.getenv("VANADIS_FP_ARITH_UNITS", 2))
branch_arith_cycles = int(os.getenv("VANADIS_BRANCH_ARITH_CYCLES", 2))

cpu_clock = os.getenv("VANADIS_CPU_CLOCK", "2.3GHz")

vanadis_cpu_type = "vanadis.dbg_VanadisCPU"

#if (verbosity > 0):
#	print("Verbosity (" + str(verbosity) + ") is non-zero, using debug version of Vanadis.")
#	vanadis_cpu_type = "vanadisdbg.VanadisCPU"

print("Verbosity: " + str(verbosity) + " -> loading Vanadis CPU type: " + vanadis_cpu_type)
print("Auto-clock syscalls: " + str(auto_clock_sys))

v_cpu_0 = sst.Component("v0", vanadis_cpu_type)
v_cpu_0.addParams({
       "clock" : cpu_clock,
#       "max_cycle" : 100000000,
       "verbose" : verbosity,
       # "start_verbose_when_issue_address": 0x10b36,
       # "start_verbose_when_issue_address": 0x1f0a0,
       # "start_verbose_when_issue_address": 0x7a522,
       # "start_verbose_when_issue_address": 0x7f42c,
        #"start_verbose_when_issue_address": 0x106f4,
        #"start_verbose_when_issue_address": 0x79ee4,
       #"pause_when_retire_address" : 0x79ef8,
       "dbg_mask" : 0,
       "physical_fp_registers" : 168,
       "physical_int_registers" : 180,
       "integer_arith_cycles" : integer_arith_cycles,
       "integer_arith_units" : integer_arith_units,
       "fp_arith_cycles" : fp_arith_cycles,
       "fp_arith_units" : fp_arith_units,
       "branch_unit_cycles" : branch_arith_cycles,
       "print_int_reg" : 1,
       "print_fp_reg" : 0,
       "print_issue_tables" : False,
       "print_retire_tables" : False,
       "print_rob" : True,
       "pipeline_trace_file" : pipe_trace_file,
       "reorder_slots" : rob_slots,
       "decodes_per_cycle" : decodes_per_cycle,
       "issues_per_cycle" :  issues_per_cycle,
       "retires_per_cycle" : retires_per_cycle,
       #"pause_when_retire_address" : os.getenv("VANADIS_HALT_AT_ADDRESS", 0),
       #"start_verbose_when_issue_address" : os.getenv("VANADIS_START_DBG_AT_ADDRESS", 0) 
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

vanadis_decoder = "vanadis.Vanadis" + vanadis_isa + "Decoder"
vanadis_os_hdlr = "vanadis.Vanadis" + vanadis_isa + "OSHandler"

if vanadis_isa == "MIPS":
	lsq_mask = 0xFFFFFFFF

decode0     = v_cpu_0.setSubComponent( "decoder0", vanadis_decoder )
os_hdlr     = decode0.setSubComponent( "os_handler", vanadis_os_hdlr )
#os_hdlr     = decode0.setSubComponent( "os_handler", "vanadis.VanadisMIPSOSHandler" )
branch_pred = decode0.setSubComponent( "branch_unit", "vanadis.VanadisBasicBranchUnit" )

decode0.addParams({
	"uop_cache_entries" : 1536,
	"predecode_cache_entries" : 4
})

os_hdlr.addParams({
})

branch_pred.addParams({
	"branch_entries" : 32
})

icache_if = v_cpu_0.setSubComponent( "mem_interface_inst", "memHierarchy.standardInterface" )

#v_cpu_0_lsq = v_cpu_0.setSubComponent( "lsq", "vanadis.VanadisStandardLoadStoreQueue" )
v_cpu_0_lsq = v_cpu_0.setSubComponent( "lsq", "vanadis.VanadisBasicLoadStoreQueue" )
v_cpu_0_lsq.addParams({
	"verbose" : verbosity,
	"address_mask" : lsq_mask,
	"max_loads" : lsq_ld_entries
	"max_stores" : lsq_st_entries
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
	"heap_verbose" : verbosity,
    "executable" : full_exe_name,
    "process0.arg0" : exe_name,
    "process0..env_count" : 2,
    "process0.env0" : "HOME=/home/sdhammo",
    "process0.env1" : "NEWHOME=/home/sdhammo2",
#    "process0.argc" : 4,
#    "process0.arg1" : "16",
#    "process0.arg2" : "8",
#    "process0.arg3" : "8",
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
      "debug" : mh_dbg,
      "debug_level" : 10,
      "debug_addr" : dbg_addr, 
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
      "debug" : mh_dbg,
      "debug_level" : 10,
      "debug_addr" : dbg_addr, 
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
      "debug" : mh_dbg,
      "debug_level" : 10,
      "debug_addr" : dbg_addr, 
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
      "debug" : mh_dbg,
      "debug_level" : 10,
      "debug_addr" : dbg_addr, 
})
l2cache_2_l1caches = cpu0_l2cache.setSubComponent("cpulink", "memHierarchy.MemLink")
l2cache_2_mem = cpu0_l2cache.setSubComponent("memlink", "memHierarchy.MemLink")

cache_bus = sst.Component("bus", "memHierarchy.Bus")
cache_bus.addParams({
      "bus_frequency" : cpu_clock,
})

dirctrl = sst.Component("dirctrl", "memHierarchy.DirectoryController")
dirctrl.addParams({
      "coherence_protocol" : "MESI",
      "entry_cache_size" : "1024",
      "debug" : 0,
      "debug_level" : 0,
      "addr_range_start" : "0x0",
      "addr_range_end" : "0xFFFFFFFF",
      "debug" : mh_dbg,
      "debug_level" : 10,
      "debug_addr" : dbg_addr, 
})
dirtoM = dirctrl.setSubComponent("memlink", "memHierarchy.MemLink")
dirtoCPU = dirctrl.setSubComponent("cpulink", "memHierarchy.MemLink")

memctrl = sst.Component("memory", "memHierarchy.MemController")
memctrl.addParams({
      "clock" : cpu_clock,
      "backend.mem_size" : "4GiB",
      "backing" : "malloc",
      "initBacking": 1,
      "addr_range_start": 0, 
      "addr_range_end": 0xffffffff, 
      "debug" : mh_dbg,
      "debug_level" : 10,
      "debug_addr" : dbg_addr, 
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


link_l2cache_2_dir = sst.Link("link_l2cache_2_dir")
link_l2cache_2_dir.connect( (dirtoCPU, "port", "1ns"), (l2cache_2_mem, "port", "1ns") )

link_dir_2_mem = sst.Link("link_dir_2_mem")
link_dir_2_mem.connect( (dirtoM, "port", "1ns"), (memToDir, "port", "1ns") )

link_core0_os_link = sst.Link("link_core0_os_link")
link_core0_os_link.connect( (v_cpu_0, "os_link", "5ns"), (node_os, "core0", "5ns") )


