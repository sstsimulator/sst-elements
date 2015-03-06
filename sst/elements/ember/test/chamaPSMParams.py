
debug = 0

topology="fattree"
radix=18
loading=9

networkParams = {
    "topology" : "merlin." + "fattree",
    "packetSize" : "2048B",
    "link_bw" : "3.85GB/s",
    "flitSize" : "8B",
    "buffer_size" : "14KB",
}

nicParams = {
    "debug" : 0,
    "verboseLevel": 1,
    "module" : "merlin.linkcontrol",
    "topology" : networkParams['topology'], 
    "packetSize" : networkParams['packetSize'],
    "link_bw" : networkParams['link_bw'],
    "buffer_size" : networkParams['buffer_size'],
    "rxMatchDelay_ns" : 100,
    "txDelay_ns" : 50,
    "dmaBW_GBs" : 4.0,
    "dmaContentionMult" : 0.4,
}

emberParams = {
    "os.module"    : "firefly.hades",
    "os.name"      : "hermesParams",
    "api.0.module" : "firefly.hadesMP",
    "verbose" : 0,
}

hermesParams = {
    "hermesParams.debug" : 0,
    "hermesParams.verboseLevel" : 1,
    "hermesParams.nicModule" : "firefly.VirtNic",
    "hermesParams.nicParams.debug" : 0,
    "hermesParams.nicParams.debugLevel" : 1 ,
    "hermesParams.policy" : "adjacent",
    "hermesParams.functionSM.defaultEnterLatency" : 10000,
    "hermesParams.functionSM.defaultReturnLatency" : 10000,
    "hermesParams.functionSM.defaultDebug" : 0,
    "hermesParams.functionSM.defaultVerbose" : 2,
    "hermesParams.ctrlMsg.debug" : 0,
    "hermesParams.ctrlMsg.verboseLevel" : 2,
    "hermesParams.ctrlMsg.shortMsgLength" : 64000,
    "hermesParams.ctrlMsg.matchDelay_ns" : 150,

    "hermesParams.ctrlMsg.txSetupMod" : "firefly.LatencyMod",
    "hermesParams.ctrlMsg.txSetupModParams.range.0" : "0-15:160ns",
    "hermesParams.ctrlMsg.txSetupModParams.range.1" : "16-:330ns",

    "hermesParams.ctrlMsg.rxSetupMod" : "firefly.LatencyMod",
    "hermesParams.ctrlMsg.rxSetupModParams.range.0" : "0-:50ns",

    "hermesParams.ctrlMsg.txMemcpyMod" : "firefly.LatencyMod",
    "hermesParams.ctrlMsg.txMemcpyModParams.op" : "Linear",
    "hermesParams.ctrlMsg.txMemcpyModParams.range.0" : "0-63:0ps",
    "hermesParams.ctrlMsg.txMemcpyModParams.range.1" : "64-2047:400ps",
    "hermesParams.ctrlMsg.txMemcpyModParams.range.2" : "2048-4097:350ps",
    "hermesParams.ctrlMsg.txMemcpyModParams.range.3" : "4098-10000:200ps",
    "hermesParams.ctrlMsg.txMemcpyModParams.range.4" : "10001-:150ps",

    "hermesParams.ctrlMsg.rxMemcpyMod" : "firefly.LatencyMod",
    "hermesParams.ctrlMsg.rxMemcpyModParams.op" : "Linear",

    "hermesParams.ctrlMsg.txNicDelay_ns" : 0,
    "hermesParams.ctrlMsg.rxNicDelay_ns" : 0,
    "hermesParams.ctrlMsg.txFiniMod" : "firefly.LatencyMod",
    "hermesParams.ctrlMsg.txFiniModParams.range.0" : "0-:60ns",
    "hermesParams.ctrlMsg.rxFiniMod" : "firefly.LatencyMod",
    "hermesParams.ctrlMsg.rxFiniModParams.range.0" : "0-:120ns",
    "hermesParams.ctrlMsg.sendAckDelay_ns" : 1700,
    "hermesParams.ctrlMsg.regRegionBaseDelay_ns" : 8000,
    "hermesParams.ctrlMsg.regRegionPerPageDelay_ns" : 120,
    "hermesParams.ctrlMsg.regRegionXoverLength" : 4096,
    "hermesParams.loadMap.0.start" : 0,
    "hermesParams.loadMap.0.len" : 2,
}
