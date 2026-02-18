# Automatically generated SST Python input
import sst
from sst.merlin import *
import os
import sys,getopt

L1cachesz = "8 KB"
L2cachesz = "32 KB"
L3cachesz = "32 KB"
L1assoc = 2
L2assoc = 2
L3assoc = 2
L1Replacp = "lru"
L2Replacp = "lru"
L3Replacp = "lru"
L2MSHR = 32
L3MSHR = 32
MSIMESI = "MSI"
Pref1 = "cassini.NextBlockPrefetcher"
Pref2 = "cassini.NextBlockPrefetcher"
Executable = os.getenv('OMP_EXE', "ompbarrier/ompbarrier.x")

# The parameters for processes are sub-parameters, and need to be keyed differently.
# This function accomplishes prefixing the keys.
def addParamsPrefix(prefix,params):
    ret = {}
    for key, value in params.items():
        ret[ prefix + "." + key] = value

    return ret

def main():
    global L1cachesz
    global L2cachesz
    global L3cachesz
    global L1assoc
    global L2assoc
    global L3assoc
    global L1Replacp
    global L2Replacp
    global L3Replacp
    global L2MSHR
    global L3MSHR
    global MSIMESI
    global Pref1
    global Pref2
    global Executable

    try:
        opts, args = getopt.getopt(sys.argv[1:], "", ["L1cachesz=","L2cachesz=","L3cachesz=","L1assoc=","L2assoc=","L3assoc=","L1Replacp=","L2Replacp=","L3Replacp=","L2MSHR=","L3MSHR=","MSIMESI=","Pref1=","Pref2=","Executable="])
    except getopt.GetopError as err:
        print (str(err))
        sys.exit(2)
    for o, a in opts:
        if o in ("--L1cachesz"):
            L1cachesz = a
            print ('found L1c')
        elif o in ("--L2cachesz"):
            L2cachesz = a
        elif o in ("--L3cachesz"):
            L3cachesz = a
        elif o in ("--L1assoc"):
            L1assoc = a
        elif o in ("--L2assoc"):
            L2assoc = a
        elif o in ("--L3assoc"):
            L3assoc = a
        elif o in ("--L1Replacp"):
            L1Replacp = a
        elif o in ("--L2Replacp"):
            L2Replacp = a
        elif o in ("--L3Replacp"):
            L3Replacp = a
        elif o in ("--L2MSHR"):
            L2MSHR = a
        elif o in ("--L3MSHR"):
            L3MSHR = a
        elif o in ("--MSIMESI"):
            MSIMESI = a
        elif o in ("--Pref1"):
            if a == "yes":
                Pref1 = "cassini.NextBlockPrefetcher"
        elif o in ("--Pref2"):
            if a == "yes":
                Pref2 = "cassini.NextBlockPrefetcher"
        elif o in ("--Executable"):
            Executable = a
        else:
            print (o)
            assert False, "Unknown Options"
    print (L1cachesz, L2cachesz, L3cachesz, L1assoc, L2assoc, L3assoc, L1Replacp, L2Replacp, L3Replacp, L2MSHR,L2MSHR,  MSIMESI, Pref1, Pref2, Executable)

main()
# Define needed environment params
os.environ['OMP_NUM_THREADS']="8"

# Define SST core options
sst.setProgramOption("timebase", "1 ps")
sst.setProgramOption("stop-at", "100ms")

# Define the simulation components
os_params = {
    "processDebugLevel" : 0,
    "dbgLevel" : 0,
    "dbgMask" : 8,
    "cores" : 1,
    "hardwareThreadCount" : 1,
    "page_size" : 4096,
    "physMemSize" : "1GiB",
    "useMMU" : True
}

node_os = sst.Component("core0_os", "vanadis.VanadisNodeOs")
node_os.addParams(os_params)

# process set up needs to be verified
process_list = (
    ( 1, {
        "env_count" : 1,
        "env0" : "OMP_NUM_THREADS=8",
        "exe" : Executable,
        "arg0" : Executable,
        "arg1" : "-ifeellucky"
    } ),
)

app_params = {"argc" : 2}
process_list[0][1].update(app_params)

num = 0
for i, process in process_list:
    for y in range(i):
        node_os.addParams(addParamsPrefix("process" + str(num), process))
        num+=1

node_os_mmu = node_os.setSubComponent("mmu", "mmu.SimpleMMU")
node_os_mmu.addParams(mmuParams = {
    "debug_level": 0,
    "num_cores": 8,
    "num_threads": 1,
    "page_size": 4096,
})

node_os_mem_if = node_os.setSubComponent("mem_interface", "memHierarchy.standardInterface")

os_cache = sst.Component("node_os.cache", "memHierarchy.Cache")
os_cache.addParams({
    "debug" : "0",
    "access_latency_cycles" : "1",
    "cache_frequency" : "2 Ghz",
    "replacement_policy" : L1Replacp,
    "coherence_protocol" : MSIMESI,
    "associativity" : L1assoc,
    "cache_line_size" : "64",
    "debug_level" : "6",
    "L1" : "1",
    "cache_size" : L1cachesz,
    "prefectcher" : Pref1
})

os_cache_2_cpu = os_cache.setSubComponent("cpulink", "memHierarchy.MemLink")
os_cache_2_mem = os_cache.setSubComponent("memlink", "memHierarchy.MemNIC")
os_cache_2_mem.addParams({
    "group" : 1,
    "network_bw" : "25GB/s"
})

# Build the CPUs for the Node
prefix="node0"
cpu_params = {
    "verbose" : 0,
    "clock" : "2 Ghz",
    "hardware_threads": 1,
    "physical_fp_registers" : 168,
    "physical_integer_registers" : 180,
    "integer_arith_cycles" : 2,
    "integer_arith_units" : 2,
    "fp_arith_cycles" : 2,
    "fp_arith_units" : 2,
    "branch_unit_cycles" : 2,
    "print_int_reg" : False,
    "print_fp_reg" : False,
    "reorder_slots" : 64,
    "decodes_per_cycle" : 4,
    "issues_per_cycle" :  4,
    "retires_per_cycle" : 4,
    "pause_when_retire_address" : 0,
    "start_verbose_when_issue_address": "0",
    "stop_verbose_when_retire_address": "0",
    "print_rob" : False
}

decoderParams = {
    "loader_mode" : "0",
    "uop_cache_entries" : 1536,
    "predecode_cache_entries" : 4
}

branchPredictorParams = {
    "branch_entries" : 32
}

lsqParams = {
    "verbose" : 0,
    "address_mask" : 0xFFFFFFFF,
    "max_stores" : 8,
    "max_loads" : 16
}

tlbWrapperParams = {
    "debug_level": 0,
}

tlbParams = {
    "debug_level": 0,
    "hitLatency": 1,
    "num_hardware_threads": 1,
    "num_tlb_entries_per_thread": 64,
    "tlb_set_size": 4,
}


cpu0 = sst.Component(prefix + ".cpu0", "vanadis.dbg_VanadisCPU")
cpu0.addParams(cpu_params)
cpu0.addParam("core_id", 0)

decoder0 = cpu0.setSubComponent("decoder0", "vanadis.Vanadis.RISCV64Decoder")
decoder0.addParams(decoderParams)

os_hdlr0 = decoder0.setSubComponent("os_handler0", "vanadis.Vanadis.RISCV64OSHandler")

branch_pred0 = decoder0.setSubComponent("branch_unit0", "vanadis.VanadisBasicBranchUnit")
branch_pred0.addParams(branchPredictorParams)

lsq0 = cpu0.setSubComponent("lsq0", "vanadis.VanadisBasicLoadStoreQueue")
lsq0.addParams(lsqParams)

cpu0_dcacheIf = lsq0.setSubcomponent("memory_interface0", "memHierarchy.standardInterface")
cpu0_icacheIf = cpu0.setSubComponent("mem_interface_inst0", "memHierarchy.standardInterface")

comp_c0_l1Dcache = sst.Component("c0.l1Dcache", "memHierarchy.Cache")
comp_c0_l1Dcache.addParams({
      "debug" : """0""",
      "access_latency_cycles" : """1""",
      "cache_frequency" : """2 Ghz""",
      "replacement_policy" : L1Replacp,
      "coherence_protocol" : MSIMESI,
      "associativity" : L1assoc,
      "cache_line_size" : """64""",
      "debug_level" : """6""",
      "L1" : """1""",
      "cache_size" : L1cachesz,
      "prefetcher" : Pref1
})

cpu0_l1Dcache_2_cpu = comp_c0_l1Dcache.setSubComponent("cpu0_l1dcache_cpulink", "memHierarchy.MemLink")
cpu0_liDcache_2_l2cache = comp_c0_l1Dcache.setSubComponent("cpu0_l1icache_memlink", "memHierarchy.MemLink")

cpu0_l1Icache = sst.Component("c0.l1Icache", "memHierarchy.Cache")
cpu0_l1Icache.addParams({
    "access_latency_cycles" : "2",
    "cache_frequency" : """2 Ghz""",
    "replacement_policy" : "lru",
    "coherence_protocol" : MSIMESI,
    "associativity" : "8",
    "cache_line_size" : "64",
    "cache_size" : "32 KB",
    "prefetcher" : "cassini.NextBlockPrefetcher",
    "prefetcher.reach" : 1,
    "L1" : "1",
    "debug" : """0""",
    "debug_level" :  """6""",
})

cpu0_l1Icache_2_cpu = cpu0_l1Icache.setSubComponent("cpu0_l1Icache_cpulink", "memHierarchy.MemLink")
cpu0_l1Icache_2_l2cache = cpu0_l1Icache.setSubComponent("cpu0_l1Icache_memlink", "memHierarcy.MemLink")

cpu0_dtlb_wrapper = sst.Component("cpu0_dtlb", "mmu.tlb_wrapper")
cpu0_dtlb_wrapper.addParams(tlbWrapperParams)

cpu0_dtlb = cpu0_dtlb_wrapper.setSubComponent("cpu0_dtlb", "mmu.simpleTLB")
cpu0_dtlb.addParams(tlbParams)

cpu0_itlb_wrapper = sst.Component("cpu0_itlb", "mmu.tlb_wrapper")
cpu0_itlb_wrapper.addParams(tlbWrapperParams)

cpu0_itlb = cpu0_itlb_wrapper.setSubComponent("cpu0_itlb", "mmu.simpleTLB")
cpu0_itlb.addParams(tlbParams)

cpu0_dtlb_link = sst.Link("cpu0_dtlb_link")
cpu0_dtlb_link.connect( (cpu0_dcacheIf, "port", "500ps"), (cpu0_dtlb_wrapper, "cpu_if", "500ps") )

cpu0_l1Dcache_link = sst.Link("cpu0_l1Dcache_link")
cpu0_l1Dcache_link.connect( (cpu0_dtlb_wrapper, "dcache_link_0", "500ps" ), (cpu0_l1Dcache_2_cpu, "highlink", "500ps") )

cpu0_itlb_link = sst.Link("cpu0_itlb_link")
cpu0_itlb_link.connect( (cpu0_icacheIf, "port", "500ps"), (cpu0_itlb_wrapper, "cpu_if", "500ps") )

cpu0_l1Icache_link = sst.Link("cpu_l1Icache_link")
cpu0_l1Icache_link.connect( (cpu0_itlb_wrapper, "icache_link_0", "500ps"), (cpu0_l1Icache_2_cpu, "highlink", "500ps") )

cpu1 = sst.Component(prefix + ".cpu1", "vanadis.dbg_VanadisCPU")
cpu1.addParams(cpu_params)
cpu1.addParam("core_id", 1)

decoder1 = cpu1.setSubComponent("decoder1", "vanadis.Vanadis.RISCV64Decoder")
decoder1.addParams(decoderParams)

os_hdlr1 = decoder1.setSubComponent("os_handler1", "vanadis.Vanadis.RISCV64OSHandler")

branch_pred1 = decoder1.setSubComponent("branch_unit1", "vanadis.VanadisBasicBranchUnit")
branch_pred1.addParams(branchPredictorParams)

lsq1 = cpu1.setSubComponent("lsq1", "vanadis.VanadisBasicLoadStoreQueue")
lsq1.addParams(lsqParams)

cpu1_dcacheIf = lsq1.setSubcomponent("memory_interface1", "memHierarchy.standardInterface")
cpu1_icacheIf = cpu1.setSubComponent("mem_interface_inst1", "memHierarchy.standardInterface")

comp_c1_l1Dcache = sst.Component("c1.l1Dcache", "memHierarchy.Cache")
comp_c1_l1Dcache.addParams({
      "debug" : """0""",
      "access_latency_cycles" : """10""",
      "cache_frequency" : """2 Ghz""",
      "replacement_policy" : L1Replacp,
      "coherence_protocol" : MSIMESI,
      "associativity" : L1assoc,
      "cache_line_size" : """64""",
      "debug_level" : """6""",
      "L1" : """1""",
      "cache_size" : L1cachesz,
      "prefetcher" : Pref1
})

cpu1_l1Dcache_2_cpu = comp_c1_l1Dcache.setSubComponent("cpu1_l1dcache_cpulink", "memHierarchy.MemLink")
cpu1_liDcache_2_l2cache = comp_c1_l1Dcache.setSubComponent("cpu1_l1icache_memlink", "memHierarchy.MemLink")

cpu1_l1Icache = sst.Component("c1.l1Icache", "memHierarchy.Cache")
cpu1_l1Icache.addParams({
    "access_latency_cycles" : "2",
    "cache_frequency" : """2 Ghz""",
    "replacement_policy" : "lru",
    "coherence_protocol" : MSIMESI,
    "associativity" : "8",
    "cache_line_size" : "64",
    "cache_size" : "32 KB",
    "prefetcher" : "cassini.NextBlockPrefetcher",
    "prefetcher.reach" : 1,
    "L1" : "1",
    "debug" : """0""",
    "debug_level" :  """6""",
})

cpu1_l1Icache_2_cpu = cpu1_l1Icache.setSubComponent("cpu1_l1Icache_cpulink", "memHierarchy.MemLink")
cpu1_l1Icache_2_l2cache = cpu1_l1Icache.setSubComponent("cpu1_l1Icache_memlink", "memHierarcy.MemLink")

cpu1_dtlb_wrapper = sst.Component("cpu1_dtlb", "mmu.tlb_wrapper")
cpu1_dtlb_wrapper.addParams(tlbWrapperParams)

cpu1_dtlb = cpu0_dtlb_wrapper.setSubComponent("cpu1_dtlb", "mmu.simpleTLB")
cpu1_dtlb.addParams(tlbParams)

cpu1_itlb_wrapper = sst.Component("cpu1_itlb", "mmu.tlb_wrapper")
cpu1_itlb_wrapper.addParams(tlbWrapperParams)

cpu1_itlb = cpu0_itlb_wrapper.setSubComponent("cpu1_itlb", "mmu.simpleTLB")
cpu1_itlb.addParams(tlbParams)

cpu1_dtlb_link = sst.Link("cpu1_dtlb_link")
cpu1_dtlb_link.connect( (cpu1_dcacheIf, "port", "500ps"), (cpu1_dtlb_wrapper, "cpu_if", "500ps") )

cpu1_l1Dcache_link = sst.Link("cpu1_l1Dcache_link")
cpu1_l1Dcache_link.connect( (cpu1_dtlb_wrapper, "dcache_link_1", "500ps" ), (cpu1_l1Dcache_2_cpu, "highlink", "500ps") )

cpu1_itlb_link = sst.Link("cpu1_itlb_link")
cpu1_itlb_link.connect( (cpu1_icacheIf, "port", "500ps"), (cpu1_itlb_wrapper, "cpu_if", "500ps") )

cpu1_l1Icache_link = sst.Link("cpu_l1Icache_link")
cpu1_l1Icache_link.connect( (cpu1_itlb_wrapper, "icache_link_1", "500ps"), (cpu1_l1Icache_2_cpu, "highlink", "500ps") )

cpu2 = sst.Component(prefix + ".cpu2", "vanadis.dbg_VanadisCPU")
cpu2.addParams(cpu_params)
cpu2.addParam("core_id", 2)

decoder2 = cpu2.setSubComponent("decoder2", "vanadis.Vanadis.RISCV64Decoder")
decoder2.addParams(decoderParams)

os_hdlr2 = decoder2.setSubComponent("os_handler2", "vanadis.Vanadis.RISCV64OSHandler")

branch_pred2 = decoder2.setSubComponent("branch_unit2", "vanadis.VanadisBasicBranchUnit")
branch_pred2.addParams(branchPredictorParams)

lsq2 = cpu2.setSubComponent("lsq2", "vanadis.VanadisBasicLoadStoreQueue")
lsq2.addParams(lsqParams)

cpu2_dcacheIf = lsq2.setSubcomponent("memory_interface2", "memHierarchy.standardInterface")
cpu2_icacheIf = cpu2.setSubComponent("mem_interface_inst12", "memHierarchy.standardInterface")

comp_c2_l1Dcache = sst.Component("c2.l1Dcache", "memHierarchy.Cache")
comp_c2_l1Dcache.addParams({
      "debug" : """0""",
      "access_latency_cycles" : """1""",
      "cache_frequency" : """2 Ghz""",
      "replacement_policy" : L1Replacp,
      "coherence_protocol" : MSIMESI,
      "associativity" : L1assoc,
      "cache_line_size" : """64""",
      "debug_level" : """6""",
      "L1" : """1""",
      "cache_size" : L1cachesz,
      "prefetcher" : Pref1
})

cpu2_l1Dcache_2_cpu2 = comp_c2_l1Dcache.setSubComponent("cpu2_l1dcache_cpulink", "memHierarchy.MemLink")
cpu2_liDcache_2_l2cache = comp_c2_l1Dcache.setSubComponent("cpu2_l1icache_memlink", "memHierarchy.MemLink")

cpu2_l1Icache = sst.Component("c2.l1Icache", "memHierarchy.Cache")
cpu2_l1Icache.addParams({
    "access_latency_cycles" : "2",
    "cache_frequency" : """2 Ghz""",
    "replacement_policy" : "lru",
    "coherence_protocol" : MSIMESI,
    "associativity" : "8",
    "cache_line_size" : "64",
    "cache_size" : "32 KB",
    "prefetcher" : "cassini.NextBlockPrefetcher",
    "prefetcher.reach" : 1,
    "L1" : "1",
    "debug" : """0""",
    "debug_level" :  """6""",
})

cpu2_l1Icache_2_cpu = cpu2_l1Icache.setSubComponent("cpu2_l1Icache_cpulink", "memHierarchy.MemLink")
cpu2_l1Icache_2_l2cache = cpu2_l1Icache.setSubComponent("cpu2_l1Icache_memlink", "memHierarcy.MemLink")

cpu2_dtlb_wrapper = sst.Component("cpu2_dtlb", "mmu.tlb_wrapper")
cpu2_dtlb_wrapper.addParams(tlbWrapperParams)

cpu2_dtlb = cpu2_dtlb_wrapper.setSubComponent("cpu2_dtlb", "mmu.simpleTLB")
cpu2_dtlb.addParams(tlbParams)

cpu2_itlb_wrapper = sst.Component("cpu2_itlb", "mmu.tlb_wrapper")
cpu2_itlb_wrapper.addParams(tlbWrapperParams)

cpu2_itlb = cpu2_itlb_wrapper.setSubComponent("cpu2_itlb", "mmu.simpleTLB")
cpu2_itlb.addParams(tlbParams)

cpu2_dtlb_link = sst.Link("cpu2_dtlb_link")
cpu2_dtlb_link.connect( (cpu2_dcacheIf, "port", "500ps"), (cpu2_dtlb_wrapper, "cpu_if", "500ps") )

cpu2_l1Dcache_link = sst.Link("cpu0_l1Dcache_link")
cpu2_l1Dcache_link.connect( (cpu2_dtlb_wrapper, "dcache_link_2", "500ps" ), (cpu2_l1Dcache_2_cpu, "highlink", "500ps") )

cpu2_itlb_link = sst.Link("cpu2_itlb_link")
cpu2_itlb_link.connect( (cpu2_icacheIf, "port", "500ps"), (cpu2_itlb_wrapper, "cpu_if", "500ps") )

cpu2_l1Icache_link = sst.Link("cpu2_l1Icache_link")
cpu2_l1Icache_link.connect( (cpu2_itlb_wrapper, "icache_link_2", "500ps"), (cpu2_l1Icache_2_cpu, "highlink", "500ps") )

cpu3 = sst.Component(prefix + ".cpu3", "vanadis.dbg_VanadisCPU")
cpu3.addParams(cpu_params)
cpu3.addParam("core_id", 3)

decoder3 = cpu3.setSubComponent("decoder3", "vanadis.Vanadis.RISCV64Decoder")
decoder3.addParams(decoderParams)

os_hdlr3 = decoder3.setSubComponent("os_handler3", "vanadis.Vanadis.RISCV64OSHandler")

branch_pred3 = decoder3.setSubComponent("branch_unit3", "vanadis.VanadisBasicBranchUnit")
branch_pred3.addParams(branchPredictorParams)

lsq3 = cpu3.setSubComponent("lsq3", "vanadis.VanadisBasicLoadStoreQueue")
lsq3.addParams(lsqParams)

cpu3_dcacheIf = lsq3.setSubcomponent("memory_interface3", "memHierarchy.standardInterface")
cpu3_icacheIf = cpu3.setSubComponent("mem_interface_inst3", "memHierarchy.standardInterface")

comp_c3_l1Dcache = sst.Component("c3.l1Dcache", "memHierarchy.Cache")
comp_c3_l1Dcache.addParams({
      "debug" : """0""",
      "access_latency_cycles" : """10""",
      "cache_frequency" : """2 Ghz""",
      "replacement_policy" : L1Replacp,
      "coherence_protocol" : MSIMESI,
      "associativity" : L1assoc,
      "cache_line_size" : """64""",
      "debug_level" : """6""",
      "L1" : """1""",
      "cache_size" : L1cachesz,
      "prefetcher" : Pref1
})

cpu3_l1Dcache_2_cpu = comp_c3_l1Dcache.setSubComponent("cpu3_l1dcache_cpulink", "memHierarchy.MemLink")
cpu3_liDcache_2_l2cache = comp_c3_l1Dcache.setSubComponent("cpu3_l1icache_memlink", "memHierarchy.MemLink")

cpu3_l1Icache = sst.Component("c3.l1Icache", "memHierarchy.Cache")
cpu3_l1Icache.addParams({
    "access_latency_cycles" : "2",
    "cache_frequency" : """2 Ghz""",
    "replacement_policy" : "lru",
    "coherence_protocol" : MSIMESI,
    "associativity" : "8",
    "cache_line_size" : "64",
    "cache_size" : "32 KB",
    "prefetcher" : "cassini.NextBlockPrefetcher",
    "prefetcher.reach" : 1,
    "L1" : "1",
    "debug" : """0""",
    "debug_level" :  """6""",
})

cpu3_l1Icache_2_cpu = cpu3_l1Icache.setSubComponent("cpu3_l1Icache_cpulink", "memHierarchy.MemLink")
cpu3_l1Icache_2_l2cache = cpu3_l1Icache.setSubComponent("cpu3_l1Icache_memlink", "memHierarcy.MemLink")

cpu3_dtlb_wrapper = sst.Component("cpu3_dtlb", "mmu.tlb_wrapper")
cpu3_dtlb_wrapper.addParams(tlbWrapperParams)

cpu3_dtlb = cpu3_dtlb_wrapper.setSubComponent("cpu3_dtlb", "mmu.simpleTLB")
cpu3_dtlb.addParams(tlbParams)

cpu3_itlb_wrapper = sst.Component("cpu3_itlb", "mmu.tlb_wrapper")
cpu3_itlb_wrapper.addParams(tlbWrapperParams)

cpu3_itlb = cpu3_itlb_wrapper.setSubComponent("cpu3_itlb", "mmu.simpleTLB")
cpu3_itlb.addParams(tlbParams)

cpu3_dtlb_link = sst.Link("cpu3_dtlb_link")
cpu3_dtlb_link.connect( (cpu3_dcacheIf, "port", "500ps"), (cpu3_dtlb_wrapper, "cpu_if", "500ps") )

cpu3_l1Dcache_link = sst.Link("cpu0_l1Dcache_link")
cpu3_l1Dcache_link.connect( (cpu3_dtlb_wrapper, "dcache_link_3", "500ps" ), (cpu3_l1Dcache_2_cpu, "highlink", "500ps") )

cpu3_itlb_link = sst.Link("cpu3_itlb_link")
cpu3_itlb_link.connect( (cpu3_icacheIf, "port", "500ps"), (cpu3_itlb_wrapper, "cpu_if", "500ps") )

cpu3_l1Icache_link = sst.Link("cpu3_l1Icache_link")
cpu3_l1Icache_link.connect( (cpu3_itlb_wrapper, "icache_link_3", "500ps"), (cpu3_l1Icache_2_cpu, "highlink", "500ps") )

cpu4 = sst.Component(prefix + ".cpu4", "vanadis.dbg_VanadisCPU")
cpu4.addParams(cpu_params)
cpu4.addParam("core_id", 4)

decoder4 = cpu4.setSubComponent("decoder4", "vanadis.Vanadis.RISCV64Decoder")
decoder4.addParams(decoderParams)

os_hdlr4 = decoder4.setSubComponent("os_handler4", "vanadis.Vanadis.RISCV64OSHandler")

branch_pred4 = decoder4.setSubComponent("branch_unit4", "vanadis.VanadisBasicBranchUnit")
branch_pred4.addParams(branchPredictorParams)

lsq4 = cpu4.setSubComponent("lsq4", "vanadis.VanadisBasicLoadStoreQueue")
lsq4.addParams(lsqParams)

cpu4_dcacheIf = lsq4.setSubcomponent("memory_interface4", "memHierarchy.standardInterface")
cpu4_icacheIf = cpu4.setSubComponent("mem_interface_inst4", "memHierarchy.standardInterface")

comp_c4_l1Dcache = sst.Component("c4.l1Dcache", "memHierarchy.Cache")
comp_c4_l1Dcache.addParams({
      "debug" : """0""",
      "access_latency_cycles" : """1""",
      "cache_frequency" : """2 Ghz""",
      "replacement_policy" : L1Replacp,
      "coherence_protocol" : MSIMESI,
      "associativity" : L1assoc,
      "cache_line_size" : """64""",
      "debug_level" : """6""",
      "L1" : """1""",
      "cache_size" : L1cachesz,
      "prefetcher" : Pref1
})

cpu4_l1Dcache_2_cpu = comp_c4_l1Dcache.setSubComponent("cpu4_l1dcache_cpulink", "memHierarchy.MemLink")
cpu4_liDcache_2_l2cache = comp_c4_l1Dcache.setSubComponent("cpu4_l1icache_memlink", "memHierarchy.MemLink")

cpu4_l1Icache = sst.Component("c4.l1Icache", "memHierarchy.Cache")
cpu4_l1Icache.addParams({
    "access_latency_cycles" : "2",
    "cache_frequency" : """2 Ghz""",
    "replacement_policy" : "lru",
    "coherence_protocol" : MSIMESI,
    "associativity" : "8",
    "cache_line_size" : "64",
    "cache_size" : "32 KB",
    "prefetcher" : "cassini.NextBlockPrefetcher",
    "prefetcher.reach" : 1,
    "L1" : "1",
    "debug" : """0""",
    "debug_level" :  """6""",
})

cpu4_l1Icache_2_cpu = cpu4_l1Icache.setSubComponent("cpu4_l1Icache_cpulink", "memHierarchy.MemLink")
cpu4_l1Icache_2_l2cache = cpu4_l1Icache.setSubComponent("cpu4_l1Icache_memlink", "memHierarcy.MemLink")

cpu4_dtlb_wrapper = sst.Component("cpu4_dtlb", "mmu.tlb_wrapper")
cpu4_dtlb_wrapper.addParams(tlbWrapperParams)

cpu4_dtlb = cpu4_dtlb_wrapper.setSubComponent("cpu4_dtlb", "mmu.simpleTLB")
cpu4_dtlb.addParams(tlbParams)

cpu4_itlb_wrapper = sst.Component("cpu4_itlb", "mmu.tlb_wrapper")
cpu4_itlb_wrapper.addParams(tlbWrapperParams)

cpu4_itlb = cpu4_itlb_wrapper.setSubComponent("cpu4_itlb", "mmu.simpleTLB")
cpu4_itlb.addParams(tlbParams)

cpu4_dtlb_link = sst.Link("cpu4_dtlb_link")
cpu4_dtlb_link.connect( (cpu4_dcacheIf, "port", "500ps"), (cpu4_dtlb_wrapper, "cpu_if", "500ps") )

cpu4_l1Dcache_link = sst.Link("cpu4_l1Dcache_link")
cpu4_l1Dcache_link.connect( (cpu4_dtlb_wrapper, "dcache_link_4", "500ps" ), (cpu4_l1Dcache_2_cpu, "highlink", "500ps") )

cpu4_itlb_link = sst.Link("cpu4_itlb_link")
cpu4_itlb_link.connect( (cpu4_icacheIf, "port", "500ps"), (cpu4_itlb_wrapper, "cpu_if", "500ps") )

cpu4_l1Icache_link = sst.Link("cpu4_l1Icache_link")
cpu4_l1Icache_link.connect( (cpu4_itlb_wrapper, "icache_link_4", "500ps"), (cpu4_l1Icache_2_cpu, "highlink", "500ps") )

cpu5 = sst.Component(prefix + ".cpu5", "vanadis.dbg_VanadisCPU")
cpu5.addParams(cpu_params)
cpu5.addParam("core_id", 5)

decoder5 = cpu5.setSubComponent("decoder5", "vanadis.Vanadis.RISCV64Decoder")
decoder5.addParams(decoderParams)

os_hdlr5 = decoder5.setSubComponent("os_handler5", "vanadis.Vanadis.RISCV64OSHandler")

branch_pred5 = decoder5.setSubComponent("branch_unit5", "vanadis.VanadisBasicBranchUnit")
branch_pred5.addParams(branchPredictorParams)

lsq5 = cpu5.setSubComponent("lsq5", "vanadis.VanadisBasicLoadStoreQueue")
lsq5.addParams(lsqParams)

cpu5_dcacheIf = lsq5.setSubcomponent("memory_interface5", "memHierarchy.standardInterface")
cpu5_icacheIf = cpu5.setSubComponent("mem_interface_inst5", "memHierarchy.standardInterface")

comp_c5_l1Dcache = sst.Component("c5.l1Dcache", "memHierarchy.Cache")
comp_c5_l1Dcache.addParams({
      "debug" : """0""",
      "access_latency_cycles" : """10""",
      "cache_frequency" : """2 Ghz""",
      "replacement_policy" : L1Replacp,
      "coherence_protocol" : MSIMESI,
      "associativity" : L1assoc,
      "cache_line_size" : """64""",
      "debug_level" : """6""",
      "L1" : """1""",
      "cache_size" : L1cachesz,
      "prefetcher" : Pref1
})

cpu5_l1Dcache_2_cpu = comp_c5_l1Dcache.setSubComponent("cpu5_l1dcache_cpulink", "memHierarchy.MemLink")
cpu5_liDcache_2_l2cache = comp_c5_l1Dcache.setSubComponent("cpu5_l1icache_memlink", "memHierarchy.MemLink")

cpu5_l1Icache = sst.Component("c5.l1Icache", "memHierarchy.Cache")
cpu5_l1Icache.addParams({
    "access_latency_cycles" : "2",
    "cache_frequency" : """2 Ghz""",
    "replacement_policy" : "lru",
    "coherence_protocol" : MSIMESI,
    "associativity" : "8",
    "cache_line_size" : "64",
    "cache_size" : "32 KB",
    "prefetcher" : "cassini.NextBlockPrefetcher",
    "prefetcher.reach" : 1,
    "L1" : "1",
    "debug" : """0""",
    "debug_level" :  """6""",
})

cpu5_l1Icache_2_cpu = cpu5_l1Icache.setSubComponent("cpu5_l1Icache_cpulink", "memHierarchy.MemLink")
cpu5_l1Icache_2_l2cache = cpu5_l1Icache.setSubCompnent("cpu5_l1Icache_memlink", "memHierarcy.MemLink")

cpu5_dtlb_wrapper = sst.Component("cpu5_dtlb", "mmu.tlb_wrapper")
cpu5_dtlb_wrapper.addParams(tlbWrapperParams)

cpu5_dtlb = cpu0_dtlb_wrapper.setSubComponent("cpu5_dtlb", "mmu.simpleTLB")
cpu5_dtlb.addParams(tlbParams)

cpu5_itlb_wrapper = sst.Component("cpu5_itlb", "mmu.tlb_wrapper")
cpu5_itlb_wrapper.addParams(tlbWrapperParams)

cpu5_itlb = cpu5_itlb_wrapper.setSubComponent("cpu5_itlb", "mmu.simpleTLB")
cpu5_itlb.addParams(tlbParams)

cpu5_dtlb_link = sst.Link("cpu5_dtlb_link")
cpu5_dtlb_link.connect( (cpu5_dcacheIf, "port", "500ps"), (cpu5_dtlb_wrapper, "cpu_if", "500ps") )

cpu5_l1Dcache_link = sst.Link("cpu5_l1Dcache_link")
cpu5_l1Dcache_link.connect( (cpu5_dtlb_wrapper, "dcache_link_5", "500ps" ), (cpu5_l1Dcache_2_cpu, "highlink", "500ps") )

cpu5_itlb_link = sst.Link("cpu5_itlb_link")
cpu5_itlb_link.connect( (cpu5_icacheIf, "port", "500ps"), (cpu5_itlb_wrapper, "cpu_if", "500ps") )

cpu5_l1Icache_link = sst.Link("cpu5_l1Icache_link")
cpu5_l1Icache_link.connect( (cpu5_itlb_wrapper, "icache_link_5", "500ps"), (cpu5_l1Icache_2_cpu, "highlink", "500ps") )

cpu6 = sst.Component(prefix + ".cpu6", "vanadis.dbg_VanadisCPU")
cpu6.addParams(cpu_params)
cpu6.addParam("core_id", 6)

decoder6 = cpu6.setSubComponent("decoder6", "vanadis.Vanadis.RISCV64Decoder")
decoder6.addParams(decoderParams)

os_hdlr6 = decoder6.setSubComponent("os_handler6", "vanadis.Vanadis.RISCV64OSHandler")

branch_pred6 = decoder6.setSubComponent("branch_unit6", "vanadis.VanadisBasicBranchUnit")
branch_pred6.addParams(branchPredictorParams)

lsq6 = cpu6.setSubComponent("lsq6", "vanadis.VanadisBasicLoadStoreQueue")
lsq6.addParams(lsqParams)

cpu6_dcacheIf = lsq6.setSubcomponent("memory_interface6", "memHierarchy.standardInterface")
cpu6_icacheIf = cpu6.setSubComponent("mem_interface_inst6", "memHierarchy.standardInterface")

comp_c6_l1Dcache = sst.Component("c6.l1Dcache", "memHierarchy.Cache")
comp_c6_l1Dcache.addParams({
      "debug" : """0""",
      "access_latency_cycles" : """1""",
      "cache_frequency" : """2 Ghz""",
      "replacement_policy" : L1Replacp,
      "coherence_protocol" : MSIMESI,
      "associativity" : L1assoc,
      "cache_line_size" : """64""",
      "debug_level" : """6""",
      "L1" : """1""",
      "cache_size" : L1cachesz,
      "prefetcher" : Pref1
})
cpu6_l1Dcache_2_cpu = comp_c6_l1Dcache.setSubComponent("cpu6_l1dcache_cpulink", "memHierarchy.MemLink")
cpu6_liDcache_2_l2cache = comp_c6_l1Dcache.setSubComponent("cpu6_l1icache_memlink", "memHierarchy.MemLink")

cpu6_l1Icache = sst.Component("c6.l1Icache", "memHierarchy.Cache")
cpu6_l1Icache.addParams({
    "access_latency_cycles" : "2",
    "cache_frequency" : """2 Ghz""",
    "replacement_policy" : "lru",
    "coherence_protocol" : MSIMESI,
    "associativity" : "8",
    "cache_line_size" : "64",
    "cache_size" : "32 KB",
    "prefetcher" : "cassini.NextBlockPrefetcher",
    "prefetcher.reach" : 1,
    "L1" : "1",
    "debug" : """0""",
    "debug_level" :  """6""",
})

cpu6_l1Icache_2_cpu = cpu6_l1Icache.setSubComponent("cpu6_l1Icache_cpulink", "memHierarchy.MemLink")
cpu6_l1Icache_2_l2cache = cpu6_l1Icache.setSubCompnent("cpu6_l1Icache_memlink", "memHierarcy.MemLink")

cpu6_dtlb_wrapper = sst.Component("cpu6_dtlb", "mmu.tlb_wrapper")
cpu6_dtlb_wrapper.addParams(tlbWrapperParams)

cpu6_dtlb = cpu0_dtlb_wrapper.setSubComponent("cpu6_dtlb", "mmu.simpleTLB")
cpu6_dtlb.addParams(tlbParams)

cpu6_itlb_wrapper = sst.Component("cpu6_itlb", "mmu.tlb_wrapper")
cpu6_itlb_wrapper.addParams(tlbWrapperParams)

cpu6_itlb = cpu6_itlb_wrapper.setSubComponent("cpu6_itlb", "mmu.simpleTLB")
cpu6_itlb.addParams(tlbParams)

cpu6_dtlb_link = sst.Link("cpu0_dtlb_link")
cpu6_dtlb_link.connect( (cpu0_dcacheIf, "port", "500ps"), (cpu0_dtlb_wrapper, "cpu_if", "500ps") )

cpu6_l1Dcache_link = sst.Link("cpu6_l1Dcache_link")
cpu6_l1Dcache_link.connect( (cpu6_dtlb_wrapper, "dcache_link_6", "500ps" ), (cpu6_l1Dcache_2_cpu, "highlink", "500ps") )

cpu6_itlb_link = sst.Link("cpu6_itlb_link")
cpu6_itlb_link.connect( (cpu6_icacheIf, "port", "500ps"), (cpu6_itlb_wrapper, "cpu_if", "500ps") )

cpu6_l1Icache_link = sst.Link("cpu6_l1Icache_link")
cpu6_l1Icache_link.connect( (cpu6_itlb_wrapper, "icache_link_6", "500ps"), (cpu6_l1Icache_2_cpu, "highlink", "500ps") )

cpu7 = sst.Component(prefix + ".cpu7", "vanadis.dbg_VanadisCPU")
cpu7.addParams(cpu_params)
cpu7.addParam("core_id", 7)

decoder7 = cpu7.setSubComponent("decoder7", "vanadis.Vanadis.RISCV64Decoder")
decoder7.addParams(decoderParams)

os_hdlr7 = decoder7.setSubComponent("os_handler7", "vanadis.Vanadis.RISCV64OSHandler")

branch_pred7 = decoder7.setSubComponent("branch_unit7", "vanadis.VanadisBasicBranchUnit")
branch_pred7.addParams(branchPredictorParams)

lsq7 = cpu7.setSubComponent("lsq7", "vanadis.VanadisBasicLoadStoreQueue")
lsq7.addParams(lsqParams)

cpu7_dcacheIf = lsq7.setSubcomponent("memory_interface7", "memHierarchy.standardInterface")
cpu7_icacheIf = cpu7.setSubComponent("mem_interface_inst7", "memHierarchy.standardInterface")

comp_c7_l1Dcache = sst.Component("c7.l1Dcache", "memHierarchy.Cache")
comp_c7_l1Dcache.addParams({
      "debug" : """0""",
      "access_latency_cycles" : """10""",
      "cache_frequency" : """2 Ghz""",
      "replacement_policy" : L1Replacp,
      "coherence_protocol" : MSIMESI,
      "associativity" : L1assoc,
      "cache_line_size" : """64""",
      "debug_level" : """6""",
      "L1" : """1""",
      "cache_size" : L1cachesz,
      "prefetcher" : Pref1
})

cpu7_l1Dcache_2_cpu7 = comp_c7_l1Dcache.setSubComponent("cpu7_l1dcache_cpulink", "memHierarchy.MemLink")
cpu7_liDcache_2_l2cache = comp_c7_l1Dcache.setSubComponent("cpu7_l1icache_memlink", "memHierarchy.MemLink")

cpu7_l1Icache = sst.Component("c7.l1Icache", "memHierarchy.Cache")
cpu7_l1Icache.addParams({
    "access_latency_cycles" : "2",
    "cache_frequency" : """2 Ghz""",
    "replacement_policy" : "lru",
    "coherence_protocol" : MSIMESI,
    "associativity" : "8",
    "cache_line_size" : "64",
    "cache_size" : "32 KB",
    "prefetcher" : "cassini.NextBlockPrefetcher",
    "prefetcher.reach" : 1,
    "L1" : "1",
    "debug" : """0""",
    "debug_level" :  """6""",
})

cpu7_l1Icache_2_cpu = cpu7_l1Icache.setSubComponent("cpu7_l1Icache_cpulink", "memHierarchy.MemLink")
cpu7_l1Icache_2_l2cache = cpu7_l1Icache.setSubCompnent("cpu7_l1Icache_memlink", "memHierarcy.MemLink")

cpu7_dtlb_wrapper = sst.Component("cpu7_dtlb", "mmu.tlb_wrapper")
cpu7_dtlb_wrapper.addParams(tlbWrapperParams)

cpu7_dtlb = cpu7_dtlb_wrapper.setSubComponent("cpu7_dtlb", "mmu.simpleTLB")
cpu7_dtlb.addParams(tlbParams)

cpu7_itlb_wrapper = sst.Component("cpu7_itlb", "mmu.tlb_wrapper")
cpu7_itlb_wrapper.addParams(tlbWrapperParams)

cpu7_itlb = cpu7_itlb_wrapper.setSubComponent("cpu7_itlb", "mmu.simpleTLB")
cpu7_itlb.addParams(tlbParams)

cpu7_dtlb_link = sst.Link("cpu7_dtlb_link")
cpu7_dtlb_link.connect( (cpu7_dcacheIf, "port", "500ps"), (cpu7_dtlb_wrapper, "cpu_if", "500ps") )

cpu7_l1Dcache_link = sst.Link("cpu7_l1Dcache_link")
cpu7_l1Dcache_link.connect( (cpu7_dtlb_wrapper, "dcache_link_7", "500ps" ), (cpu7_l1Dcache_2_cpu, "highlink", "500ps") )

cpu7_itlb_link = sst.Link("cpu7_itlb_link")
cpu7_itlb_link.connect( (cpu7_icacheIf, "port", "500ps"), (cpu7_itlb_wrapper, "cpu_if", "500ps") )

cpu7_l1Icache_link = sst.Link("cpu7_l1Icache_link")
cpu7_l1Icache_link.connect( (cpu7_itlb_wrapper, "icache_link_7", "500ps"), (cpu7_l1Icache_2_cpu, "highlink", "500ps") )

comp_n0_bus = sst.Component("n0.bus", "memHierarchy.Bus")
comp_n0_bus.addParams({
      "bus_frequency" : """2 Ghz"""
})
comp_n0_l2cache = sst.Component("n0.l2cache", "memHierarchy.Cache")
comp_n0_l2cache.addParams({
      "debug" : """0""",
      "access_latency_cycles" : """15""",
      "cache_frequency" : """2.0 Ghz""",
      "replacement_policy" : L2Replacp,
      "coherence_protocol" : MSIMESI,
      "associativity" : L2assoc,
      "cache_line_size" : """64""",
      "debug_level" : """6""",
      "L1" : """0""",
      "cache_size" : L2cachesz,
      "mshr_num_entries" : L2MSHR,
      "prefetcher" : Pref2,
})

highlink_n0_l2cache = comp_n0_l2cache.setSubComponent("highlink", "memHierarchy.MemLink")
lowlink_n0_l2cache = comp_n0_l2cache.setSubComponent("lowlink", "memHierarchy.MemNIC")
lowlink_n0_l2cache.addParams({
    "group" : 0, 
    "network_bw" : """25GB/s""",
    "network_input_buffer_size" : "2KB",
    "network_output_buffer_size" : "2KB"
})

comp_n1_bus = sst.Component("n1.bus", "memHierarchy.Bus")
comp_n1_bus.addParams({
      "bus_frequency" : """2 Ghz"""
})
comp_n1_l2cache = sst.Component("n1.l2cache", "memHierarchy.Cache")
comp_n1_l2cache.addParams({
      "debug" : """0""",
      "access_latency_cycles" : """5""",
      "cache_frequency" : """2.0 Ghz""",
      "replacement_policy" : L2Replacp,
      "coherence_protocol" : MSIMESI,
      "associativity" : L2assoc,
      "cache_line_size" : """64""",
      "debug_level" : """6""",
      "L1" : """0""",
      "cache_size" : L2cachesz,
      "mshr_num_entries" : L2MSHR,
      "prefetcher" : Pref2,
})
highlink_n1_l2cache = comp_n1_l2cache.setSubComponent("highlink", "memHierarchy.MemLink")
lowlink_n1_l2cache = comp_n1_l2cache.setSubComponent("lowlink", "memHierarchy.MemNIC")
lowlink_n1_l2cache.addParams({
    "group" : 0,
    "network_bw" : """25GB/s""",
    "network_input_buffer_size" : "2KB",
    "network_output_buffer_size" : "2KB"
})
comp_l3cache0 = sst.Component("l3cache0", "memHierarchy.Cache")
comp_l3cache0.addParams({
      "debug" : """0""",
      "access_latency_cycles" : """8""",
      "cache_frequency" : """2.0 Ghz""",
      "replacement_policy" : L3Replacp,
      "coherence_protocol" : MSIMESI,
      "associativity" : L3assoc,
      "cache_line_size" : """64""",
      "debug_level" : """6""",
      "L1" : """0""",
      "cache_size" : L3cachesz,
      "mshr_num_entries" : L3MSHR,
      "num_cache_slices" : """2""",
      "slice_id" : """0""",
      "slice_allocation_policy" : """rr"""
})
highlink_l3cache0 = comp_l3cache0.setSubComponent("highlink", "memHierarchy.MemNIC")
highlink_l3cache0.addParams({
    "group" : 1,
    "network_bw" : """25GB/s""",
    "network_input_buffer_size" : "2KB",
    "network_output_buffer_size" : "2KB"
})
comp_l3cache1 = sst.Component("l3cache1", "memHierarchy.Cache")
comp_l3cache1.addParams({
      "debug" : """0""",
      "access_latency_cycles" : """8""",
      "cache_frequency" : """2.0 Ghz""",
      "replacement_policy" : L3Replacp,
      "coherence_protocol" : MSIMESI,
      "associativity" : L3assoc,
      "cache_line_size" : """64""",
      "debug_level" : """6""",
      "cache_size" : L3cachesz,
      "mshr_num_entries" : L3MSHR,
      "num_cache_slices" : """2""",
      "slice_id" : """1""",
      "slice_allocation_policy" : """rr"""
})
highlink_l3cache1 = comp_l3cache1.setSubComponent("highlink", "memHierarchy.MemNIC")
highlink_l3cache1.addParams({
    "group" : 1,
    "network_bw" : """25GB/s""",
    "network_input_buffer_size" : "2KB",
    "network_output_buffer_size" : "2KB"
})
comp_chipRtr = sst.Component("chipRtr", "merlin.hr_router")
comp_chipRtr.addParams({
      "input_buf_size" : """2KB""",
      "num_ports" : """6""",
      "id" : """0""",
      "output_buf_size" : """2KB""",
      "flit_size" : """64B""",
      "xbar_bw" : """51.2 GB/s""",
      "link_bw" : """25.6 GB/s""",
})
comp_chipRtr.setSubComponent("topology", "merlin.singlerouter")
comp_dirctrl0 = sst.Component("dirctrl0", "memHierarchy.DirectoryController")
comp_dirctrl0.addParams({
      "debug" : """0""",
      "coherence_protocol" : MSIMESI,
      "entry_cache_size" : """32768""",
      "addr_range_start" : """0x0""",
      "addr_range_end" : """0x000FFFFF""",
      "mshr_num_entries" : "2",
})
highlink_dirctrl0 = comp_dirctrl0.setSubComponent("highlink", "memHierarchy.MemNIC")
lowlink_dirctrl0 = comp_dirctrl0.setSubComponent("lowlink", "memHierarchy.MemLink")
highlink_dirctrl0.addParams({
    "group" : 2,
    "network_bw" : """25GB/s""",
    "network_input_buffer_size" : "2KB",
    "network_output_buffer_size" : "2KB",
})
comp_memctrl0 = sst.Component("memory0", "memHierarchy.MemController")
comp_memctrl0.addParams({
      "debug" : """0""",
      "clock" : """1.6GHz""",
})
comp_memory0 = comp_memctrl0.setSubComponent("backend", "memHierarchy.simpleMem")
comp_memory0.addParams({
      "mem_size" : "512MiB",
      "access_time" : "5 ns"
})
comp_dirctrl1 = sst.Component("dirctrl1", "memHierarchy.DirectoryController")
comp_dirctrl1.addParams({
      "debug" : """0""",
      "coherence_protocol" : MSIMESI,
      "entry_cache_size" : """32768""",
      "addr_range_start" : """0x00100000""",
      "addr_range_end" : """0x3FFFFFFF""",
      "mshr_num_entries" : "2",
})
highlink_dirctrl1 = comp_dirctrl1.setSubComponent("highlink", "memHierarchy.MemNIC")
lowlink_dirctrl1 = comp_dirctrl1.setSubComponent("lowlink", "memHierarchy.MemLink")
highlink_dirctrl1.addParams({
    "group" : 2,
    "network_bw" : """25GB/s""",
    "network_input_buffer_size" : "2KB",
    "network_output_buffer_size" : "2KB",
})
comp_memctrl1 = sst.Component("memory1", "memHierarchy.MemController")
comp_memctrl1.addParams({
      "debug" : """0""",
      "clock" : """1.6GHz""",
})
comp_memory1 = comp_memctrl1.setSubComponent("backend", "memHierarchy.simpleMem")
comp_memory1.addParams({
      "mem_size" : "512MiB",
      "access_time" : "5 ns"
})

# Define the simulation links
link_c0dcache_bus_link = sst.Link("link_c0dcache_bus_link")
link_c0dcache_bus_link.connect( (comp_c0_l1Dcache, "lowlink", "100ps"), (comp_n0_bus, "highlink0", "100ps") )
link_c1dcache_bus_link = sst.Link("link_c1dcache_bus_link")
link_c1dcache_bus_link.connect( (comp_c1_l1Dcache, "lowlink", "100ps"), (comp_n0_bus, "highlink1", "100ps") )
link_c2dcache_bus_link = sst.Link("link_c2dcache_bus_link")
link_c2dcache_bus_link.connect( (comp_c2_l1Dcache, "lowlink", "100ps"), (comp_n0_bus, "highlink2", "100ps") )
link_c3dcache_bus_link = sst.Link("link_c3dcache_bus_link")
link_c3dcache_bus_link.connect( (comp_c3_l1Dcache, "lowlink", "100ps"), (comp_n0_bus, "highlink3", "100ps") )
link_c4dcache_bus_link = sst.Link("link_c4dcache_bus_link")
link_c4dcache_bus_link.connect( (comp_c4_l1Dcache, "lowlink", "100ps"), (comp_n1_bus, "highlink0", "100ps") )
link_c5dcache_bus_link = sst.Link("link_c5dcache_bus_link")
link_c5dcache_bus_link.connect( (comp_c5_l1Dcache, "lowlink", "100ps"), (comp_n1_bus, "highlink1", "100ps") )
link_c6dcache_bus_link = sst.Link("link_c6dcache_bus_link")
link_c6dcache_bus_link.connect( (comp_c6_l1Dcache, "lowlink", "100ps"), (comp_n1_bus, "highlink2", "100ps") )
link_c7dcache_bus_link = sst.Link("link_c7dcache_bus_link")
link_c7dcache_bus_link.connect( (comp_c7_l1Dcache, "lowlink", "100ps"), (comp_n1_bus, "highlink3", "100ps") )
link_n0bus_n0l2cache = sst.Link("link_n0bus_n0l2cache")
link_n0bus_n0l2cache.connect( (comp_n0_bus, "lowlink0", "100ps"), (highlink_n0_l2cache, "port", "100ps") )
link_n0bus_router = sst.Link("link_n0bus_router")
link_n0bus_router.connect( (lowlink_n0_l2cache, "port", "100ps"), (comp_chipRtr, "port2", "1000ps") )
link_n1bus_n1l2cache = sst.Link("link_n1bus_n1l2cache")
link_n1bus_n1l2cache.connect( (comp_n1_bus, "lowlink0", "100ps"), (highlink_n1_l2cache, "port", "100ps") )
link_n1bus_router = sst.Link("link_n1bus_router")
link_n1bus_router.connect( (lowlink_n1_l2cache, "port", "100ps"), (comp_chipRtr, "port3", "1000ps") )
link_l3cache0_router = sst.Link("link_l3cache0_router")
link_l3cache0_router.connect( (comp_chipRtr, "port4", "1000ps"), (highlink_l3cache0, "port", "100ps") );
link_l3cache1_router = sst.Link("link_l3cache1_router")
link_l3cache1_router.connect( (comp_chipRtr, "port5", "1000ps"), (highlink_l3cache1, "port", "100ps") );
link_dirctrl0_router = sst.Link("link_dirctrl0_router")
link_dirctrl0_router.connect( (comp_chipRtr, "port0", "1000ps"), (highlink_dirctrl0, "port", "100ps") )
link_dirctrl1_router = sst.Link("link_dirctrl1_router")
link_dirctrl1_router.connect( (comp_chipRtr, "port1", "1000ps"), (highlink_dirctrl1, "port", "100ps") )
link_dirctrl0_mem = sst.Link("link_dirctrl0_mem")
link_dirctrl0_mem.connect( (lowlink_dirctrl0, "port", "100ps"), (comp_memctrl0, "highlink", "100ps") )
link_dirctrl1_mem = sst.Link("link_dirctrl1_mem")
link_dirctrl1_mem.connect( (lowlink_dirctrl1, "port", "100ps"), (comp_memctrl1, "highlink", "100ps") )
# End of generated output.
