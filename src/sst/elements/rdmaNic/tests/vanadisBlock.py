import os
import sst

map = "0x0:0x80000000:0x80000000"

coherence_protocol="MESI"
#coherence_protocol="MSI"

cache_debug = 0
stdMem_debug = 0

debug_addr=0x7ffffb80
debug_start_time = 26760650595 
#debug_start_time = 0

executable="./app/rdma/msg"

verbosity = int(os.getenv("VANADIS_VERBOSE", 0))
os_verbosity = os.getenv("VANADIS_OS_VERBOSE", 0)
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

cpu_clock = os.getenv("VANADIS_CPU_CLOCK", "2.3GHz")

vanadis_cpu_type = "vanadis.dbg_VanadisCPU"
#vanadis_cpu_type = "vanadis.VanadisCPU"

if (verbosity > 0):
	print( "Verbosity (" + str(verbosity) + ") is non-zero, using debug version of Vanadis." )
	vanadis_cpu_type = "vanadisdbg.VanadisCPU"

print( "Verbosity: " + str(verbosity) + " -> loading Vanadis CPU type: " + vanadis_cpu_type )


class Vanadis_Builder:
	def __init__(self):
		pass

	def build( self, nodeId, cpuId ):

		print( "nodeId {} cpuID={}".format( nodeId, cpuId ) )

		prefix = 'node' + str(nodeId) + '.cpu' + str( cpuId ) 
		v_cpu_0 = sst.Component(prefix, vanadis_cpu_type)
		v_cpu_0.addParams({
			   "clock" : cpu_clock,
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
			   "pause_when_retire_address" : os.getenv("VANADIS_HALT_AT_ADDRESS", 0)
		})

		app_args = os.getenv("VANADIS_EXE_ARGS", "")

		if app_args != "":
			app_args_list = app_args.split(" ")
			# We have a plus 1 because the executable name is arg0
			app_args_count = len( app_args_list ) + 1
			v_cpu_0.addParams({ "app.argc" : app_args_count })
			print( "Identified " + str(app_args_count) + " application arguments, adding to input parameters." )
			arg_start = 1
			for next_arg in app_args_list:
				print( "arg" + str(arg_start) + " = " + next_arg )
				v_cpu_0.addParams({ "app.arg" + str(arg_start) : next_arg })
				arg_start = arg_start + 1
		else:
			print( "No application arguments found, continuing with argc=0" )

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

		icache_if = v_cpu_0.setSubComponent( "mem_interface_inst", "memHierarchy.standardInterface" )
		icache_if.addParam("coreId",cpuId)
		icache_if.addParams({
			  "debug" : stdMem_debug,
			  "debug_level" : 11,
			  "debug_start_time": debug_start_time,
		})

		v_cpu_0_lsq = v_cpu_0.setSubComponent( "lsq", "vanadis.VanadisSequentialLoadStoreQueue" )
		v_cpu_0_lsq.addParams({
			"verbose" : verbosity,
			"address_mask" : 0xFFFFFFFF,
			"load_store_entries" : lsq_entries,
			"fault_non_written_loads_after" : 0,
			"check_memory_loads" : "no",
			"allow_speculated_operations": "no"
		})

		dcache_if = v_cpu_0_lsq.setSubComponent( "memory_interface", "memHierarchy.standardInterface" )
		dcache_if.addParam("coreId",cpuId)
		dcache_if.addParams( {
			  "debug" : stdMem_debug,
			  "debug_level" : 11,
			  "debug_start_time": debug_start_time,
		})
		

		node_os = sst.Component(prefix + ".os", "vanadis.VanadisNodeOS")
		node_os.addParams({
			"verbose" : os_verbosity,
			"cores" : 1,
			"heap_start" : 512 * 1024 * 1024,
			"heap_end"   : (2 * 1024 * 1024 * 1024) - 4096,
			"page_size"  : 4096,
			"heap_verbose" : 0, #verbosity
	        "executable" : os.getenv("VANADIS_EXE", executable),
		    "app.argc" : 4,
            "app.arg0" : "IMB",
            "app.arg1" : "PingPong",
            "app.arg2" : "-iter",
            "app.arg3" : "3",
            "app.env_count" : 1,
            "app.env0" : "HOME=/home/sdhammo",
            "app.env1" : "VANADIS_THREAD_NUM=0",
		})

		node_os_mem_if = node_os.setSubComponent( "mem_interface", "memHierarchy.standardInterface" )
		node_os_mem_if.addParam("coreId",cpuId)
		node_os_mem_if.addParams({
			  "debug" : stdMem_debug,
			  "debug_level" : 11,
			  "debug_start_time": debug_start_time,
		})

		os_l1dcache = sst.Component(prefix + ".node_os.l1dcache", "memHierarchy.Cache")
		os_l1dcache.addParams({
			  "access_latency_cycles" : "2",
			  "cache_frequency" : cpu_clock,
			  "replacement_policy" : "lru",
			  "coherence_protocol" : coherence_protocol,
			  "associativity" : "8",
			  "cache_line_size" : "64",
			  "cache_size" : "32 KB",
			  "L1" : "1",
            "debug_mask" : 1<<3 | 1<<5,
            "debug_level" : "20",
			  "debug" : cache_debug,
			  "debug_start_time": debug_start_time,
			"debug_addr" : debug_addr,
		})

		cpu0_l1dcache = sst.Component(prefix + ".l1dcache", "memHierarchy.Cache")
		cpu0_l1dcache.addParams({
			  "access_latency_cycles" : "2",
			  "cache_frequency" : cpu_clock,
			  "replacement_policy" : "lru",
			  "coherence_protocol" : coherence_protocol,
			  "associativity" : "8",
			  "cache_line_size" : "64",
			  "cache_size" : "32 KB",
			  "L1" : "1",
            "debug_mask" : 1<<3 | 1<<5,
            "debug_level" : "20",
			  "debug" : cache_debug,
			  "debug_start_time": debug_start_time,
			"debug_addr" : debug_addr,
		})

		l1dcache_2_cpu     = cpu0_l1dcache.setSubComponent("cpulink", "memHierarchy.MemLink")
		l1dcache_2_l2cache = cpu0_l1dcache.setSubComponent("memlink", "memHierarchy.MemLink")

		cpu0_l1icache = sst.Component(prefix + ".l1icache", "memHierarchy.Cache")
		cpu0_l1icache.addParams({
			  "access_latency_cycles" : "2",
			  "cache_frequency" : cpu_clock,
			  "replacement_policy" : "lru",
			  "coherence_protocol" : coherence_protocol,
			  "associativity" : "8",
			  "cache_line_size" : "64",
			  "cache_size" : "32 KB",
			  #"prefetcher" : "cassini.NextBlockPrefetcher",
			  #"prefetcher.reach" : 1,
			  "L1" : "1",
            "debug_mask" : 1<<3 | 1<<5,
            "debug_level" : "20",
			  "debug" : cache_debug,
			  "debug_start_time": debug_start_time,
			"debug_addr" : debug_addr,
		})

		cache_bus = sst.Component(prefix + ".bus", "memHierarchy.Bus")
		cache_bus.addParams({
			  "bus_frequency" : cpu_clock,
		})

		l1icache_2_cpu     = cpu0_l1icache.setSubComponent("cpulink", "memHierarchy.MemLink")
		l1icache_2_l2cache = cpu0_l1icache.setSubComponent("memlink", "memHierarchy.MemLink")

		link_cpu0_l1dcache_link = sst.Link(prefix+".link_cpu0_l1dcache")
		link_cpu0_l1dcache_link.connect( (dcache_if, "port", "1ns"), (l1dcache_2_cpu, "port", "1ns") )

		link_cpu0_l1icache_link = sst.Link(prefix + ".link_cpu0_l1icache")
		link_cpu0_l1icache_link.connect( (icache_if, "port", "1ns"), (l1icache_2_cpu, "port", "1ns") )

		link_os_l1dcache_link = sst.Link(prefix + ".link_os_l1dcache")
		link_os_l1dcache_link.connect( (node_os_mem_if, "port", "1ns"), (os_l1dcache, "high_network_0", "1ns") )

		link_l1dcache_l2cache_link = sst.Link(prefix + ".link_l1dcache_l2cache")
		link_l1dcache_l2cache_link.connect( (l1dcache_2_l2cache, "port", "1ns"), (cache_bus, "high_network_0", "1ns") )

		link_l1icache_l2cache_link = sst.Link(prefix + ".link_l1icache_l2cache")
		link_l1icache_l2cache_link.connect( (l1icache_2_l2cache, "port", "1ns"), (cache_bus, "high_network_1", "1ns") )

		link_os_l1dcache_l2cache_link = sst.Link(prefix + ".link_os_l1dcache_l2cache")
		link_os_l1dcache_l2cache_link.connect( (os_l1dcache, "low_network_0", "1ns"), (cache_bus, "high_network_2", "1ns") )

		link_core0_os_link = sst.Link(prefix + ".link_core0_os")
		link_core0_os_link.connect( (v_cpu_0, "os_link", "5ns"), (node_os, "core0", "5ns") )

		return (cache_bus, "low_network_0", "1ns")
