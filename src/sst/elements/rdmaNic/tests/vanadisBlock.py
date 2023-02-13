import os
import sst

#map = "0x0:0x80000000:0x80000000"

coherence_protocol="MESI"

#physMemSize = "4GiB"

l2_debug = 0
cache_debug = 0
stdMem_debug = 0

debug_level=11
debug_addr=0x6280

debugPython=False

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

if (verbosity > 0):
    print( "Verbosity: " + str(verbosity) + " -> loading Vanadis CPU type: " + vanadis_cpu_type )

tlbParams = {
    "debug_level": 0,
    "hitLatency": 10,
    "num_hardware_threads": 1,
    "num_tlb_entries_per_thread": 64,
    "tlb_set_size": 4,
}

tlbWrapperParams = {
    "debug_level": 0,
}

class Vanadis_Builder:
    def __init__(self):
        pass

    def build( self, nodeId, cpuId ):

        if debugPython:
            print( "nodeId {} cpuID={}".format( nodeId, cpuId ) )

        prefix = 'node' + str(nodeId) + '.cpu' + str( cpuId ) 
        cpu = sst.Component(prefix, vanadis_cpu_type)
        cpu.addParams({
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
            cpu.addParams({ "app.argc" : app_args_count })
            if (verbosity > 0):
                print( "Identified " + str(app_args_count) + " application arguments, adding to input parameters." )
            arg_start = 1
            for next_arg in app_args_list:
                if (verbosity > 0):
                    print( "arg" + str(arg_start) + " = " + next_arg )
                cpu.addParams({ "app.arg" + str(arg_start) : next_arg })
                arg_start = arg_start + 1
        else:
            if (verbosity > 0):
                print( "No application arguments found, continuing with argc=0" )

        decode = cpu.setSubComponent( "decoder0", "vanadis.VanadisMIPSDecoder" )

        decode.addParams({
            "uop_cache_entries" : 1536,
            "predecode_cache_entries" : 4
        })

        os_hdlr = decode.setSubComponent( "os_handler", "vanadis.VanadisMIPSOSHandler" )
        os_hdlr.addParams({
            "verbose" : os_verbosity,
            "brk_zero_memory" : "yes"
        })

        branch_pred = decode.setSubComponent( "branch_unit", "vanadis.VanadisBasicBranchUnit")
        branch_pred.addParams({
            "branch_entries" : 32
        })

        icache_if = cpu.setSubComponent( "mem_interface_inst", "memHierarchy.standardInterface" )
        icache_if.addParam("coreId",cpuId)
        icache_if.addParams({
            "debug" : stdMem_debug,
            "debug_level" : 11,
        })

        cpu_lsq = cpu.setSubComponent( "lsq", "vanadis.VanadisBasicLoadStoreQueue" )
        cpu_lsq.addParams({
            "verbose" : verbosity,
            "address_mask" : 0xFFFFFFFF,
            "load_store_entries" : lsq_entries,
            "fault_non_written_loads_after" : 0,
            "check_memory_loads" : "no",
            "allow_speculated_operations": "no"
        })

        dcache_if = cpu_lsq.setSubComponent( "memory_interface", "memHierarchy.standardInterface" )
        dcache_if.addParam("coreId",cpuId)
        dcache_if.addParams( {
            "debug" : stdMem_debug,
            "debug_level" : 11,
        })
		
        # L1 D-Cache
        l1cache = sst.Component(prefix + ".l1dcache", "memHierarchy.Cache")
        l1cache.addParams({
            "access_latency_cycles" : "2",
            "cache_frequency" : cpu_clock,
            "replacement_policy" : "lru",
            "coherence_protocol" : coherence_protocol,
            "associativity" : "8",
            "cache_line_size" : "64",
            "cache_size" : "32 KB",
            "L1" : "1",
            "debug_level" : debug_level,
            "debug" : cache_debug,
            "debug_addr" : debug_addr,
        })

        l1dcache_2_cpu     = l1cache.setSubComponent("cpulink", "memHierarchy.MemLink")
        l1dcache_2_l2cache = l1cache.setSubComponent("memlink", "memHierarchy.MemLink")

        # L1 I-Cache
        l1icache = sst.Component(prefix + ".l1icache", "memHierarchy.Cache")
        l1icache.addParams({
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
            "debug_level" : debug_level,
            "debug" : cache_debug,
            "debug_addr" : debug_addr,
        })

        # Bus
        cache_bus = sst.Component(prefix + ".bus", "memHierarchy.Bus")
        cache_bus.addParams({
            "bus_frequency" : cpu_clock,
        })

        # L2 D-Cache
        l2cache = sst.Component(prefix + ".l2cache", "memHierarchy.Cache")
        l2cache.addParams({
            "access_latency_cycles" : "14",
            "cache_frequency" : cpu_clock,
            "replacement_policy" : "lru",
            "coherence_protocol" : coherence_protocol,
            "associativity" : "16",
            "cache_line_size" : "64",
            "mshr_latency_cycles" : 3,
            "cache_size" : "1MB",
            "debug": l2_debug,
            "debug_level" : debug_level,
            "debug_addr" : debug_addr,
        })

        l2cache_2_cpu = l2cache.setSubComponent("cpulink", "memHierarchy.MemLink")

        # CPU D-TLB
        dtlbWrapper = sst.Component(prefix+".dtlb", "mmu.tlb_wrapper")
        dtlbWrapper.addParams(tlbWrapperParams)
        dtlb = dtlbWrapper.setSubComponent("tlb", "mmu.simpleTLB" );
        dtlb.addParams(tlbParams)

        # CPU I-TLB
        itlbWrapper = sst.Component(prefix+".itlb", "mmu.tlb_wrapper")
        itlbWrapper.addParams(tlbWrapperParams)
        itlbWrapper.addParam("exe",True)
        itlb = itlbWrapper.setSubComponent("tlb", "mmu.simpleTLB" );
        itlb.addParams(tlbParams)

        # CPU (data) -> D-TLB
        link = sst.Link(prefix+".link_cpu_dtlb")
        link.connect( (dcache_if, "port", "1ns"), (dtlbWrapper, "cpu_if", "1ns") )

        # CPU (instruction) -> I-TLB
        link = sst.Link(prefix+".link_cpu_itlb")
        link.connect( (icache_if, "port", "1ns"), (itlbWrapper, "cpu_if", "1ns") )

        l1icache_2_cpu     = l1icache.setSubComponent("cpulink", "memHierarchy.MemLink")
        l1icache_2_l2cache = l1icache.setSubComponent("memlink", "memHierarchy.MemLink")

        # D-TLB -> D-L1 
        link = sst.Link(prefix+".link_l1cache")
        link.connect( (dtlbWrapper, "cache_if", "1ns"), (l1dcache_2_cpu, "port", "1ns") )

        # I-TLB -> I-L1 
        link = sst.Link(prefix+".link_l1icache")
        link.connect( (itlbWrapper, "cache_if", "1ns"), (l1icache_2_cpu, "port", "1ns") )

        # L1 I-Cache to bus
        link = sst.Link(prefix + ".link_l1dcache_l2cache")
        link.connect( (l1dcache_2_l2cache, "port", "1ns"), (cache_bus, "high_network_0", "1ns") )

        # L1 D-Cache to bus
        link = sst.Link(prefix + ".link_l1icache_l2cache")
        link.connect( (l1icache_2_l2cache, "port", "1ns"), (cache_bus, "high_network_1", "1ns") )

        # BUS to L2 cache
        link = sst.Link(prefix+".link_bus_l2cache")
        link.connect( (cache_bus, "low_network_0", "1ns"), (l2cache_2_cpu, "port", "1ns") )

        return cpu, l2cache, dtlb, itlb
