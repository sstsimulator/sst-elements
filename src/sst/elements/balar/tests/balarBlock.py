import os
import sst
try:
    import ConfigParser
except ImportError:
    import configparser as ConfigParser
from utils import *

# Constans shared across components
network_bw = "25GB/s"
clock = "2GHz"
balar_mmio_testcpu_addr = 4096
balar_mmio_vanadis_addr = 0x80100000
balar_mmio_size = 1024
dma_mmio_addr = balar_mmio_vanadis_addr + balar_mmio_size
dma_mmio_size = 512
debug_params = {"debug": 0, "debug_level": 0}

class Builder():
    """BalarMMIO and GPU memory hierarchy builder
    """
    def __init__(self):
        pass

    def build(self, cfgFile, balar_addr, separate_mem_iface=True, verbosity=0):
        """
            Build configuration for Balar and its memory hierarchy

        Args:
            cfgFile (str): gpgpusim configuration path 
            statFile (str): SST stat file path
            statLevel (str): stat level
            balar_addr (int): balar mmio address
            verbosity (int, optional): Verbosity of balar component. Defaults to 0.

        Raises:
            SystemExit: _description_

        Returns:
            balar and mmio interfaces
        """
        # BalarMMIO component
        gpuconfig = Config(cfgFile, verbose=verbosity)
        balar = sst.Component("balar", "balar.balarMMIO")
        balar.addParams({
            "verbose": verbosity,
            "clock": clock,
            "base_addr": balar_addr,
            "mmio_size": balar_mmio_size,
            "dma_addr": dma_mmio_addr,
            "separate_mem_iface": separate_mem_iface,
        })
        balar.addParams(gpuconfig.getGPUConfig())

        # Only set mmio_iface as mem_iface is used only for vanadis
        balar_mmio_iface = balar.setSubComponent("mmio_iface", "memHierarchy.standardInterface")

        # GPU memory hierarchy
        print("Configuring GPU Network-on-Chip...")

        # GPU Xbar group
        l1g_group = 1
        l2g_group = 2

        gpu_router_ports = gpuconfig.gpu_cores + gpuconfig.gpu_l2_parts

        GPUrouter = sst.Component("gpu_xbar", "shogun.ShogunXBar")
        GPUrouter.addParams(gpuconfig.getGPUXBarParams())
        GPUrouter.addParams({
            "verbose": 1,
            "port_count": gpu_router_ports,
        })

        # Connect Cores & l1caches to the router
        for next_core_id in range(gpuconfig.gpu_cores):
            print("Configuring GPU core %d..." % next_core_id)

            gpuPort = "requestGPUCacheLink%d" % next_core_id

            l1g = sst.Component("l1gcache_%d" % (next_core_id), "memHierarchy.Cache")
            l1g.addParams(gpuconfig.getGPUL1Params())
            l1g.addParams(debug_params)

            l1g_gpulink = l1g.setSubComponent("cpulink", "memHierarchy.MemLink")
            l1g_memlink = l1g.setSubComponent("memlink", "memHierarchy.MemNIC")
            l1g_memlink.addParams({"group": l1g_group})
            l1g_linkctrl = l1g_memlink.setSubComponent(
                "linkcontrol", "shogun.ShogunNIC")

            connect("gpu_cache_link_%d" % next_core_id,
                    balar, gpuPort,
                    l1g_gpulink, "port",
                    gpuconfig.default_link_latency).setNoCut()

            connect("l1gcache_%d_link" % next_core_id,
                    l1g_linkctrl, "port",
                    GPUrouter, "port%d" % next_core_id,
                    gpuconfig.default_link_latency)

        # Connect GPU L2 caches to the routers
        num_L2s_per_stack = gpuconfig.gpu_l2_parts // gpuconfig.hbmStacks
        sub_mems = gpuconfig.gpu_memory_controllers
        total_mems = gpuconfig.hbmStacks * sub_mems

        interleaving = 256
        next_mem = 0
        cacheStartAddr = 0x00
        memStartAddr = 0x00

        if (gpuconfig.gpu_l2_parts % total_mems) != 0:
            print("FAIL Number of L2s (%d) must be a multiple of the total number memory controllers (%d)." % (
                gpuconfig.gpu_l2_parts, total_mems))
            raise SystemExit

        for next_group_id in range(gpuconfig.hbmStacks):
            next_cache = next_group_id * sub_mems

            mem_l2_bus = sst.Component(
                "mem_l2_bus_" + str(next_group_id), "memHierarchy.Bus")
            mem_l2_bus.addParams({
                "bus_frequency": "4GHz",
                "drain_bus": "1"
            })

            for sub_group_id in range(sub_mems):
                memStartAddr = 0 + (256 * next_mem)
                endAddr = memStartAddr + \
                    gpuconfig.gpu_memory_capacity_inB - (256 * total_mems)

                if backend == "simple":
                    # Create SimpleMem
                    print("Configuring Simple mem part" + str(next_mem) +
                        " out of " + str(gpuconfig.hbmStacks) + "...")
                    mem = sst.Component("Simplehbm_" + str(next_mem),
                                        "memHierarchy.MemController")
                    mem.addParams(gpuconfig.get_GPU_mem_params(
                        total_mems, memStartAddr, endAddr))
                    membk = mem.setSubComponent("backend", "memHierarchy.simpleMem")
                    membk.addParams(gpuconfig.get_GPU_simple_mem_params())
                elif backend == "ddr":
                    # Create DDR (Simple)
                    print("Configuring DDR-Simple mem part" + str(next_mem) +
                        " out of " + str(gpuconfig.hbmStacks) + "...")
                    mem = sst.Component("DDR-shbm_" + str(next_mem),
                                        "memHierarchy.MemController")
                    mem.addParams(gpuconfig.get_GPU_ddr_memctrl_params(
                        total_mems, memStartAddr, endAddr))
                    membk = mem.setSubComponent("backend", "memHierarchy.simpleDRAM")
                    membk.addParams(gpuconfig.get_GPU_simple_ddr_params(next_mem))
                elif backend == "timing":
                    # Create DDR (Timing)
                    print("Configuring DDR-Timing mem part" + str(next_mem) +
                        " out of " + str(gpuconfig.hbmStacks) + "...")
                    mem = sst.Component("DDR-thbm_" + str(next_mem),
                                        "memHierarchy.MemController")
                    mem.addParams(gpuconfig.get_GPU_ddr_memctrl_params(
                        total_mems, memStartAddr, endAddr))
                    membk = mem.setSubComponent("backend", "memHierarchy.timingDRAM")
                    membk.addParams(gpuconfig.get_GPU_ddr_timing_params(next_mem))
                else:
                    # Create CramSim HBM
                    print("Creating HBM controller " + str(next_mem) +
                        " out of " + str(gpuconfig.hbmStacks) + "...")

                    mem = sst.Component("GPUhbm_" + str(next_mem),
                                        "memHierarchy.MemController")
                    mem.addParams(gpuconfig.get_GPU_hbm_memctrl_cramsim_params(
                        total_mems, memStartAddr, endAddr))

                    cramsimBridge = sst.Component(
                        "hbm_cs_bridge_" + str(next_mem), "CramSim.c_MemhBridge")
                    cramsimCtrl = sst.Component(
                        "hbm_cs_ctrl_" + str(next_mem), "CramSim.c_Controller")
                    cramsimDimm = sst.Component(
                        "hbm_cs_dimm_" + str(next_mem), "CramSim.c_Dimm")

                    cramsimBridge.addParams(
                        gpuconfig.get_GPU_hbm_cramsim_bridge_params(next_mem))
                    cramsimCtrl.addParams(
                        gpuconfig.get_GPU_hbm_cramsim_ctrl_params(next_mem))
                    cramsimDimm.addParams(
                        gpuconfig.get_GPU_hbm_cramsim_dimm_params(next_mem))

                    linkMemBridge = sst.Link("memctrl_cramsim_link_" + str(next_mem))
                    linkMemBridge.connect((mem, "cube_link", "2ns"),
                                        (cramsimBridge, "cpuLink", "2ns"))
                    linkBridgeCtrl = sst.Link("cramsim_bridge_link_" + str(next_mem))
                    linkBridgeCtrl.connect(
                        (cramsimBridge, "memLink", "1ns"), (cramsimCtrl, "txngenLink", "1ns"))
                    linkDimmCtrl = sst.Link("cramsim_dimm_link_" + str(next_mem))
                    linkDimmCtrl.connect(
                        (cramsimCtrl, "memLink", "1ns"), (cramsimDimm, "ctrlLink", "1ns"))

                print(" - Capacity: " + str(gpuconfig.gpu_memory_capacity_inB //
                    gpuconfig.hbmStacks) + " per HBM")
                print(" - Start Address: " + str(hex(memStartAddr)) +
                    " End Address: " + str(hex(endAddr)))

                connect("bus_mem_link_%d" % next_mem,
                        mem_l2_bus, "low_network_%d" % sub_group_id,
                        mem, "direct_link",
                        "50ps").setNoCut()

                next_mem = next_mem + 1

            for next_mem_id in range(num_L2s_per_stack):
                cacheStartAddr = 0 + (256 * next_cache)
                endAddr = cacheStartAddr + \
                    gpuconfig.gpu_memory_capacity_inB - (256 * total_mems)

                print("Creating L2 controller %d-%d (%d)..." %
                    (next_group_id, next_mem_id, next_cache))
                print(" - Start Address: " + str(hex(cacheStartAddr)) +
                    " End Address: " + str(hex(endAddr)))

                l2g = sst.Component("l2gcache_%d" % (next_cache), "memHierarchy.Cache")
                l2g.addParams(gpuconfig.getGPUL2Params(cacheStartAddr, endAddr))
                l2g_gpulink = l2g.setSubComponent("cpulink", "memHierarchy.MemNIC")
                l2g_gpulink.addParams({"group": l2g_group})
                l2g_linkctrl = l2g_gpulink.setSubComponent(
                    "linkcontrol", "shogun.ShogunNIC")
                l2g_memlink = l2g.setSubComponent("memlink", "memHierarchy.MemLink")

                connect("l2g_xbar_link_%d" % (next_cache),
                        GPUrouter, "port%d" % (gpuconfig.gpu_cores+(next_cache)),
                        l2g_linkctrl, "port",
                        gpuconfig.default_link_latency).setNoCut()

                connect("l2g_mem_link_%d" % (next_cache),
                        l2g_memlink, "port",
                        mem_l2_bus, "high_network_%d" % (next_mem_id),
                        gpuconfig.default_link_latency).setNoCut()

                print("++ %d-%d (%d)..." %
                    (next_cache, sub_mems, (next_cache + 1) % sub_mems))
                if (next_cache + 1) % sub_mems == 0:
                    next_cache = next_cache + total_mems - (sub_mems - 1)
                else:
                    next_cache = next_cache + 1

        # Return balar and its MMIO interface
        return balar, balar_mmio_iface
    
    def buildTestCPU(self, cfgFile, verbosity=0):
        balar, balar_mmio_iface = self.build(cfgFile, balar_mmio_testcpu_addr, separate_mem_iface=False, verbosity=verbosity)

        return balar, balar_mmio_iface, balar_mmio_testcpu_addr

    def buildVanadisIntegration(self, cfgFile, verbosity=0):
        """
            Build balar for vanadis integration, include balarTLB and dmaEngine for cudaMemcpy
           
            Overall connection:
            BalarMMIO <--mem_iface--> BalarTLBBus <--mem_iface--> dmaEngine
                                         |
            BalarTLB <-------------------|

        Args:
            cfgFile (str): gpgpusim configuration path 
            statFile (str): SST stat file path
            statLevel (str): stat level
            verbosity (int, optional): Verbosity of balar component. Defaults to 0.

        Returns:
            balarTLB and balar, dmaEngine mmio interfaces
        """
        balar, balar_mmio_iface = self.build(cfgFile, balar_mmio_vanadis_addr, separate_mem_iface=True, verbosity=verbosity)

        # Set mem interface
        balar_mem_iface = balar.setSubComponent("mem_iface", "memHierarchy.standardInterface")

        # DMA engine
        dma = sst.Component("dmaEngine", "balar.dmaEngine")
        dma.addParams({
            "verbose": verbosity,
            "clock": clock,
            "mmio_addr": dma_mmio_addr,
            "mmio_size": dma_mmio_size,
        })
        dma_mmio_if = dma.setSubComponent("mmio_iface", "memHierarchy.standardInterface")
        dma_mem_if = dma.setSubComponent("mem_iface", "memHierarchy.standardInterface")

        # Balar TLB
        balarTlbParams = {
            "debug_level": 0,
            "hitLatency": 10,
            "num_hardware_threads": 1,
            "num_tlb_entries_per_thread": 64,
            "tlb_set_size": 4,
        }

        balarTlbWrapperParams = {
            "debug_level": 0,
        }
        balarTlbWrapper = sst.Component("balarTlb", "mmu.tlb_wrapper")
        balarTlbWrapper.addParams(balarTlbWrapperParams)
        balarTlb = balarTlbWrapper.setSubComponent("tlb", "mmu.simpleTLB")
        balarTlb.addParams(balarTlbParams)

        # BalarBus for sharing TLB between DMA and Balar
        balarBus = sst.Component("balarBus", "memHierarchy.Bus")
        balarBus.addParams({
            "bus_frequency": "4GHz",
            "drain_bus": "1"
        })

        # Connect the data links for Balar and dmaEngine here to TLB
        connect("balarBus_balarTlb_link", balar_mem_iface, "port",
                balarBus, "high_network_0", "1ns")
        connect("dmaEngine_balarBus_link", dma_mem_if, "port", 
                balarBus, "high_network_1", "1ns")
        connect("balar_balarBus_link", balarBus, "low_network_0",
                balarTlbWrapper, "cpu_if", "1ns")
        
        # mem_iface replaced by TLB, keep mmio_iface
        return balarTlbWrapper, balarTlb, balar_mmio_iface, dma_mmio_if


