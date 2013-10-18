import sst

def sstcreatemodel():

    sst.creategraph()

    if sst.verbose():
        print "Verbose Model"

    id = sst.createcomponent("a0", "ariel.ariel")
    sst.addcompparam(id, "verbose", "1")
    sst.addcompparam(id, "executable", "/home/sdhammo/xgc/xgc_co_design/sorting/spsort/bsr")
    sst.addcompparam(id, "arieltool", "/home/sdhammo/subversion/sst-simulator/sst/elements/ariel/tool/arieltool.so")
    sst.addcompparam(id, "fastmempagecount", "131072")

    sst.addcompparam(id, "appargcount", "6")
    sst.addcompparam(id, "apparg0", "10000")
    sst.addcompparam(id, "apparg1", "1")
    sst.addcompparam(id, "apparg2", "1048576")
    sst.addcompparam(id, "apparg3", "512")
    sst.addcompparam(id, "apparg4", "4")
    sst.addcompparam(id, "apparg5", "8")

    corecount = 8;

    sst.addcompparam(id, "corecount", str(corecount))

    print "Simulation core count: ", corecount

    membus0 = sst.createcomponent("membus0", "memHierarchy.Bus")
    sst.addcompparam(membus0, "numPorts", str(corecount/2 + 1))
    sst.addcompparam(membus0, "busDelay", "1ns")

    membus1 = sst.createcomponent("membus1", "memHierarchy.Bus")
    sst.addcompparam(membus1, "numPorts", str(corecount/2 + 1))
    sst.addcompparam(membus1, "busDelay", "1ns")
    #    sst.addcompparam(membus, "atomicDelivery", "1")

    # Create a series of caches all linked to the Ariel "core"
    max_core = corecount - 1;

    for x in range(0, 4):
       sst.addcomplink(id, "cpu_cache_link_" + str(x), "cache_link_" + str(x), "1ns")
       l1id = sst.createcomponent("l1cache_" + str(x), "memHierarchy.Cache")
       sst.addcompparam(l1id, "num_ways", "2")
       sst.addcompparam(l1id, "num_rows", "256")
       sst.addcompparam(l1id, "blocksize", "64")
       sst.addcompparam(l1id, "access_time", "1ns")
       sst.addcompparam(l1id, "num_upstream", "1")
       sst.addcompparam(l1id, "next_level", "l2cache_0")
       sst.addcomplink(l1id, "cpu_cache_link_" + str(x), "upstream0", "1ns")
       sst.addcomplink(l1id, "cache_bus_link_" + str(x), "snoop_link", "1ns")
       sst.addcompparam(l1id, "printStats", "1")
       sst.addcomplink(membus0, "cache_bus_link_" + str(x), "port" + str(x), "1ns")
       print "added cache_bus_link_%d at port %d\n"%(x,x)

    for x in range(4, 8):
       sst.addcomplink(id, "cpu_cache_link_" + str(x), "cache_link_" + str(x), "1ns")
       l1id = sst.createcomponent("l1cache_" + str(x), "memHierarchy.Cache")
       sst.addcompparam(l1id, "num_ways", "2")
       sst.addcompparam(l1id, "num_rows", "256")
       sst.addcompparam(l1id, "blocksize", "64")
       sst.addcompparam(l1id, "access_time", "1ns")
       sst.addcompparam(l1id, "num_upstream", "1")
       sst.addcompparam(l1id, "next_level", "l2cache_1")
       sst.addcomplink(l1id, "cpu_cache_link_" + str(x), "upstream0", "1ns")
       sst.addcomplink(l1id, "cache_bus_link_" + str(x), "snoop_link", "1ns")
       sst.addcompparam(l1id, "printStats", "1")
       sst.addcomplink(membus1, "cache_bus_link_" + str(x), "port" + str(x-4), "1ns")
       print "added cache_bus_link_%d at port %d\n"%(x,x-4)

    l2_q1 = sst.createcomponent("l2cache_0", "memHierarchy.Cache")
    sst.addcompparam(l2_q1, "num_ways", "16")
    sst.addcompparam(l2_q1, "num_rows", "512")
    sst.addcompparam(l2_q1, "blocksize", "64")
    sst.addcompparam(l2_q1, "access_time", "9ns")
    sst.addcompparam(l2_q1, "printStats", "1")
    sst.addcompparam(l2_q1, "net_addr", "2")
    #sst.addcompparam(l2_q1, "debug", "3")
    sst.addcompparam(l2_q1, "mode", "INCLUSIVE")
    sst.addcomplink(l2_q1, "l2cache_0_link", "snoop_link", "1ns")
    sst.addcomplink(l2_q1, "l2cache_0_dirlink", "directory_link", "10ns")

    l2_q2 = sst.createcomponent("l2cache_1", "memHierarchy.Cache")
    sst.addcompparam(l2_q2, "num_ways", "16")
    sst.addcompparam(l2_q2, "num_rows", "512")
    sst.addcompparam(l2_q2, "blocksize", "64")
    sst.addcompparam(l2_q2, "access_time", "9ns")
    sst.addcompparam(l2_q2, "printStats", "1")
    sst.addcompparam(l2_q2, "net_addr", "3")
    #sst.addcompparam(l2_q2, "debug", "3")
    sst.addcompparam(l2_q2, "mode", "INCLUSIVE")
    sst.addcomplink(l2_q2, "l2cache_1_link", "snoop_link", "1ns")
    sst.addcomplink(l2_q2, "l2cache_1_dirlink", "directory_link", "10ns")

    # Put a link to memory from the memory bus
    sst.addcomplink(membus0, "l2cache_0_link", "port%d"%(corecount/2), "1ns")
    sst.addcomplink(membus1, "l2cache_1_link", "port%d"%(corecount/2), "1ns")

    print "Creating memController..."
    memory = sst.createcomponent("memory", "memHierarchy.MemController")
    sst.addcompparam(memory, "access_time", "70ns")
    sst.addcompparam(memory, "rangeStart", "0x00000000")
    sst.addcompparam(memory, "mem_size", "512")
    sst.addcompparam(memory, "clock", "1GHz")
    sst.addcompparam(memory, "printStats", "1")
    sst.addcompparam(memory, "use_dramsim", "0")
    #sst.addcompparam(memory, "debug", "3")
    sst.addcompparam(memory, "device_ini", "DDR3_micron_32M_8B_x4_sg125.ini")
    sst.addcompparam(memory, "system_ini", "system_far.ini")
    sst.addcomplink(memory, "dir1_mem_link", "direct_link", "50ps")

    print "Creating near memController..."
    nearMemory = sst.createcomponent("nearmemory", "memHierarchy.MemController")
    sst.addcompparam(nearMemory, "access_time", "70ns")
    sst.addcompparam(nearMemory, "rangeStart", "0x0")
    sst.addcompparam(nearMemory, "clock", "1GHz")
    sst.addcompparam(nearMemory, "mem_size", "512")
    sst.addcompparam(nearMemory, "printStats", "1")
    sst.addcompparam(nearMemory, "use_dramsim", "0")
    #sst.addcompparam(nearMemory, "debug", "3")
    sst.addcompparam(nearMemory, "device_ini", "DDR3_micron_32M_8B_x4_sg125.ini")
    sst.addcompparam(nearMemory, "system_ini", "system_near.ini")
    sst.addcomplink(nearMemory, "dir0_mem_link", "direct_link", "50ps")

    print "Creating Merlin Network..."
    merlin = sst.createcomponent("chiprtr", "merlin.hr_router")
    sst.addcompparam(merlin, "num_ports", "5")
    sst.addcompparam(merlin, "num_vcs", "3")
    sst.addcompparam(merlin, "link_bw", "5GHz")
    sst.addcompparam(merlin, "xbar_bw", "5GHz")
    sst.addcompparam(merlin, "topology", "merlin.singlerouter")
    sst.addcompparam(merlin, "id", "0")
    sst.addcomplink(merlin, "dir0_net_link", "port0", "10ns")
    sst.addcomplink(merlin, "dir1_net_link", "port1", "10ns")
    sst.addcomplink(merlin, "l2cache_0_dirlink", "port2", "10ns")
    sst.addcomplink(merlin, "l2cache_1_dirlink", "port3", "10ns")
    sst.addcomplink(merlin, "dma_dirlink", "port4", "10ns")

    print "Creating directory controller..."
    dirctrl0 = sst.createcomponent("dirctrl0", "memHierarchy.DirectoryController")
    sst.addcompparam(dirctrl0, "network_addr", "0")
    sst.addcompparam(dirctrl0, "network_bw", "1GHz")
    sst.addcompparam(dirctrl0, "addrRangeStart", "0x0")
    sst.addcompparam(dirctrl0, "addrRangeEnd", "0x20000000")
    sst.addcompparam(dirctrl0, "backingStoreSize", "0")
    sst.addcompparam(dirctrl0, "entryCacheSize", "16384")
    sst.addcompparam(dirctrl0, "printStats", "1")
    sst.addcompparam(dirctrl0, "debug", "3")
    sst.addcomplink(dirctrl0, "dir0_mem_link", "memory", "50ps")
    sst.addcomplink(dirctrl0, "dir0_net_link", "network", "10ns")


    print "Creating second directory controller..."
    dirctrl1 = sst.createcomponent("dirctrl1", "memHierarchy.DirectoryController")
    sst.addcompparam(dirctrl1, "network_addr", "1")
    sst.addcompparam(dirctrl1, "network_bw", "1GHz")
    sst.addcompparam(dirctrl1, "addrRangeStart", "0x20000000")
    sst.addcompparam(dirctrl1, "addrRangeEnd", "0x40000000")
    sst.addcompparam(dirctrl1, "backingStoreSize", "0")
    sst.addcompparam(dirctrl1, "entryCacheSize", "16384")
    sst.addcompparam(dirctrl1, "printStats", "1")
    #sst.addcompparam(dirctrl1, "debug", "3")
    sst.addcomplink(dirctrl1, "dir1_mem_link", "memory", "50ps")
    sst.addcomplink(dirctrl1, "dir1_net_link", "network", "10ns")

    dmactrl = sst.createcomponent("dmactrl", "memHierarchy.DMAEngine")
    sst.addcompparam(dmactrl, "debug", "3")
    sst.addcompparam(dmactrl, "netAddr", "4")
    sst.addcompparam(dmactrl, "clockRate", "1GHz")
    sst.addcomplink(dmactrl, "dma_dirlink", "netLink", "10ns")
    sst.addcomplink(dmactrl, "ariel_dma", "cmdLink", "1ns")

    sst.addcomplink(id, "ariel_dma", "dmaLink", "1ns")

    print "done configuring SST"

    return 0
