import os
import sst

quiet = True

memCapacity = 4 # In GB
memPageSize = 4 # in KB
memNumPages = memCapacity * 1024 * 1024 / memPageSize

mesh_stops_x        = 3
mesh_stops_y        = 3

mesh_clock          = 2200
ctrl_mesh_flit      = 8
data_mesh_flit      = 36
mesh_link_latency   = "100ps"    # Note, used to be 50ps, didn't seem to make a difference when bumping it up to 100
ctrl_mesh_link_bw   = str( (mesh_clock * 1000 * 1000 * ctrl_mesh_flit) ) + "B/s"
data_mesh_link_bw   = str( (mesh_clock * 1000 * 1000 * data_mesh_flit) ) + "B/s"

core_clock         = "1800MHz"
coherence_protocol = "MESI"

ctrl_network_buffers = "32B"
data_network_buffers = "288B"

ctrl_network_params = {
        "link_bw" : ctrl_mesh_link_bw,
        "flit_size" : str(ctrl_mesh_flit) + "B",
        "input_buf_size" : ctrl_network_buffers,
}

data_network_params = {
        "link_bw" : data_mesh_link_bw,
        "flit_size" : str(data_mesh_flit) + "B",
        "input_buf_size" : data_network_buffers,
        "port_priority_equal" : 1,
}

# Debug parameters for memH
debugAll = 0
debugL1 = max(debugAll, 0)
debugL2 = max(debugAll, 0)
debugDDRDC = max(debugAll, 0)
debugMemCtrl = max(debugAll, 0)
debugNIC = max(debugAll, 0)
debugLev = 3

# Verbose
verbose = 2

l1_cache_params = {
    "cache_frequency"    : core_clock,
    "coherence_protocol" : coherence_protocol,
    "replacement_policy" : "lru",
    "cache_size"         : "32KiB",
    "associativity"      : 8,
    "cache_line_size"    : 64,
    "access_latency_cycles" : 4,
    "tag_access_latency_cycles" : 1,
    "mshr_num_entries"   : 12, # Outstanding misses per core
    "maxRequestDelay"    : 10000000,
    "events_up_per_cycle" : 2,
    "mshr_latency_cycles" : 2,
    "max_requests_per_cycle" : 1,
    #"request_link_width" : "72B",
    #"response_link_width" : "36B",
    "L1"                 : 1,
    "verbose"            : verbose,
    "debug"              : debugL1,
    "debug_level"        : debugLev,
}

l2_prefetch_params = {
    "prefetcher" : "cassini.StridePrefetcher",
    "prefetcher.reach" : 16,
    "prefetcher.detect_range" : 1
}

l2_cache_params = {
    "cache_frequency"    : core_clock,
    "coherence_protocol" : coherence_protocol,
    "replacement_policy" : "lru",
    "cache_size"         : "1MiB",
    "associativity"      : 16,
    "cache_line_size"    : 64,
    "access_latency_cycles" : 8,   # Guess - co-processor s/w dev guide says 11 for 512KiB cache
    "tag_access_latency_cycles" : 3,
    "mshr_num_entries"   : 48, # Actually 48 reads and 32 writebacks
    #"max_requests_per_cycle" : 2,
    "mshr_latency_cycles" : 4,
    #"request_link_width" : "72B",
    "response_link_width" : "72B",
    "memNIC.req.linkcontrol" : "kingsley.linkcontrol",
    "memNIC.ack.linkcontrol" : "kingsley.linkcontrol",
    "memNIC.fwd.linkcontrol" : "kingsley.linkcontrol",
    "memNIC.data.linkcontrol" : "kingsley.linkcontrol",
    "memNIC.req.network_bw" : ctrl_mesh_link_bw,
    "memNIC.ack.network_bw" : ctrl_mesh_link_bw,
    "memNIC.fwd.network_bw" : ctrl_mesh_link_bw,
    "memNIC.data.network_bw" : data_mesh_link_bw,
    "memNIC.req.network_input_buffer_size" : ctrl_network_buffers,
    "memNIC.ack.network_input_buffer_size" : ctrl_network_buffers,
    "memNIC.fwd.network_input_buffer_size" : ctrl_network_buffers,
    "memNIC.data.network_input_buffer_size" : data_network_buffers,
    "memNIC.req.network_output_buffer_size" : ctrl_network_buffers,
    "memNIC.ack.network_output_buffer_size" : ctrl_network_buffers,
    "memNIC.fwd.network_output_buffer_size" : ctrl_network_buffers,
    "memNIC.data.network_output_buffer_size" : data_network_buffers,
    "memNIC.debug" : debugNIC,
    "memNIC.debug_level" : debugLev,
    "verbose" : verbose,
    "debug"              : debugL2,
    "debug_level"        : debugLev
}

###### DDR Directory #######
ddr_dc_params = {
    "coherence_protocol": coherence_protocol,
    "memNIC.req.network_bw" : ctrl_mesh_link_bw,
    "memNIC.ack.network_bw" : ctrl_mesh_link_bw,
    "memNIC.fwd.network_bw" : ctrl_mesh_link_bw,
    "memNIC.data.network_bw" : data_mesh_link_bw,
    "clock"             : str(mesh_clock) + "MHz",
    "entry_cache_size"  : 256*1024*1024, #Entry cache size of mem/blocksize
    "mshr_num_entries"  : 128,
    "access_latency_cycles" : 2,
    "memNIC.req.network_input_buffer_size" : ctrl_network_buffers,
    "memNIC.req.network_output_buffer_size" : ctrl_network_buffers,
    "memNIC.ack.network_input_buffer_size" : ctrl_network_buffers,
    "memNIC.ack.network_output_buffer_size" : ctrl_network_buffers,
    "memNIC.fwd.network_input_buffer_size" : ctrl_network_buffers,
    "memNIC.fwd.network_output_buffer_size" : ctrl_network_buffers,
    "memNIC.data.network_input_buffer_size" : data_network_buffers,
    "memNIC.data.network_output_buffer_size" : data_network_buffers,
    "verbose" : verbose,
    "debug"             : debugDDRDC,
    "debug_level"       : debugLev
}

##### TimingDRAM #####
# DDR4-2400
ddr_mem_timing_params = {
    "verbose" : verbose,
    "backing" : "none",
    "backend" : "memHierarchy.timingDRAM",
    "backend.clock" : "1200MHz",
    "backend.id" : 0,
    "backend.addrMapper" : "memHierarchy.simpleAddrMapper",
    "backend.channel.transaction_Q_size" : 32,
    "backend.channel.numRanks" : 2,
    "backend.channel.rank.numBanks" : 16,
    "backend.channel.rank.bank.CL" : 15,
    "backend.channel.rank.bank.CL_WR" : 12,
    "backend.channel.rank.bank.RCD" : 15,
    "backend.channel.rank.bank.TRP" : 15,
    "backend.channel.rank.bank.dataCycles" : 4,
    "backend.channel.rank.bank.pagePolicy" : "memHierarchy.simplePagePolicy",
    "backend.channel.rank.bank.transactionQ" : "memHierarchy.reorderTransactionQ",
    "backend.channel.rank.bank.pagePolicy.close" : 0,
    "memNIC.req.linkcontrol" : "kingsley.linkcontrol",
    "memNIC.ack.linkcontrol" : "kingsley.linkcontrol",
    "memNIC.fwd.linkcontrol" : "kingsley.linkcontrol",
    "memNIC.data.linkcontrol" : "kingsley.linkcontrol",
    "memNIC.req.network_bw" : ctrl_mesh_link_bw,
    "memNIC.ack.network_bw" : ctrl_mesh_link_bw,
    "memNIC.fwd.network_bw" : ctrl_mesh_link_bw,
    "memNIC.data.network_bw" : data_mesh_link_bw,
    "memNIC.req.network_input_buffer_size" : ctrl_network_buffers,
    "memNIC.ack.network_input_buffer_size" : ctrl_network_buffers,
    "memNIC.fwd.network_input_buffer_size" : ctrl_network_buffers,
    "memNIC.data.network_input_buffer_size" : data_network_buffers,
    "memNIC.req.network_output_buffer_size" : ctrl_network_buffers,
    "memNIC.ack.network_output_buffer_size" : ctrl_network_buffers,
    "memNIC.fwd.network_output_buffer_size" : ctrl_network_buffers,
    "memNIC.data.network_output_buffer_size" : data_network_buffers,
}


# Miranda STREAM Bench params        
thread_iters = 1000
cpu_params = {
    "verbose" : 0,
    "generator" : "miranda.STREAMBenchGenerator",
    "clock" : core_clock,
    "generatorParams.verbose" : 0,
    "generatorParams.n" : thread_iters,
    "generatorParams.operandWidth" : 8,
    "printStats" : 1
}

class DDRBuilder:
    def __init__(self, capacity):
        self.next_ddr_id = 0
        self.mem_capacity = capacity

    def build(self, nodeID):
        if not quiet:
            print "Creating DDR controller " + str(self.next_ddr_id) + " out of 4 on node " + str(nodeID) + "..."
            print " - Capacity: " + str(self.mem_capacity / 4) + " per DDR."

        mem = sst.Component("ddr_" + str(self.next_ddr_id), "memHierarchy.MemController")
        mem.addParams(ddr_mem_timing_params)
        mem.addParams({
                "backend.mem_size" : str(self.mem_capacity / 4) + "B",
            })


        mem.addParams({
            "memNIC.req.linkcontrol" : "kingsley.linkcontrol",
            "memNIC.ack.linkcontrol" : "kingsley.linkcontrol",
            "memNIC.fwd.linkcontrol" : "kingsley.linkcontrol",
            "memNIC.data.linkcontrol" : "kingsley.linkcontrol",
            "memNIC.req.network_bw" : ctrl_mesh_link_bw,
            "memNIC.ack.network_bw" : ctrl_mesh_link_bw,
            "memNIC.fwd.network_bw" : ctrl_mesh_link_bw,
            "memNIC.data.network_bw" : data_mesh_link_bw,
            "memNIC.addr_range_start" : (64 * self.next_ddr_id),
            "memNIC.addr_range_end" : (self.mem_capacity - (64 * self.next_ddr_id)),
            "memNIC.interleave_step" : str(4 * 64) + "B",
            "memNIC.interleave_size" : "64B",
            "memNIC.accept_region" : 0,
        })
        self.next_ddr_id = self.next_ddr_id + 1
        return (mem, "network", mesh_link_latency), (mem, "network_ack", mesh_link_latency), (mem, "network_fwd", mesh_link_latency), (mem, "network_data", mesh_link_latency)


class DDRDCBuilder:
    def __init__(self, capacity):
        self.next_ddr_dc_id = 0
        self.memCapacity = capacity

    def build(self, nodeID):
        # Stripe addresses across each mem & stripe those across each DC for the mem
        #   Interleave 64B blocks across 8 DCs (and then map every 4th to the same DDR)
       
        dcNum = nodeID % 2
        if nodeID == 1 or nodeID == 2:
            memId = 0
        elif nodeID == 3 or nodeID == 6:
            memId = 1
        elif nodeID == 4 or nodeID == 7:
            memId = 2
        elif nodeID == 5 or nodeID == 8:
            memId = 3
        
        myStart = 0 + (memId * 64) + (dcNum * 64 * 4)
        myEnd = self.memCapacity - 64 * (8 - memId - 4 * dcNum) + 63

        if not quiet:
            print "\tCreating ddr dc with start: " + str(myStart) + " end: " + str(myEnd)

        dc = sst.Component("ddr_dc_" + str(self.next_ddr_dc_id), "memHierarchy.DirectoryController")
        dc.addParams(ddr_dc_params)

        dc.addParams({
            "memNIC.req.linkcontrol" : "kingsley.linkcontrol",
            "memNIC.ack.linkcontrol" : "kingsley.linkcontrol",
            "memNIC.fwd.linkcontrol" : "kingsley.linkcontrol",
            "memNIC.data.linkcontrol" : "kingsley.linkcontrol",
            "memNIC.addr_range_start" : myStart,
            "memNIC.addr_range_end" : myEnd,
            "memNIC.interleave_step" : str(8 * 64) + "B",
            "memNIC.interleave_size" : "64B",
            "net_memory_name" : "ddr_" + str(memId),
        })

        self.next_ddr_dc_id = self.next_ddr_dc_id + 1
        return (dc, "network", mesh_link_latency), (dc, "network_ack", mesh_link_latency), (dc, "network_fwd", mesh_link_latency), (dc, "network_data", mesh_link_latency)


class TileBuilder:
    def __init__(self):
        self.next_tile_id = 0
        self.next_core_id = 0
        self.next_addr_id = 0
        self.base_a = 0
        self.base_b = thread_iters * 8 * 36
        self.base_c = self.base_b + thread_iters * 8 * 36

    def build(self, nodeID):
        # L2
        tileL2cache = sst.Component("l2cache_" + str(self.next_tile_id), "memHierarchy.Cache")
        tileL2cache.addParams(l2_cache_params)
        tileL2cache.addParams(l2_prefetch_params)
        tileL2cache.addParams({
            })

        l2bus = sst.Component("l2cachebus_" + str(self.next_tile_id), "memHierarchy.Bus")
        l2bus.addParams({
            "bus_frequency" : core_clock,
            })

        l2busLink = sst.Link("l2bus_link_" + str(self.next_tile_id))
        l2busLink.connect( (l2bus, "low_network_0", mesh_link_latency),
            (tileL2cache, "high_network_0", mesh_link_latency))
        l2busLink.setNoCut()

        self.next_tile_id = self.next_tile_id + 1

        # Left Core L1
        tileLeftL1 = sst.Component("l1cache_" + str(self.next_core_id), "memHierarchy.Cache")
        tileLeftL1.addParams(l1_cache_params)

        if not quiet:
            print "Creating core " + str(self.next_core_id) + " on tile: " + str(self.next_tile_id) + "..."

        # Left SMT
        leftSMT = sst.Component("smt_" + str(self.next_core_id), "memHierarchy.multithreadL1")
        leftSMT.addParams({
            "clock" : core_clock,
            "requests_per_cycle" : 2,
            "responses_per_cycle" : 2,
            })

        # Left Core
        mirandaL0 = sst.Component("thread_" + str(self.next_core_id), "miranda.BaseCPU")
        mirandaL1 = sst.Component("thread_" + str(self.next_core_id + 18), "miranda.BaseCPU")
        mirandaL0.addParams(cpu_params)
        mirandaL1.addParams(cpu_params)


        mirandaL0.addParams({
            "generatorParams.start_a" : self.base_a + self.next_core_id * thread_iters * 8,
            "generatorParams.start_b" : self.base_b + self.next_core_id * thread_iters * 8,
            "generatorParams.start_c" : self.base_c + self.next_core_id * thread_iters * 8
            })
        mirandaL1.addParams({
            "generatorParams.start_a" : self.base_a + (self.next_core_id + 18) * thread_iters * 8,
            "generatorParams.start_b" : self.base_b + (self.next_core_id + 18) * thread_iters * 8,
            "generatorParams.start_c" : self.base_c + (self.next_core_id + 18) * thread_iters * 8
            })

        # Thread 0
        leftSMTCPUlink0 = sst.Link("smt_cpu_" + str(self.next_core_id))
        leftSMTCPUlink0.connect( (mirandaL0, "cache_link", mesh_link_latency), (leftSMT, "thread0", mesh_link_latency) )
        # Thread 1
        leftSMTCPUlink1 = sst.Link("smt_cpu_" + str(self.next_core_id + 18))
        leftSMTCPUlink1.connect( (mirandaL1, "cache_link", mesh_link_latency), (leftSMT, "thread1", mesh_link_latency) )
        # SMT Shim <-> L1
        leftSMTL1link = sst.Link("l1cache_smt_" + str(self.next_core_id))
        leftSMTL1link.connect( (leftSMT, "cache", mesh_link_latency), (tileLeftL1, "high_network_0", mesh_link_latency) )

        leftSMTCPUlink0.setNoCut()
        leftSMTCPUlink1.setNoCut()
        leftSMTL1link.setNoCut()


        leftL1L2link = sst.Link("l1cache_link_" + str(self.next_core_id))
        leftL1L2link.connect( (l2bus, "high_network_0", mesh_link_latency),
            (tileLeftL1, "low_network_0", mesh_link_latency))
        leftL1L2link.setNoCut()

        self.next_core_id = self.next_core_id + 1

        tileRightL1 = sst.Component("l1cache_" + str(self.next_core_id), "memHierarchy.Cache")
        tileRightL1.addParams(l1_cache_params)

        if not quiet:
            print "Creating core " + str(self.next_core_id) + " on tile: " + str(self.next_tile_id) + "..."
        
        # Right SMT
        rightSMT = sst.Component("smt_" + str(self.next_core_id), "memHierarchy.multithreadL1")
        rightSMT.addParams({
            "clock" : core_clock,
            "requests_per_cycle" : 2,
            "responses_per_cycle" : 2,
            })

        # Right Core
        mirandaR0 = sst.Component("thread_" + str(self.next_core_id), "miranda.BaseCPU")
        mirandaR1 = sst.Component("thread_" + str(self.next_core_id + 18), "miranda.BaseCPU")
        mirandaR0.addParams(cpu_params)
        mirandaR1.addParams(cpu_params)


        mirandaR0.addParams({
            "generatorParams.start_a" : self.base_a + self.next_core_id * thread_iters * 8,
            "generatorParams.start_b" : self.base_b + self.next_core_id * thread_iters * 8,
            "generatorParams.start_c" : self.base_c + self.next_core_id * thread_iters * 8
            })
        mirandaR1.addParams({
            "generatorParams.start_a" : self.base_a + (self.next_core_id + 18) * thread_iters * 8,
            "generatorParams.start_b" : self.base_b + (self.next_core_id + 18) * thread_iters * 8,
            "generatorParams.start_c" : self.base_c + (self.next_core_id + 18) * thread_iters * 8
            })

        # Thread 0
        rightSMTCPUlink0 = sst.Link("smt_cpu_" + str(self.next_core_id))
        rightSMTCPUlink0.connect( (mirandaR0, "cache_link", mesh_link_latency), (rightSMT, "thread0", mesh_link_latency) )
        # Thread 1
        rightSMTCPUlink1 = sst.Link("smt_cpu_" + str(self.next_core_id + 18))
        rightSMTCPUlink1.connect( (mirandaR1, "cache_link", mesh_link_latency), (rightSMT, "thread1", mesh_link_latency) )
        # SMT Shim <-> L1
        rightSMTL1link = sst.Link("l1cache_smt_" + str(self.next_core_id))
        rightSMTL1link.connect( (rightSMT, "cache", mesh_link_latency), (tileRightL1, "high_network_0", mesh_link_latency) )

        rightSMTCPUlink0.setNoCut()
        rightSMTCPUlink1.setNoCut()
        rightSMTL1link.setNoCut()

        rightL1L2link = sst.Link("l1cache_link_" + str(self.next_core_id))
        rightL1L2link.connect( (l2bus, "high_network_1", mesh_link_latency),
                        (tileRightL1, "low_network_0", mesh_link_latency))
        rightL1L2link.setNoCut()

        self.next_core_id = self.next_core_id + 1

        return (tileL2cache, "directory", mesh_link_latency), (tileL2cache, "directory_ack", mesh_link_latency), (tileL2cache, "directory_fwd", mesh_link_latency), (tileL2cache, "directory_data", mesh_link_latency)


tileBuilder = TileBuilder()
memBuilder  = DDRBuilder(memCapacity * 1024 * 1024 * 1024)
DCBuilder = DDRDCBuilder(memCapacity * 1024 * 1024 * 1024)

def setNodeDist(nodeId, rtrreq, rtrack, rtrfwd, rtrdata):
    port = nodeId % 2   # Even port = tile, odd = DC
    actNode = nodeId // 2

    if nodeId == 1 or nodeId == 3 or nodeId == 5 or nodeId == 7:
        req, ack, fwd, data = memBuilder.build(nodeId)
        if nodeId == 1:
            port = "north"
        elif nodeId == 3:
            port = "west"
        elif nodeId == 5:
            port = "east"
        elif nodeId == 7:
            port = "south"
        
        rtrreqport = sst.Link("krtr_req_" + port + "_" +str(nodeId))
        rtrreqport.connect( (rtrreq, port, mesh_link_latency), req )
        rtrackport = sst.Link("krtr_ack_" + port + "_" + str(nodeId))
        rtrackport.connect( (rtrack, port, mesh_link_latency), ack )
        rtrfwdport = sst.Link("krtr_fwd_" + port + "_" + str(nodeId))
        rtrfwdport.connect( (rtrfwd, port, mesh_link_latency), fwd )
        rtrdataport = sst.Link("kRtr_data_" + port + "_" + str(nodeId))
        rtrdataport.connect( (rtrdata, port, mesh_link_latency), data )

    # Place tiles on all routers
    tilereq, tileack, tilefwd, tiledata = tileBuilder.build(nodeId)
    reqport0 = sst.Link("krtr_req_port0_" + str(nodeId))
    reqport0.connect( (rtrreq, "local0", mesh_link_latency), tilereq )
    ackport0 = sst.Link("krtr_ack_port0_" + str(nodeId))
    ackport0.connect( (rtrack, "local0", mesh_link_latency), tileack )
    fwdport0 = sst.Link("krtr_fwd_port0_" + str(nodeId))
    fwdport0.connect( (rtrfwd, "local0", mesh_link_latency), tilefwd )
    dataport0 = sst.Link("kRtr_data_port0_" + str(nodeId))
    dataport0.connect( (rtrdata, "local0", mesh_link_latency), tiledata )

    # Place DC at every tile except 0
    if nodeId != 0:
        req, ack, fwd, data = DCBuilder.build(nodeId)
        reqport1 = sst.Link("krtr_req_port1_" + str(nodeId))
        reqport1.connect( (rtrreq, "local1", mesh_link_latency), req )
        ackport1 = sst.Link("krtr_ack_port1_" + str(nodeId))
        ackport1.connect( (rtrack, "local1", mesh_link_latency), ack )
        fwdport1 = sst.Link("krtr_fwd_port1_" + str(nodeId))
        fwdport1.connect( (rtrfwd, "local1", mesh_link_latency), fwd )
        dataport1 = sst.Link("kRtr_data_port1_" + str(nodeId))
        dataport1.connect( (rtrdata, "local1", mesh_link_latency), data )

# Build Kingsley Mesh
kRtrReq=[]
kRtrAck=[]
kRtrFwd=[]
kRtrData=[]
for x in range (0, mesh_stops_x):
    for y in range (0, mesh_stops_y):
        nodeNum = len(kRtrReq)
        kRtrReq.append(sst.Component("krtr_req_" + str(nodeNum), "kingsley.noc_mesh"))
        kRtrReq[-1].addParams(ctrl_network_params)
        kRtrAck.append(sst.Component("krtr_ack_" + str(nodeNum), "kingsley.noc_mesh"))
        kRtrAck[-1].addParams(ctrl_network_params)
        kRtrFwd.append(sst.Component("krtr_fwd_" + str(nodeNum), "kingsley.noc_mesh"))
        kRtrFwd[-1].addParams(ctrl_network_params)
        kRtrData.append(sst.Component("krtr_data_" + str(nodeNum), "kingsley.noc_mesh"))
        kRtrData[-1].addParams(data_network_params)
        
        kRtrReq[-1].addParams({"local_ports" : 2})
        kRtrAck[-1].addParams({"local_ports" : 2})
        kRtrFwd[-1].addParams({"local_ports" : 2})
        kRtrData[-1].addParams({"local_ports" : 2})

i = 0
for y in range(0, mesh_stops_y):
    for x in range (0, mesh_stops_x):

        # North-south connections
        if y != (mesh_stops_y -1):
            kRtrReqNS = sst.Link("krtr_req_ns_" + str(i))
            kRtrReqNS.connect( (kRtrReq[i], "south", mesh_link_latency), (kRtrReq[i + mesh_stops_x], "north", mesh_link_latency) )
            kRtrAckNS = sst.Link("krtr_ack_ns_" + str(i))
            kRtrAckNS.connect( (kRtrAck[i], "south", mesh_link_latency), (kRtrAck[i + mesh_stops_x], "north", mesh_link_latency) )
            kRtrFwdNS = sst.Link("krtr_fwd_ns_" + str(i))
            kRtrFwdNS.connect( (kRtrFwd[i], "south", mesh_link_latency), (kRtrFwd[i + mesh_stops_x], "north", mesh_link_latency) )
            kRtrDataNS = sst.Link("krtr_data_ns_" + str(i))
            kRtrDataNS.connect( (kRtrData[i], "south", mesh_link_latency), (kRtrData[i + mesh_stops_x], "north", mesh_link_latency) )

        if x != (mesh_stops_x - 1):
            kRtrReqEW = sst.Link("krtr_req_ew_" + str(i))
            kRtrReqEW.connect( (kRtrReq[i], "east", mesh_link_latency), (kRtrReq[i+1], "west", mesh_link_latency) )
            kRtrAckEW = sst.Link("krtr_ack_ew_" + str(i))
            kRtrAckEW.connect( (kRtrAck[i], "east", mesh_link_latency), (kRtrAck[i+1], "west", mesh_link_latency) )
            kRtrFwdEW = sst.Link("krtr_fwd_ew_" + str(i))
            kRtrFwdEW.connect( (kRtrFwd[i], "east", mesh_link_latency), (kRtrFwd[i+1], "west", mesh_link_latency) )
            kRtrDataEW = sst.Link("krtr_data_ew_" + str(i))
            kRtrDataEW.connect( (kRtrData[i], "east", mesh_link_latency), (kRtrData[i+1], "west", mesh_link_latency) )

        setNodeDist(i, kRtrReq[i], kRtrAck[i], kRtrFwd[i], kRtrData[i])
        i = i + 1

# Enable SST Statistics Outputs for this simulation
sst.setStatisticLoadLevel(16)
sst.enableAllStatisticsForAllComponents({"type":"sst.AccumulatorStatistic"})
sst.setStatisticOutput("sst.statOutputConsole")
