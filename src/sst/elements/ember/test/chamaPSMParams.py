
debug = 0

netConfig = {
	'topology': 'fattree',
	'shape' : '9,9:9,9:18'
	#'radix':    18,
	#'loading':  9.
	#'shape' : '18,18:18,18:36'
}

networkParams = {
    "topology" : "merlin." + "fattree",
    "packetSize" : "2048B",
    "link_bw" : "3.85GB/s",
    "link_lat" : "120ns",
    "input_latency" : "50ns",
    "output_latency" : "50ns",
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
    "dmaBW_GBs" : 4.0,
    "dmaContentionMult" : 0.4,
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
    "hermesParams.ctrlMsg.regRegionBaseDelay_ns" : 8000,
    "hermesParams.ctrlMsg.regRegionPerPageDelay_ns" : 120,
    "hermesParams.ctrlMsg.regRegionXoverLength" : 4096,
    "hermesParams.loadMap.0.start" : 0,
    "hermesParams.loadMap.0.len" : 2,
}
