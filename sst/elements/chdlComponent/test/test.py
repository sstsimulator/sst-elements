import sst;

sst.setProgramOption("timebase", "1 ps");
sst.setProgramOption("stopAtCycle", "1ms");

NCPUS = 8;
CPROT = "MESI"
BENCHMARK = "vecsum"

test_dir = "/home/cdkerse/src/sst-simulator/sst/elements/chdlComponent/test/"
netlist_path = test_dir + "score.nand"
meminit_path = test_dir + BENCHMARK + ".bin"

cpus = list();
cpus_l1 = list();
l1_l2 = list();
chip_net = list();
l1 = list();
l2 = list();

chiprtr = sst.Component("chiprtr", "merlin.hr_router");
chiprtr.addParams({
  "num_ports" : "%d"%(NCPUS+1),
  "link_bw" : "2GB/s",
  "xbar_bw" : "2GB/s",
  "flit_size" : "72B",
  "input_buf_size" : "1KB",
  "output_buf_size" : "1KB",
  "topology" : "merlin.singlerouter",
  "id" : "0"
});

dirctrl0 = sst.Component("dirctrl0", "memHierarchy.DirectoryController");
dirctrl0.addParams({
  "coherence_protocol" : CPROT,
  "network_bw" : "2GB/s",
  "network_address" : "0",
  "addr_range_start" : "0x00000000",
  "addr_range_end" : "0xe8000000",
  "backingStoreSize" : "0"
});

memory = sst.Component("memcontroller", "memHierarchy.MemController");
memory.addParams({
  "coherence_protocol" : CPROT,
  "access_time" : "50ns",
  "mem_size" : "4096",
  "clock" : "1GHz"
});

dir_net_0 = sst.Link("dir_net_0");
dir_net_0.connect((chiprtr, "port0", "5ns"), (dirctrl0, "network", "5ns"));

dir_mem = sst.Link("dir_mem_0");
dir_mem.connect((memory, "direct_link", "10ns"), (dirctrl0, "memory", "10ns"));

for i in range(0, NCPUS):
  cpus.append(sst.Component("c%d"%i, "chdlComponent"));
  cpus[i].addParams({
    "netlist" : netlist_path,
    "id" : "%d"%i,
    "cores" : NCPUS,
    "debug" : 1,
    "debugLevel" : 10
  });
  l1.append(sst.Component("l1_%d"%i, "memHierarchy.Cache"));
  l1[i].addParams({
    "coherence_protocol" : CPROT,
    "cache_frequency" : "1GHz",
    "replacement_policy" : "LRU",
    "associativity" : "4",
    "cache_size" : "8kB",
    "cache_line_size" : "64",
    "access_latency_cycles" : "1",
    "low_network_links" : "1",
    "L1" : "1"
  });
  l2.append(sst.Component("l2_%d"%i, "memHierarchy.Cache"));
  l2[i].addParams({
    "coherence_protocol" : CPROT,
    "cache_frequency" : "1GHz",
    "replacement_policy" : "LRU",
    "mode" : "INCLUSIVE",
    "associativity" : "16",
    "cache_size" : "64kB",
    "cache_line_size" : "64",
    "access_latency_cycles" : "10",
    "L1" : "0",
    "directory_at_next_level" : "1",
    "high_network_links" : "1",
    "network_address" : "%d"%(1+i),
    "network_bw" : "2GB/s",
  });
  cpus_l1.append(sst.Link("link_cpu_l1_%d"%i));
  cpus_l1[i].connect(
    (cpus[i], "memLink", "0ns"),
    (l1[i], "high_network_0", "0ns")
  );
  l1_l2.append(sst.Link("link_l1_l2_%d"%i));
  l1_l2[i].connect(
    (l1[i], "low_network_0", "4ns"),
    (l2[i], "high_network_0", "4ns")
  );
  chip_net.append(sst.Link("chip_net_%d"%i));
  chip_net[i].connect(
    (l2[i], "directory", "10ns"),
    (chiprtr, "port%d"%(i+1), "10ns")
  );

cpus[0].addParams({"memInit" : meminit_path});
