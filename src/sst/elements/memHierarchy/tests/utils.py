import sst
import os
try:
    import ConfigParser
except ImportError:
    import configparser as ConfigParser


def connect(name, c0, port0, c1, port1, latency):
    link = sst.Link(name)
    link.connect( (c0, port0, latency), (c1, port1, latency) )
    return link



class Config:
    def __init__(self, cfgFile, **kwargs):
        cp = ConfigParser.ConfigParser()
        if not cp.read(cfgFile):
            raise Exception('Unable to read file "%s"'%cfgFile)

        self.verbose = "verbose" in kwargs and kwargs["verbose"]

        self.clock = cp.get('CPU', 'clock')
        self.total_cores = cp.getint('CPU', 'num_cores')
        self.max_reqs_cycle = cp.get('CPU', 'max_reqs_cycle')

        self.memory_clock = cp.get('Memory', 'clock')
        self.memory_network_bandwidth = cp.get('Memory', 'network_bw')
        self.memory_capacity = cp.get('Memory', 'capacity')
        self.coherence_protocol = "MESI"

        self.num_ring_stops = self.total_cores + 1

        self.ring_latency = cp.get('Network', 'latency')
        self.ring_bandwidth = cp.get('Network', 'bandwidth')
        self.ring_flit_size = cp.get('Network', 'flit_size')

        self.app = cp.get('CPU', 'application')
        self.coreConfigParams = dict(cp.items(self.app))
        if self.app == 'miranda.STREAMBenchGeneratorCustomCmd':
            self.coreConfig = self._streamCoreConfig
        elif self.app == 'miranda.SPMVGenerator':
            self.coreConfig = self._SPMVCoreConfig
        elif self.app == 'miranda.RandomGenerator':
            self.coreConfig = self._randomCoreConfig
        elif self.app == 'miranda.GUPSGenerator':
            self.coreConfig = self._GUPSCoreConfig
        else:
            raise Exception("Unknown application '%s'"%app)

    def getCoreConfig(self, core_id):
        params = dict({
                'clock': self.clock,
                'verbose': int(self.verbose)
                })
        params.update(self.coreConfig(core_id))
        return params

    def _streamCoreConfig(self, core_id):
        streamN = int(self.coreConfigParams['total_streamn'])
        params = dict()
        params['max_reqs_cycle'] =  self.max_reqs_cycle
        params['generator'] = 'miranda.STREAMBenchGeneratorCustomCmd'
        params['generatorParams.n'] = streamN // self.total_cores
        params['generatorParams.start_a'] = ( (streamN * 32) // self.total_cores ) * core_id
        params['generatorParams.start_b'] = ( (streamN * 32) // self.total_cores ) * core_id + (streamN * 32)
        params['generatorParams.start_c'] = ( (streamN * 32) // self.total_cores ) * core_id + (2 * streamN * 32)
        params['generatorParams.operandwidth'] = 32
        params['generatorParams.verbose'] = int(self.verbose)
        params['generatorParams.write_cmd'] = 10
        return params

    def _GUPSCoreConfig(self, core_id):
        streamN = int(self.coreConfigParams['total_streamn'])
        params = dict()
        # params['max_reqs_cycle'] =  self.max_reqs_cycle
        params['generator'] = 'miranda.GUPSGenerator'
        params['generatorParams.count'] = streamN // self.total_cores
        params['generatorParams.seed_a'] = 11
        params['generatorParams.seed_b'] = 31
        params['generatorParams.length'] = 32
        params['generatorParams.iterations'] = 1
        params['generatorParams.verbose'] = int(self.verbose)
        params['generatorParams.issue_op_fences'] = "no"
        return params

    def _randomCoreConfig(self, core_id):
        streamN = int(self.coreConfigParams['total_streamn'])
        params = dict()
        # params['max_reqs_cycle'] =  self.max_reqs_cycle
        params['generator'] = 'miranda.RandomGenerator'
        params['generatorParams.count'] = streamN // self.total_cores
        params['generatorParams.max_address'] = 16384
        params['generatorParams.issue_op_fences'] = "no"
        params['generatorParams.length'] = 32
        params['generatorParams.verbose'] = int(self.verbose)
        params['generatorParams.write_cmd'] = 10
        return params

    def _SPMVCoreConfig(self, core_id):
        streamN = int(self.coreConfigParams['total_streamn'])
        params = dict()
        params['generatorParams.verbose'] = int(self.verbose)
        params['generator'] = 'miranda.SPMVGenerator'
        params['generatorParams.matrixnx'] = 10
        params['generatorParams.matrixny'] = 10
        params['generatorParams.element_width'] = 32
        params['generatorParams.lhs_start_addr'] = 0
        params['generatorParams.rhs_start_addr'] = 320
        params['generatorParams.local_row_start'] = 0
        params['generatorParams.local_row_end'] = 10
        params['generatorParams.ordinal_width'] = 8
        params['generatorParams.matrix_row_indices_start_addr'] = 0
        params['generatorParams.matrix_col_indices_start_addr'] = 0
        params['generatorParams.matrix_element_start_addr'] = 0
        params['generatorParams.iterations'] = streamN // self.total_cores
        params['generatorParams.matrix_nnz_per_row'] = 9
        return params

    def getL1Params(self):
        return dict({
            "prefetcher": "cassini.StridePrefetcher",
            "prefetcher.reach": 4,
            "detect_range" : 1,
            "cache_frequency": self.clock,
            "cache_size": "32KB",
            "associativity": 8,
            "access_latency_cycles": 4,
            "L1": 1,
            # Default params
            # "cache_line_size": 64,
            # "coherence_protocol": self.coherence_protocol,
            # "replacement_policy": "lru",
            # Not neccessary for simple cases:
            #"maxRequestDelay" : "1000000",
            })

    def getL2Params(self):
        return dict({
            "prefetcher": "cassini.StridePrefetcher",
            "prefetcher.reach": 16,
            "prefetcher.detect_range" : 1,
            "cache_frequency": self.clock,
            "cache_size": "256KB",
            "associativity": 8,
            "access_latency_cycles": 6,
            "mshr_num_entries" : 16,
            "memNIC.network_bw": self.ring_bandwidth,
            # Default params
            #"cache_line_size": 64,
            #"coherence_protocol": self.coherence_protocol,
            #"replacement_policy": "lru",
            })

    def getMemCtrlParams(self):
        return dict({
            "backing" : "none",
            "debug" : "0",
            "clock" : "1GHz",
            #"clock" : self.memory_clock,
            "customCmdHandler" : "memHierarchy.amoCustomCmdHandler",
            "addr_range_end" : (int(''.join(filter(str.isdigit, self.memory_capacity))) * 1024 * 1024) - 1,
            })
    def getMemParams(self):
        return dict({
            #"backend" : "memHierarchy.simpleMem",
            #"backend.access_time" : "30ns",
            #"memNIC.network_bw": self.ring_bandwidth,
            "mem_size" : self.memory_capacity,
            #"system_ini" : "system.ini",
            #"clock" : "1Ghz",
            #"backend.access_time" : "100 ns",
            #"backend.device_ini" : "HBMDevice.ini",
            #"backend.system_ini" : "HBMSystem.ini",
            #"backend.tracing" : "1",
            #"backend" : "memHierarchy.hbmdramsim",
            "clock" : self.memory_clock,
            "access_time" : "1000 ns",
            "verbose" : "0",
            "trace-banks" : "1",
            "trace-queue" : "1",
            "trace-cmds" : "1",
            "trace-latency" : "1",
            "trace-stalls" : "1",
            "cmd-map" : "[CUSTOM:10:64:WR64]"
            })

    def getDCParams(self, dc_id):
        return dict({
            "entry_cache_size": 256*1024*1024, #Entry cache size of mem/blocksize
            "clock": self.memory_clock,
            "memNIC.network_bw": self.ring_bandwidth,
            "debug" : 0,
            "debug_level" : 10,
            "memNIC.addr_range_start" : 0,
            "memNIC.addr_range_end" : (int(''.join(filter(str.isdigit, self.memory_capacity))) * 1024 * 1024),
            # Default params
            # "coherence_protocol": coherence_protocol,
            })

    def getRouterParams(self):
        return dict({
            "output_latency" : "25ps",
            "xbar_bw" : self.ring_bandwidth,
            "input_buf_size" : "2KB",
            "input_latency" : "25ps",
            "num_ports" : self.total_cores + 1,
            "flit_size" : self.ring_flit_size,
            "output_buf_size" : "2KB",
            "link_bw" : self.ring_bandwidth,
            "topology" : "merlin.singlerouter"
        })
