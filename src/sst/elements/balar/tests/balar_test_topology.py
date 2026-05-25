"""
Shared SST topology builders for Balar contract tests (testcpu stand-in for Doorbell).

Topologies:
  router  — merlin.hr_router (default testBalar-testcpu.py pattern)
  bus     — memHierarchy.Bus (Doorbell P3 / coherent shared DRAM path)
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
    """Return (cpu, balar_mmio_addr, stat_links_dict) for merlin router topology."""
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


def build_testcpu_coherent_bus(balar_builder, cfg_file, balar_verbosity=0, dma_verbosity=0):
    """
    CPU L1 -> Bus <- Balar MMIO, DMA mem_iface, MemController (Doorbell P3 pattern).
    """
    balar_mmio_iface, balar_mmio_addr, dma_mem_if, dma_mmio_if = balar_builder.buildTestCPU(
        cfg_file, balar_verbosity, dma_verbosity
    )

    bus = sst.Component("dram_bus", "memHierarchy.Bus")
    bus.addParams({
        "bus_frequency": "2GHz",
        "drain_bus": "1",
    })

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
        "addr_range_end": balar_mmio_addr - 1,
    })
    l1_cpu = l1.setSubComponent("highlink", "memHierarchy.MemLink")
    l1_bus = l1.setSubComponent("lowlink", "memHierarchy.MemLink")

    cpu = sst.Component("cpu", "balar.BalarTestCPU")
    cpu.addParams({
        "clock": CLOCK,
        "verbose": balar_verbosity,
        "scratch_mem_addr": 0,
        "gpu_addr": balar_mmio_addr,
        "enable_memcpy_dump": False,
    })
    cpu_if = cpu.setSubComponent("memory", "memHierarchy.standardInterface")
    cpu_if.addParams(DEBUG_PARAMS)

    memctrl = sst.Component("memory", "memHierarchy.MemController")
    memctrl.addParams({
        "clock": "1GHz",
        "addr_range_end": balar_mmio_addr - 1,
    })
    mem_be = memctrl.setSubComponent("backend", "memHierarchy.simpleMem")
    mem_be.addParams({
        "access_time": "100 ns",
        "mem_size": "512MiB",
    })
    mem_hi = memctrl.setSubComponent("highlink", "memHierarchy.MemLink")

    sst.Link("cpu_l1").connect((cpu_if, "port", "1ns"), (l1_cpu, "port", "1ns"))
    sst.Link("l1_bus").connect((l1_bus, "port", "1ns"), (bus, "highlink0", "1ns"))
    sst.Link("balar_mmio_bus").connect((balar_mmio_iface, "port", "1ns"), (bus, "highlink1", "1ns"))
    sst.Link("dma_mem_bus").connect((dma_mem_if, "port", "1ns"), (bus, "highlink2", "1ns"))
    sst.Link("mem_bus").connect((mem_hi, "port", "1ns"), (bus, "lowlink0", "1ns"))

    # DMA MMIO still reaches memory controller directly (doorbell path for dma engine)
    dma_mmio_mem = sst.Component("dma_mmio_mem", "memHierarchy.MemController")
    dma_mmio_mem.addParams({
        "clock": "1GHz",
        "addr_range_start": balar_mmio_addr,
        "addr_range_end": balar_mmio_addr + 2048,
    })
    dma_mmio_be = dma_mmio_mem.setSubComponent("backend", "memHierarchy.simpleMem")
    dma_mmio_be.addParams({"access_time": "100 ns", "mem_size": "4KiB"})
    dma_mmio_hi = dma_mmio_mem.setSubComponent("highlink", "memHierarchy.MemLink")
    sst.Link("dma_mmio_link").connect((dma_mmio_if, "port", "1ns"), (dma_mmio_hi, "port", "1ns"))

    return cpu, balar_mmio_addr
