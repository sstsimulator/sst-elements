
debug = 0

netConfig = {
    "topology": "",
    "shape":    ""
}

networkParams = {
    "topology" : netConfig['topology'],
    "packetSize" : "2048B",
    "link_bw" : "125GB/s",
    #"link_bw" : "42.5GB/s",
    #"link_bw" : "5GB/s",
    "link_lat" : "30ns",
    "input_latency" : "30ns",
    "output_latency" : "30ns",
    "flitSize" : "64B",
    "buffer_size" : "28KB",
}

nicParams = {
    "module" : "merlin.linkcontrol",
    "topology" : networkParams['topology'], 
    "packetSize" : networkParams['packetSize'],
    "link_bw" : networkParams['link_bw'],
    "buffer_size" : networkParams['buffer_size'],
    "rxMatchDelay_ns" : 100,
    "txDelay_ns" : 50,
    "hostReadDelay_ns" : 200,
    "tracedNode" : 100,
    "tracedPkt" : -1,
    "nic2host_lat" : "1ns",
}

emberParams = {
    "os.module"    : "firefly.hades",
    "os.name"      : "hermesParams",
    "api.0.module" : "firefly.hadesMP",
    "verbose" : 0,
}

hermesParams = {
    "hermesParams.nicModule" : "firefly.VirtNic",

    "hermesParams.functionSM.defaultEnterLatency" : 10000,
    "hermesParams.functionSM.defaultReturnLatency" : 10000,

    "hermesParams.ctrlMsg.shortMsgLength" : 64000,
    "hermesParams.ctrlMsg.matchDelay_ns" : 150,

    "hermesParams.ctrlMsg.rxPostMod" : "firefly.LatencyMod",
    "hermesParams.ctrlMsg.rxPostModParams.range.0" : "0-:40ns",

    "hermesParams.ctrlMsg.txSetupMod" : "firefly.LatencyMod",
    "hermesParams.ctrlMsg.txSetupModParams.range.0" : "0-15:110ns",
    "hermesParams.ctrlMsg.txSetupModParams.range.1" : "16-:140ns",

    "hermesParams.ctrlMsg.rxSetupMod" : "firefly.LatencyMod",
    "hermesParams.ctrlMsg.rxSetupModParams.range.0" : "0-15:120ns",
    "hermesParams.ctrlMsg.rxSetupModParams.range.1" : "16-:260ns",

    "hermesParams.ctrlMsg.txMemcpyMod" : "firefly.ScaleLatMod",
    "hermesParams.ctrlMsg.txMemcpyModParams.range.0" : "0-50000:250ps-80ps",

    "hermesParams.ctrlMsg.rxMemcpyMod" : "firefly.ScaleLatMod",
    "hermesParams.ctrlMsg.rxMemcpyModParams.range.0" : "0-50000:250ps-100ps",

    "hermesParams.ctrlMsg.txFiniMod" : "firefly.LatencyMod",
    "hermesParams.ctrlMsg.txFiniModParams.range.0" : "0-:100ns",
    "hermesParams.ctrlMsg.rxFiniMod" : "firefly.LatencyMod",
    "hermesParams.ctrlMsg.rxFiniModParams.range.0" : "0-:80ns",
    "hermesParams.ctrlMsg.sendAckDelay_ns" : 1700,
    #"hermesParams.ctrlMsg.regRegionBaseDelay_ns" : 8000,
    #"hermesParams.ctrlMsg.regRegionPerPageDelay_ns" : 120,
    #"hermesParams.ctrlMsg.regRegionXoverLength" : 4096,
    "hermesParams.loadMap.0.start" : 0,
    "hermesParams.loadMap.0.len" : 2,
}
