
debug = 0

netConfig = {
	"topology":	"torus",
	"shape":	"4x4x4x4x2"
	#shape="2x2x2"
}

networkParams = {
    "topology" : "merlin." + "torus",
    "packetSize" : "512B",
    "link_bw" : "1.77GB/s",
    "xbar_bw" : "1.77GB/s",
    "link_lat" : "40ns",
    "input_latency" : "10ns",
    "output_latency" : "10ns",
    "flitSize" : "8B",
    "input_buf_size" : "14KB",
    "output_buf_size" : "14KB",
}

nicParams = {
    "module" : "merlin.linkcontrol",
    "topology" : networkParams['topology'], 
    "packetSize" : networkParams['packetSize'],
    "link_bw" : networkParams['link_bw'],
    "input_buf_size" : networkParams['input_buf_size'],
    "output_buf_size" : networkParams['output_buf_size'],
    "rxMatchDelay_ns" : 100,
    "txDelay_ns" : 50,
    "nic2host_lat" : "150ns",
}

emberParams = {
    "os.module"    : "firefly.hades",
    "os.name"      : "hermesParams",
    "api.0.module" : "firefly.hadesMP",
    "verbose" : 0,
}

hermesParams = {
    "hermesParams.nicModule" : "firefly.VirtNic",

    "hermesParams.functionSM.defaultEnterLatency" : 30000,
    "hermesParams.functionSM.defaultReturnLatency" : 30000,

    "hermesParams.functionSM.Send.returnLatency" : 1300000,
    #"hermesParams.functionSM.Recv.returnLatency" : 1300000,
    "hermesParams.functionSM.WaitAll.returnLatency" : 1300000,
    "hermesParams.functionSM.Wait.returnLatency" : 1300000,

    "hermesParams.ctrlMsg.sendStateDelay_ps" : 1300000,
    "hermesParams.ctrlMsg.recvStateDelay_ps" : 1300000,
    "hermesParams.ctrlMsg.waitallStateDelay_ps" : 1300000,
    "hermesParams.ctrlMsg.waitanyStateDelay_ps" : 1300000,

    "hermesParams.ctrlMsg.shortMsgLength" : 4200,
    "hermesParams.ctrlMsg.matchDelay_ns" : 250,

    "hermesParams.ctrlMsg.txSetupMod" : "firefly.LatencyMod",
    "hermesParams.ctrlMsg.txSetupModParams.range.0" : "0-127:1100ns",
    "hermesParams.ctrlMsg.txSetupModParams.range.1" : "128-511:1850ns",
    "hermesParams.ctrlMsg.txSetupModParams.range.2" : "512-:2100ns",

    "hermesParams.ctrlMsg.rxPostMod" : "firefly.LatencyMod",
    "hermesParams.ctrlMsg.rxPostModParams.range.0" : "0-:300ns",

    "hermesParams.ctrlMsg.rxSetupMod" : "firefly.LatencyMod",
    "hermesParams.ctrlMsg.rxSetupModParams.range.0" : "0:150ns",
    "hermesParams.ctrlMsg.rxSetupModParams.range.1" : "1-127:530ns",
    "hermesParams.ctrlMsg.rxSetupModParams.range.2" : "128-511:1100ns",
    "hermesParams.ctrlMsg.rxSetupModParams.range.3" : "512-:2000ns",

    "hermesParams.ctrlMsg.txMemcpyMod" : "firefly.ScaleLatMod",
    "hermesParams.ctrlMsg.txMemcpyModParams.range.0" : "0-50000:450ps-80ps",

    "hermesParams.ctrlMsg.rxMemcpyMod" : "firefly.ScaleLatMod",
    "hermesParams.ctrlMsg.rxMemcpyModParams.range.0" : "0-50000:450ps-100ps",

    "hermesParams.ctrlMsg.txFiniMod" : "firefly.LatencyMod",
    "hermesParams.ctrlMsg.txFiniModParams.range.0" : "0-127:400ns",
    "hermesParams.ctrlMsg.txFiniModParams.range.1" : "128-:900ns",

    "hermesParams.ctrlMsg.rxFiniMod" : "firefly.LatencyMod",
    "hermesParams.ctrlMsg.rxFiniModParams.range.0" : "0:70ns",
    "hermesParams.ctrlMsg.rxFiniModParams.range.1" : "1-127-:80ns",
    "hermesParams.ctrlMsg.rxFiniModParams.range.2" : "128-:100ns",

    "hermesParams.ctrlMsg.sendAckDelay_ns" : 0,

    "hermesParams.ctrlMsg.regRegionBaseDelay_ns" : 0,
    "hermesParams.ctrlMsg.regRegionPerPageDelay_ns" : 0,
    "hermesParams.ctrlMsg.regRegionXoverLength" : 4096,
    "hermesParams.loadMap.0.start" : 0,
    "hermesParams.loadMap.0.len" : 2,
}
