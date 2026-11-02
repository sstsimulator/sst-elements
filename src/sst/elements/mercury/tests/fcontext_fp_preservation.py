# Copyright 2009-2026 NTESS. Under the terms
# of Contract DE-NA0003525 with NTESS, the U.S.
# Government retains certain rights in this software.

import sst
import sst.hg


sst.setProgramOption("timebase", "1ps")

for node_id in range(2):
    node = sst.Component(f"node{node_id}", "hg.Node")
    node.addParams({
        "logicalID": node_id,
        "nranks": 2,
        "npernode": 1,
    })

    mercury_os = node.setSubComponent("os_slot", "hg.OperatingSystem")
    mercury_os.addParams({
        "app1.name": "fcontext_fp_preservation",
        "app1.exe_library_name": "fcontext_fp_preservation",
    })

    loopback = sst.Link(f"node{node_id}_loopback")
    loopback.connect(
        (node, "network", "1ns"),
        (node, "network", "1ns"),
    )
