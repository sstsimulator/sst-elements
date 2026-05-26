"""
Shared SST topology builders for Balar and Doorbell-pattern contract tests.
"""

import sst

DEBUG_PARAMS = {"debug": 0, "debug_level": 10}
NETWORK_BW = "25GB/s"
CLOCK = "2GHz"

CORE_GROUP = 0
MMIO_GROUP = 1
MEMORY_GROUP = 2

CORE_DST = [MEMORY_GROUP, MMIO_GROUP]
MMIO_SRC = [CORE_GROUP]
MMIO_DST = [MEMORY_GROUP]
MEMORY_SRC = [CORE_GROUP, MMIO_GROUP]


def build_testcpu_router(balar_builder, cfg_file, balar_verbosity=0, dma_verbosity=0):
    """Return (cpu, balar_mmio_addr) for merlin router + BalarTestCPU."""
    balar_mmio_iface, balar_mmio_addr, dma_mem_if, dma_mmio_if = balar_builder.buildTestCPU(
        cfg_file, balar_verbosity, dma_verbosity
    )

    mmio_nic = balar_mmio_iface.setSubComponent("lowlink", "memHierarchy.MemNIC")
    mmio_nic.addParams({
        "group": MMIO_GROUP,
        "sources": MMIO_SRC,
        "destinations": MMIO_DST,
        "network_bw": NETWORK_BW,
    })

    dma_mem_nic = dma_mem_if.setSubComponent("lowlink", "memHierarchy.MemNIC")
    dma_mem_nic.addParams({
        "group": CORE_GROUP,
        "destinations": CORE_DST,
        "network_bw": NETWORK_BW,
    })

    dma_mmio_nic = dma_mmio_if.setSubComponent("lowlink", "memHierarchy.MemNIC")
    dma_mmio_nic.addParams({
        "group": MEMORY_GROUP,
        "sources": MEMORY_SRC,
        "network_bw": NETWORK_BW,
    })

    cpu = sst.Component("cpu", "balar.BalarTestCPU")
    cpu.addParams({
        "clock": CLOCK,
        "verbose": balar_verbosity,
        "scratch_mem_addr": 0,
        "gpu_addr": balar_mmio_addr,
        "enable_memcpy_dump": False,
    })
    iface = cpu.setSubComponent("memory", "memHierarchy.standardInterface")
    iface.addParams(DEBUG_PARAMS)
    cpu_nic = iface.setSubComponent("lowlink", "memHierarchy.MemNIC")
    cpu_nic.addParams({
        "group": CORE_GROUP,
        "destinations": CORE_DST,
        "network_bw": NETWORK_BW,
    })

    chiprtr = sst.Component("chiprtr", "merlin.hr_router")
    chiprtr.addParams({
        "xbar_bw": "1GB/s",
        "id": "0",
        "input_buf_size": "1KB",
        "num_ports": "5",
        "flit_size": "72B",
        "output_buf_size": "1KB",
        "link_bw": "1GB/s",
        "topology": "merlin.singlerouter",
    })
    chiprtr.setSubComponent("topology", "merlin.singlerouter")

    memctrl = sst.Component("memory", "memHierarchy.MemController")
    memctrl.addParams({
        "clock": "1GHz",
        "addr_range_end": balar_mmio_addr - 1,
    })
    mem_nic = memctrl.setSubComponent("highlink", "memHierarchy.MemNIC")
    mem_nic.addParams({
        "group": MEMORY_GROUP,
        "sources": MEMORY_SRC,
        "network_bw": NETWORK_BW,
    })
    memory = memctrl.setSubComponent("backend", "memHierarchy.simpleMem")
    memory.addParams({
        "access_time": "100 ns",
        "mem_size": "512MiB",
    })

    sst.Link("link_cpu").connect((cpu_nic, "port", "1000ps"), (chiprtr, "port0", "1000ps"))
    sst.Link("link_mmio").connect((mmio_nic, "port", "500ps"), (chiprtr, "port1", "500ps"))
    sst.Link("link_dma_mem").connect((dma_mem_nic, "port", "500ps"), (chiprtr, "port2", "500ps"))
    sst.Link("link_dma_mmio").connect((dma_mmio_nic, "port", "500ps"), (chiprtr, "port3", "500ps"))
    sst.Link("link_mem").connect((mem_nic, "port", "1000ps"), (chiprtr, "port4", "1000ps"))

    return cpu, balar_mmio_addr


def build_doorbell_topology(balar_builder, cfg_file, mode="trace", balar_verbosity=0, dma_verbosity=0):
    """
    Doorbell-pattern: DoorbellTestCPU cache_link -> L1 -> coherent memory fabric,
    and mmio_link -> routed MMIO fabric for balarMMIO doorbells.
    """
    # NOTE: hard-coded high MMIO aperture so wide scratch packets cannot overflow
    # into the MMIO region. Could derive from max packet size + alignment instead.
    balar_mmio_addr = 0x10000000
    dma_mmio_addr = balar_mmio_addr + 1024
    _, balar_mmio_iface = balar_builder.build(
        cfg_file, balar_mmio_addr, dma_mmio_addr, verbosity=balar_verbosity
    )

    dma = sst.Component("dmaEngine", "balar.dmaEngine")
    dma.addParams({
        "verbose": dma_verbosity,
        "clock": CLOCK,
        "mmio_addr": dma_mmio_addr,
        "mmio_size": 512,
    })
    dma_mmio_if = dma.setSubComponent("mmio_iface", "memHierarchy.standardInterface")
    dma_mem_if = dma.setSubComponent("mem_iface", "memHierarchy.standardInterface")

    l1 = sst.Component("l1", "memHierarchy.Cache")
    l1.addParams({
        "cache_frequency": CLOCK,
        "cache_size": "32KB",
        "associativity": 4,
        "access_latency_cycles": 2,
        "L1": 1,
        "coherence_protocol": "mesi",
        "replacement_policy": "lru",
        "cache_line_size": 64,
        "addr_range_start": 0,
        "addr_range_end": balar_mmio_addr - 1,
    })
    l1_cpu = l1.setSubComponent("highlink", "memHierarchy.MemLink")
    l1_nic = l1.setSubComponent("lowlink", "memHierarchy.MemNIC")
    l1_nic.addParams({
        "group": CORE_GROUP,
        "destinations": CORE_DST,
        "network_bw": NETWORK_BW,
    })

    cpu = sst.Component("cpu", "balar.DoorbellTestCPU")
    cpu.addParams({
        "clock": CLOCK,
        "verbose": balar_verbosity,
        "scratch_mem_addr": 0,
        "mmio_addr": balar_mmio_addr,
        "cache_line_size": 64,
        "mode": mode,
        "enable_memcpy_dump": False,
    })
    cache_if = cpu.setSubComponent("cache_link", "memHierarchy.standardInterface")
    cache_if.addParams(DEBUG_PARAMS)
    mmio_if = cpu.setSubComponent("mmio_link", "memHierarchy.standardInterface")
    mmio_if.addParams(DEBUG_PARAMS)
    cpu_mmio_nic = mmio_if.setSubComponent("lowlink", "memHierarchy.MemNIC")
    cpu_mmio_nic.addParams({
        "group": CORE_GROUP,
        "destinations": CORE_DST,
        "network_bw": NETWORK_BW,
    })

    balar_mmio_nic = balar_mmio_iface.setSubComponent("lowlink", "memHierarchy.MemNIC")
    balar_mmio_nic.addParams({
        "group": MMIO_GROUP,
        "sources": MMIO_SRC,
        "destinations": MMIO_DST,
        "network_bw": NETWORK_BW,
    })

    dma_mem_nic = dma_mem_if.setSubComponent("lowlink", "memHierarchy.MemNIC")
    dma_mem_nic.addParams({
        "group": CORE_GROUP,
        "destinations": CORE_DST,
        "network_bw": NETWORK_BW,
    })

    dma_mmio_nic = dma_mmio_if.setSubComponent("lowlink", "memHierarchy.MemNIC")
    dma_mmio_nic.addParams({
        "group": MEMORY_GROUP,
        "sources": MEMORY_SRC,
        "network_bw": NETWORK_BW,
    })

    chiprtr = sst.Component("doorbell_chiprtr", "merlin.hr_router")
    chiprtr.addParams({
        "xbar_bw": "1GB/s",
        "id": "0",
        "input_buf_size": "1KB",
        "num_ports": "6",
        "flit_size": "72B",
        "output_buf_size": "1KB",
        "link_bw": "1GB/s",
        "topology": "merlin.singlerouter",
    })
    chiprtr.setSubComponent("topology", "merlin.singlerouter")

    memctrl = sst.Component("memory", "memHierarchy.MemController")
    memctrl.addParams({
        "clock": "1GHz",
        "addr_range_start": 0,
        "addr_range_end": balar_mmio_addr - 1,
    })
    mem_be = memctrl.setSubComponent("backend", "memHierarchy.simpleMem")
    mem_be.addParams({
        "access_time": "100 ns",
        "mem_size": "512MiB",
    })
    mem_hi = memctrl.setSubComponent("highlink", "memHierarchy.MemLink")

    directory = sst.Component("directory", "memHierarchy.DirectoryController")
    directory.addParams({
        "clock": "1GHz",
        "coherence_protocol": "MESI",
        "cache_line_size": 64,
        "entry_cache_size": 32768,
        "mshr_num_entries": 16,
        "addr_range_start": 0,
        "addr_range_end": balar_mmio_addr - 1,
    })
    dir_nic = directory.setSubComponent("highlink", "memHierarchy.MemNIC")
    dir_nic.addParams({
        "group": MEMORY_GROUP,
        "sources": MEMORY_SRC,
        "network_bw": NETWORK_BW,
    })

    sst.Link("cpu_l1").connect((cache_if, "port", "1ns"), (l1_cpu, "port", "1ns"))
    sst.Link("mem_bus").connect((mem_hi, "port", "1ns"), (directory, "lowlink", "1ns"))
    sst.Link("doorbell_l1_rtr").connect((l1_nic, "port", "1ns"), (chiprtr, "port0", "1ns"))
    sst.Link("doorbell_cpu_mmio_rtr").connect((cpu_mmio_nic, "port", "1ns"), (chiprtr, "port1", "1ns"))
    sst.Link("doorbell_balar_mmio_rtr").connect((balar_mmio_nic, "port", "1ns"), (chiprtr, "port2", "1ns"))
    sst.Link("doorbell_dma_mem_rtr").connect((dma_mem_nic, "port", "1ns"), (chiprtr, "port3", "1ns"))
    sst.Link("doorbell_dma_mmio_rtr").connect((dma_mmio_nic, "port", "1ns"), (chiprtr, "port4", "1ns"))
    sst.Link("doorbell_dir_rtr").connect((dir_nic, "port", "1ns"), (chiprtr, "port5", "1ns"))

    return cpu, balar_mmio_addr
