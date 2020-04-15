
debug = 0

netConfig = {
}

networkParams = {
    "packetSize" : "2048B",
    "link_bw" : "4GB/s",
    "xbar_bw" : "4GB/s",
    "link_lat" : "40ns",
    "input_latency" : "50ns",
    "output_latency" : "50ns",
    "flitSize" : "8B",
    "input_buf_size" : "14KB",
    "output_buf_size" : "14KB",
}

nicParams = {
	"detailedCompute.name" : "thornhill.SingleThread",
    "module" : "merlin.linkcontrol",
    "packetSize" : networkParams['packetSize'],
    "link_bw" : networkParams['link_bw'],
    "input_buf_size" : networkParams['input_buf_size'],
    "output_buf_size" : networkParams['output_buf_size'],
    "rxMatchDelay_ns" : 100,
    "txDelay_ns" : 50,
    "nic2host_lat" : "150ns",
    "useSimpleMemoryModel" : 0,
# simpleMemoryModel.verboseMask: values 
#define BUS_WIDGET_MASK 1<<1
#define CACHE_MASK      1<<2
#define LOAD_MASK       1<<3
#define MEM_MASK        1<<4
#define MUX_MASK        1<<5
#define STORE_MASK      1<<6
#define THREAD_MASK     1<<7
#define BUS_BRIDGE_MASK 1<<8
	"simpleMemoryModel.verboseLevel" : 0,
	"simpleMemoryModel.verboseMask" : -1,

	"simpleMemoryModel.memNumSlots" : 32,
	"simpleMemoryModel.memReadLat_ns" : 150, 
	"simpleMemoryModel.memWriteLat_ns" : 40, 

	"simpleMemoryModel.hostCacheUnitSize" : 32, 
	"simpleMemoryModel.hostCacheNumMSHR" : 32, 
	"simpleMemoryModel.hostCacheLineSize" : 64, 

	"simpleMemoryModel.widgetSlots" : 32, 

	"simpleMemoryModel.nicNumLoadSlots" : 16, 
	"simpleMemoryModel.nicNumStoreSlots" : 16, 

	"simpleMemoryModel.nicHostLoadSlots" : 1, 
	"simpleMemoryModel.nicHostStoreSlots" : 1, 

	"simpleMemoryModel.busBandwidth_Gbs" : 7.8,
	"simpleMemoryModel.busNumLinks" : 8,
	"simpleMemoryModel.detailedModel.name" : "firefly.detailedInterface",
	"maxRecvMachineQsize" : 100,
	"maxSendMachineQsize" : 100,

    #"numVNs" : 7,

    #"getHdrVN" : 1,
    #"getRespSmallVN" : 2,
    #"getRespLargeVN" : 3,
    #"getRespSize" : 15000,

    #"shmemAckVN": 1 ,
    #"shmemGetReqVN": 2,
    #"shmemGetLargeVN": 3,
    #"shmemGetSmallVN": 4,
    #"shmemGetThresholdLength": 8,
    #"shmemPutLargeVN": 5,
    #"shmemPutSmallVN": 6,
    #"shmemPutThresholdLength": 8,

}

emberParams = {
    "os.module"    : "firefly.hades",
    "os.name"      : "hermesParams",
    "api.0.module" : "firefly.hadesMP",
    "api.1.module" : "firefly.hadesSHMEM",
    "api.2.module" : "firefly.hadesMisc",
    'firefly.hadesSHMEM.verboseLevel' : 0,
    'firefly.hadesSHMEM.verboseMask'  : -1,
    'firefly.hadesSHMEM.enterLat_ns'  : 7,
    'firefly.hadesSHMEM.returnLat_ns' : 7,
    "verbose" : 0,
}

hermesParams = {
	"hermesParams.detailedCompute.name" : "thornhill.SingleThread",
	"hermesParams.memoryHeapLink.name" : "thornhill.MemoryHeapLink",
    "hermesParams.nicModule" : "firefly.VirtNic",

    "hermesParams.functionSM.defaultEnterLatency" : 30000,
    "hermesParams.functionSM.defaultReturnLatency" : 30000,

    #"hermesParams.functionSM.smallCollectiveVN" : 1,
    #"hermesParams.functionSM.smallCollectiveSize" : 8,

    #"hermesParams.ctrlMsg.rendezvousVN" : 1,
    #"hermesParams.ctrlMsg.ackVN" : 1,

    "hermesParams.ctrlMsg.shortMsgLength" : 12000,
    "hermesParams.ctrlMsg.matchDelay_ns" : 150,

    "hermesParams.ctrlMsg.txSetupMod" : "firefly.LatencyMod",
    "hermesParams.ctrlMsg.txSetupModParams.range.0" : "0-:130ns",

    "hermesParams.ctrlMsg.rxSetupMod" : "firefly.LatencyMod",
    "hermesParams.ctrlMsg.rxSetupModParams.range.0" : "0-:100ns",

    "hermesParams.ctrlMsg.txMemcpyMod" : "firefly.LatencyMod",
    "hermesParams.ctrlMsg.txMemcpyModParams.op" : "Mult",
    "hermesParams.ctrlMsg.txMemcpyModParams.range.0" : "0-:344ps",

    "hermesParams.ctrlMsg.rxMemcpyMod" : "firefly.LatencyMod",
    "hermesParams.ctrlMsg.txMemcpyModParams.op" : "Mult",
    "hermesParams.ctrlMsg.rxMemcpyModParams.range.0" : "0-:344ps",

    "hermesParams.ctrlMsg.sendAckDelay_ns" : 0,
    "hermesParams.ctrlMsg.regRegionBaseDelay_ns" : 3000,
    "hermesParams.ctrlMsg.regRegionPerPageDelay_ns" : 100,
    "hermesParams.ctrlMsg.regRegionXoverLength" : 4096,
    "hermesParams.loadMap.0.start" : 0,
    "hermesParams.loadMap.0.len" : 2,
}
