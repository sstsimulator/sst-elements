import sst

def sstcreatemodel():

    sst.creategraph()

    if sst.verbose():
        print "Verbose Model"

    id = sst.createcomponent("a0", "ariel.ariel")
    sst.addcompparam(id, "verbose", "16")
    sst.addcompparam(id, "executable", "/home/sdhammo/subversion/sst-simulator/test/testSuites/testM5/heatfort/heatfort")
    sst.addcompparam(id, "arieltool", "/home/sdhammo/subversion/sst-simulator/sst/elements/ariel/tool/arieltool.so")
    sst.addcomplink(id, "cpu_cache_link", "cache_link", "1ns")

    l1id = sst.createcomponent("l1cache", "memHierarchy.Cache")
    sst.addcompparam(l1id, "num_ways", "8")
    sst.addcompparam(l1id, "num_rows", "128")
    sst.addcompparam(l1id, "blocksize", "64")
    sst.addcompparam(l1id, "access_time", "1ns")
    sst.addcompparam(l1id, "num_upstream", "1")
    sst.addcomplink(l1id, "cpu_cache_link", "upstream0", "1ns")
    sst.addcomplink(l1id, "cache_bus_link", "snoop_link", "50ps")
    sst.addcompparam(l1id, "printStats", "1")
    
    membus = sst.createcomponent("membus", "memHierarchy.Bus")
    sst.addcompparam(membus, "numPorts", "2")
    sst.addcompparam(membus, "busDelay", "1ns")
    sst.addcomplink(membus, "cache_bus_link", "port0", "50ps")
    sst.addcomplink(membus, "mem_bus_link", "port1", "50ps")

    memory = sst.createcomponent("memory", "memHierarchy.MemController")
    sst.addcompparam(memory, "access_time", "1000ns")
    sst.addcompparam(memory, "mem_size", "512")
    sst.addcompparam(memory, "clock", "1GHz")
    sst.addcompparam(memory, "use_dramsim", "0")
    sst.addcompparam(memory, "device_ini", "DDR3_micron_32M_8B_x4_sg125.ini")
    sst.addcompparam(memory, "system_ini", "system.ini")
    sst.addcomplink(memory, "mem_bus_link", "snoop_link", "50ps")

    return 0
