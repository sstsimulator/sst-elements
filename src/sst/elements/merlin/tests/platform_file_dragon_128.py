import sst
from sst.merlin.base import *

platdef = PlatformDefinition("platform_dragon_128")
PlatformDefinition.registerPlatformDefinition(platdef)


platdef.addParamSet("topology",{
    "hosts_per_router" : 4,
    "routers_per_group" : 8,
    "intergroup_links" : 4,
    "num_groups" : 4,
    "algorithm" : "adaptive-local",
    "link_latency" : "20ns"
})

platdef.addClassType("topology","sst.merlin.topology.topoDragonFly")
                    
platdef.addParamSet("router",{
    "link_bw" : "4GB/s",
    "flit_size" : "8B",
    "xbar_bw" : "4GB/s",
    "input_latency" : "20ns",
    "output_latency" : "20ns",
    "input_buf_size" : "4kB",
    "output_buf_size" : "4kB",
    # Set up the number of VNs to use; using 2 since one of the jobs
    # remaps to VN 1
    "num_vns" : 1,
    # Optionally set up the QOS
    #"qos_settings" : [50,50]

    # Set up the arbitration type for the routers
    "xbar_arb" : "merlin.xbar_arb_lru",
    
})

platdef.addClassType("router","sst.merlin.base.hr_router")

platdef.addParamSet("network_interface",{
    "link_bw" : "4GB/s",
    "input_buf_size" : "1kB",
    "output_buf_size" : "1kB"
})

#platdef.addClassType("network_interface","sst.merlin.base.ReorderLinkControl")
platdef.addClassType("network_interface","sst.merlin.interface.ReorderLinkControl")


platdef_cm = PlatformDefinition.compose("platform_dragon_128_cm",[("platform_dragon_128","ALL")])
platdef_cm.addParamSet("router",{"enable_congestion_management":True})
platdef_cm.addClassType("topology","sst.merlin.topology.topoDragonFly")
platdef_cm.addClassType("network_interface","sst.merlin.interface.ReorderLinkControl")
