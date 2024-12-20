import sst
from sst.merlin.base import *

platdef = PlatformDefinition("platform_mask_mpi_test")
PlatformDefinition.registerPlatformDefinition(platdef)

platdef.addParamSet("node",{
    "verbose"                  : "0",
    "name"                     : "hg.NodeCL",
    "negligible_compute_bytes" : "64B",
    "parallelism"              : "1.0",
    "frequency"                : "2.1GHz",
    "flow_mtu"                 : "512",
    "channel_bandwidth"        : "11.2 GB/s",
    "num_channels"             : "4",
})

platdef.addParamSet("nic",{
    "verbose" : "0",
    "mtu"     : "4096 B",
})

platdef.addParamSet("operating_system",{
    "verbose" : "0",
    "name"    : "hg.OperatingSystemCL"
})

platdef.addParamSet("topology",{
    "link_latency" : "20ns",
    "num_ports" : "32"
})

platdef.addParamSet("network_interface",{
    "link_bw" : "11.25 GB/s",
    "input_buf_size" : "32kB",
    "output_buf_size" : "32kB"
})

platdef.addClassType("network_interface","sst.merlin.interface.ReorderLinkControl")

platdef.addParamSet("router",{
    "link_bw" : "11.25 GB/s",
    "flit_size" : "8B",
    "xbar_bw" : "50GB/s",
    "input_latency" : "20ns",
    "output_latency" : "20ns",
    "input_buf_size" : "32kB",
    "output_buf_size" : "32kB",
    "num_vns" : 1,
    "xbar_arb" : "merlin.xbar_arb_lru",
})

platdef.addParamSet("operating_system", {
    "ncores" : "24",
    "nsockets" : "4",
    "app1.post_rdma_delay" : "1.5us",
    "app1.post_header_delay" : "0.5us",
    "app1.poll_delay" : "0us",
    "app1.rdma_pin_latency" : "5.43us",
    "app1.rdma_page_delay" : "50.50ns",
    "app1.rdma_page_size" : "4096",
    "app1.max_vshort_msg_size" : "4096 B",
    "app1.max_eager_msg_size" : "32768 B",
    "app1.use_put_window" : "false",
    "app1.compute_library_access_width" : "64",
    "app1.compute_library_loop_overhead" : "1.0",
})

platdef.addClassType("router","sst.merlin.base.hr_router")
