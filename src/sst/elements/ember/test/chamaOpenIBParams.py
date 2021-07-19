
debug = 0

netConfig = {
	"topology":	"fattree",
	'shape' : '9,9:9,9:18'
	#"radix":	18,
	#"loading":	9,
}

networkParams = {
    "topology" : "merlin." + "torus",
    "packetSize" : "2048B",
    "link_bw" : "3.85GB/s",
    "link_lat" : "120ns",
    "input_latency" : "50ns",
    "output_latency" : "50ns",
    "flitSize" : "8B",
    "buffer_size" : "14KB",
}

nicParams = {
    "module" : "merlin.linkcontrol",
    "topology" : networkParams['topology'], 
    "packetSize" : networkParams['packetSize'],
    "link_bw" : networkParams['link_bw'],
    "buffer_size" : networkParams['buffer_size'],
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

    "hermesParams.ctrlMsg.shortMsgLength" : 12000,
    "hermesParams.ctrlMsg.matchDelay_ns" : 150,

    "hermesParams.ctrlMsg.txSetupMod" : "firefly.LatencyMod",
    "hermesParams.ctrlMsg.txSetupModParams.range.0" : "0-:1000ns",

    "hermesParams.ctrlMsg.rxSetupMod" : "firefly.LatencyMod",
    "hermesParams.ctrlMsg.rxSetupModParams.range.0" : "0-:1000ns",

    "hermesParams.ctrlMsg.txMemcpyMod" : "firefly.LatencyMod",
    "hermesParams.ctrlMsg.txMemcpyModParams.op" : "Mult",
    "hermesParams.ctrlMsg.txMemcpyModParams.base" : "80ns",
    "hermesParams.ctrlMsg.txMemcpyModParams.range.0" : "0-1023:1ps",
    "hermesParams.ctrlMsg.txMemcpyModParams.range.1" : "1024-2047:200ps",
    "hermesParams.ctrlMsg.txMemcpyModParams.range.2" : "2048-:390ps",

    "hermesParams.ctrlMsg.rxMemcpyMod" : "firefly.LatencyMod",
    "hermesParams.ctrlMsg.rxMemcpyModParams.op" : "Mult",
    "hermesParams.ctrlMsg.rxMemcpyModParams.base" : "80ns",
    "hermesParams.ctrlMsg.rxMemcpyModParams.range.0" : "0-:50ps",

    "hermesParams.ctrlMsg.sendReqFiniDelay_ns" : 700,
    "hermesParams.ctrlMsg.recvReqFiniDelay_ns" : 700,
    "hermesParams.ctrlMsg.sendAckDelay_ns" : 1700,
    "hermesParams.ctrlMsg.regRegionBaseDelay_ns" : 3800,
    "hermesParams.ctrlMsg.regRegionPerPageDelay_ns" : 120,
    "hermesParams.ctrlMsg.regRegionXoverLength" : 4096,
    "hermesParams.loadMap.0.start" : 0,
    "hermesParams.loadMap.0.len" : 2,
}
