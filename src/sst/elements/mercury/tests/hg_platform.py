import sst
from sst.merlin.base import *

platdef = PlatformDefinition.compose("hg-platform",[("firefly-defaults","ALL")])
PlatformDefinition.registerPlatformDefinition(platdef)

platdef.addClassType("network_interface","sst.merlin.interface.ReorderLinkControl")

platdef.addParamSet("topology",{
     "link_latency" : "60ns" })

platdef.addParamSet("router",{
    "flit_size" : "16B",
    "xbar_bw" : "40GB/s",
    "link_bw" : "25GB/s",
    "input_buf_size" : "14kB",
    "output_buf_size" : "14kB" })

platdef.addParamSet("network_interface",{
    "link_bw" : "25 GB/s",
    "input_buf_size" : "14kB",
    "output_buf_size" : "14kB"
})
