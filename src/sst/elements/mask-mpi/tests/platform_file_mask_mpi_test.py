import sst
from sst.merlin.base import *

platdef = PlatformDefinition("platform_mask_mpi_test")
PlatformDefinition.registerPlatformDefinition(platdef)

platdef.addParamSet("operating_system",{
    "verbose" : "0",
})

platdef.addParamSet("topology",{
    "link_latency" : "20ns",
    "num_ports" : "32"
})

platdef.addParamSet("network_interface",{
    "link_bw" : "12 GB/s",
    "input_buf_size" : "16kB",
    "output_buf_size" : "16kB"
})

platdef.addClassType("network_interface","sst.merlin.interface.ReorderLinkControl")

platdef.addParamSet("router",{
    "link_bw" : "12 GB/s",
    "flit_size" : "8B",
    "xbar_bw" : "50GB/s",
    "input_latency" : "20ns",
    "output_latency" : "20ns",
    "input_buf_size" : "16kB",
    "output_buf_size" : "16kB",
    "num_vns" : 1,
    "xbar_arb" : "merlin.xbar_arb_lru",
})

platdef.addClassType("router","sst.merlin.base.hr_router")
