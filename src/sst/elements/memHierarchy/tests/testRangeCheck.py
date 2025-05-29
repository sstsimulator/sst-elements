import os
import sst
from mhlib import componentlist

RANGE_CHECK = 0  # set to 1 to demonstrate long initialization issue
NODES = 64
NUM_CPUS = 1

TOTAL_GB = 8*512
mem_info = {
    "gb"     : TOTAL_GB,
    "sz_str" : f"{TOTAL_GB}GiB",
    "sz"     : TOTAL_GB * 1024 * 1024 * 1024,
    "last"   : TOTAL_GB * 1024 * 1024 * 1024 - 1,
    "sz_rev" : TOTAL_GB * 1024 * 1024 * 1024,
}
MEM_PER_NODE = int(mem_info['sz'] / NODES)
MiB_PER_NODE = f"{(1024*1024)}MiB"

# NIC Groups
cpu_group = 1               # L2 cache level
node_group = 2              # Memory Controllers

class CPU():
    def __init__(self, cpu_num):
        self.comp = sst.Component(f"cpu{cpu_num}", "miranda.BaseCPU")
        self.comp.addParams({
            "verbose" : 0,
            "max_reqs_cycle" : 2,
            "cache_line_size" : 64,
            "maxmemreqpending" : 1,
            "clock"   : "2.0GHz",
            "pagecount" : 1,
            "pagesize" : mem_info['sz']
        })
        self.gen = self.comp.setSubComponent("generator", "miranda.RandomGenerator")
        self.gen.addParams({
            "verbose" : 0,
            "count"   : 1024,
            "max_address" : mem_info['sz'] - 64
        })
        # L1 Cache
        self.l1 = sst.Component(f"l1_{cpu_num}", "memHierarchy.Cache")
        self.l1.addParams({
            "node" : cpu_num,
            "verbose" : 0,
            "cache_frequency" : "2.0GHz",
            "cache_size" : "16KiB",
            "associativity" : "4",
            "access_latency_cycles" : "5",
            "L1" : "1",
            "cache_line_size" : "64",
            "coherence_protocol" : "MESI",
            "cache_type" : "inclusive",
            "force_noncacheable_reqs" : 1,
            "replacement_policy" : "lru",
        })
        # connect CPU to L1
        self.link_cpu_l1 = sst.Link(f"link_cpu_l1_{cpu_num}")
        self.link_cpu_l1.connect( (self.comp, "cache_link", "1ns"), (self.l1, "highlink", "1ns") )


class CPU_COMPLEX():
    def __init__(self,node):

        # CPU vector
        self.cpu = []
        for i in range(NUM_CPUS):
            self.cpu.append(CPU(i))

        # Bus connecting CPU L1 to next level of the memory hierarchy
        self.cpubus = sst.Component("cpubus", "memHierarchy.Bus")
        self.cpubus.addParams({
            "bus_frequency" : "2.0GHz",
        })

        # Level2 Cache
        self.l2cache = sst.Component("l2cache", "memHierarchy.Cache")
        self.l2cache.addParams({
            "node" : node,
            "verbose" : 0,
            "cache_frequency" : "2.0GHz",
            "cache_size" : "64 KiB",
            "associativity" : "16",
            "access_latency_cycles" : 32,
            "cache_line_size" : "64",
            "coherence_protocol" : "MESI",
            "mshr_num_entries" : 8,
            "mshr_latency_cycles" : 16
        })

        # L2 Cache interface to nic
        self.nic = self.l2cache.setSubComponent("lowlink","memHierarchy.MemNIC")
        self.nic.addParams({
            "group" : cpu_group,
            "network_bw" : "8800GiB/s",
            "network_input_buffer_size" : "1KiB",
            "network_output_buffer_size" : "1KiB",
            "range_check" : RANGE_CHECK,
        })

        # Connect cpubus to L2
        self.link_cpubus_l2 = sst.Link("link_cpubus_l2")
        self.link_cpubus_l2.connect(
            (self.cpubus, "lowlink0", "1ns"),
            (self.l2cache, "highlink", "1ns") )

        # Connect CPUs (L1) to cpubus
        self.link_l1_cpubus = []
        for i in range(NUM_CPUS):
            self.link_l1_cpubus.append(sst.Link(f"link_l1_{i}_cpubus"))
            self.link_l1_cpubus[i].connect(
                (self.cpu[i].l1, "lowlink",  "1ns"),
                (self.cpubus, f"highlink{i}", "1ns")
            )

class NODE():
    def __init__(self,node):
        node_mem_params = {}
        if NODES==1:
            memBot = 0;
            memTop = mem_info['sz']
            node_mem_params = {
                "node_id"          : 0,
                "backend.mem_size" : MiB_PER_NODE,
                "addr_range_start" : f"{memBot}",
                "addr_range_end"   : f"{memTop-1}"
            }
            print(f"memory{node} start=0x{memBot:X} end=0x{memTop:X} size=0x{MEM_PER_NODE:X}")
        else:
            # every 128MB switch memories
            istride = 0x8000000
            isize = "128MiB"
            istep = f"{128*NODES}MiB"
            memBot = (node%NODES) * istride
            memTop = mem_info['sz'] - ((NODES-node-1)*istride) - 1
            print(f"memory{node} start=0x{memBot:X} end=0x{(memTop):X} size=0x{MEM_PER_NODE:X} interleave_size={isize} interleave_step={istep}")

        self.memctrl = sst.Component(f"memory{node}", "memHierarchy.MemController")
        self.memctrl.addParams({
            "verbose" : 0,
            "node_id"          : node,
            "backend.mem_size" : MiB_PER_NODE,
            "addr_range_start" : f"{memBot}",
            "addr_range_end"   : f"{memTop}",
            "interleave_size"  : isize,
            "interleave_step"  : istep,
            "clock" : "2.0GHz",
            "request_width" : 64,
            "backing" : "malloc",
        })
        self.memory = self.memctrl.setSubComponent("backend", "memHierarchy.simpleMem")
        self.memory.addParams({
            "verbose"    : 0,
            "access_time" : "100ns",
            "mem_size"   : mem_info['sz_str']
        })

        # The memory controller NIC
        self.memNIC = self.memctrl.setSubComponent("highlink", "memHierarchy.MemNIC")
        self.memNIC.addParams({
            "group" : node_group,
            "sources" : [cpu_group, node_group],
            "destinations" : [node_group],
            "network_bw" : "8GiB/s",
            "network_input_buffer_size" : "1KiB",
            "network_output_buffer_size" : "1KiB",
            "range_check" : RANGE_CHECK,
        })

if __name__ == "__main__":

    cpucomplex = CPU_COMPLEX(0)
    node = []
    for n in range(NODES):
        node.append(NODE(n))

    local_network = sst.Component("local_network", "merlin.hr_router")
    local_network.addParams( {
        "id" : 0,
        "num_ports" : f"{1+NODES}",
        "topology"  : "merlin.singlerouter",
        "link_bw"   : "10TiB/s",
        "xbar_bw"   : "10TiB/s",
        "flit_size" : "72B",
        "input_latency" : "10ns",
        "output_latency" : "10ns",
        "input_buf_size" : "1KB",
        "output_buf_size" : "1KB",
    });
    local_network.setSubComponent("topology", "merlin.singlerouter")

    # connnect L2 NIC to Local Interconnect Network
    link_cache_net_0 = sst.Link("link_cache_net_0")
    link_cache_net_0.connect(
        (cpucomplex.nic, "port", "1ns" ),
        (local_network, "port0", "1ns" )
    )

    # connect local_network to directory controllers
    link_dir_net = []
    for n in range(NODES):
         link_dir_net.append(sst.Link(f"link_dir_net_{n}"))
         link_dir_net[n].connect(
             (local_network, f"port{n+1}", "1ns"),
             (node[n].memNIC, "port", "1ns")
         )

# Enable statistics
# sst.setStatisticLoadLevel(7)
# sst.setStatisticOutput("sst.statOutputConsole")
# for a in componentlist:
#     sst.enableAllStatisticsForComponentType(a)

# EOF
