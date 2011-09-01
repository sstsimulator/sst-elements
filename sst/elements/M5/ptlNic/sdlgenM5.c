#include <stdio.h>
#include <assert.h>

static foo( FILE* output, int nid, const char* exe );

void sdlgenM5( const char* file, const char* exe, int numM5Nids )
{
    FILE* output = fopen( file, "w" );
    char* indent="";

    assert( output );

    fprintf(output,"<?xml version=\"2.0\"?>\n");
    fprintf(output,"\n");

    fprintf(output,"<variables>\n");
    fprintf(output,"    <lat> 10ns </lat>\n");
    fprintf(output,"</variables>\n");
    fprintf(output,"\n");

    fprintf(output,"<param_include>\n");
    fprintf(output,"%s<cpuParams>\n",indent);
    fprintf(output,"%s    <physicalMemory.start> 0x00100000 </physicalMemory.start>\n",indent);
    fprintf(output,"%s    <physicalMemory.end>   0x1fffffff </physicalMemory.end>\n",indent);
    fprintf(output,"%s    <base.process.registerExit> yes </base.process.registerExit>\n",indent);
    fprintf(output,"%s\n",indent);

    fprintf(output,"%s    <base.process.egid> 1 </base.process.egid>\n",indent);
    fprintf(output,"%s    <base.process.euid> 1 </base.process.euid>\n",indent);
    fprintf(output,"%s    <base.process.gid> 1 </base.process.gid>\n",indent);
    fprintf(output,"%s    <base.process.pid> 1 </base.process.pid>\n",indent);
    fprintf(output,"%s    <base.process.ppid> 1 </base.process.ppid>\n",indent);
    fprintf(output,"%s    <base.process.uid> 1 </base.process.uid>\n",indent); 
    fprintf(output,"%s    <base.process.cwd> / </base.process.cwd>\n",indent);
    fprintf(output,"%s    <base.process.executable> %s </base.process.executable>\n",indent,exe);
    fprintf(output,"%s    <base.process.simpoint> 0 </base.process.simpoint>\n",indent);
    fprintf(output,"%s    <base.process.errout> cerr </base.process.errout>\n",indent);
    fprintf(output,"%s    <base.process.input> cin </base.process.input>\n",indent);
    fprintf(output,"%s    <base.process.output> cout </base.process.output>\n",indent);
    fprintf(output,"%s    <base.process.max_stack_size> 0x4000000 </base.process.max_stack_size>\n",indent);
    fprintf(output,"%s\n",indent);

    fprintf(output,"%s    <o3cpu.fetchTrapLatency> 1 </o3cpu.fetchTrapLatency>\n",indent);
    fprintf(output,"%s    <o3cpu.trapLatency> 13 </o3cpu.trapLatency>\n",indent);
    fprintf(output,"%s    <o3cpu.smtIQThreshold> 100 </o3cpu.smtIQThreshold>\n",indent);
    fprintf(output,"%s    <o3cpu.smtLSQThreshold> 100 </o3cpu.smtLSQThreshold>\n",indent);
    fprintf(output,"%s    <o3cpu.smtROBThreshold> 100 </o3cpu.smtROBThreshold>\n",indent);
    fprintf(output,"%s    <o3cpu.predType> tournament </o3cpu.predType>\n",indent);
    fprintf(output,"%s    <o3cpu.smtCommitPolicy> RoundRobin </o3cpu.smtCommitPolicy>\n",indent);
    fprintf(output,"%s    <o3cpu.smtFetchPolicy> SingleThread </o3cpu.smtFetchPolicy>\n",indent);
    fprintf(output,"%s    <o3cpu.smtIQPolicy> Partitioned </o3cpu.smtIQPolicy>\n",indent);
    fprintf(output,"%s    <o3cpu.smtLSQPolicy> Partitioned </o3cpu.smtLSQPolicy>\n",indent);
    fprintf(output,"%s    <o3cpu.smtROBPolicy> Partitioned </o3cpu.smtROBPolicy>\n",indent);
    fprintf(output,"%s\n",indent);

    fprintf(output,"%s    <o3cpu.BTBEntries> 4096</o3cpu.BTBEntries>\n",indent);
    fprintf(output,"%s    <o3cpu.BTBTagSize> 16 </o3cpu.BTBTagSize>\n",indent);
    fprintf(output,"%s    <o3cpu.LFSTSize> 1024 </o3cpu.LFSTSize>\n",indent);
    fprintf(output,"%s    <o3cpu.LQEntries> 32 </o3cpu.LQEntries>\n",indent);
    fprintf(output,"%s    <o3cpu.RASSize> 16 </o3cpu.RASSize>\n",indent);
    fprintf(output,"%s    <o3cpu.SQEntries> 32 </o3cpu.SQEntries>\n",indent);
    fprintf(output,"%s    <o3cpu.SSITSize> 1024 </o3cpu.SSITSize>\n",indent);
    fprintf(output,"%s    <o3cpu.activity> 0 </o3cpu.activity>\n",indent);
    fprintf(output,"%s    <o3cpu.backComSize> 5 </o3cpu.backComSize>\n",indent);
    fprintf(output,"%s    <o3cpu.cachePorts> 200 </o3cpu.cachePorts>\n",indent);
    fprintf(output,"%s    <o3cpu.choiceCtrBits> 2 </o3cpu.choiceCtrBits>\n",indent);
    fprintf(output,"%s    <o3cpu.choicePredictorSize> 8192 </o3cpu.choicePredictorSize>\n",indent);
    fprintf(output,"%s\n",indent);

    fprintf(output,"%s    <o3cpu.commitToDecodeDelay> 1 </o3cpu.commitToDecodeDelay>\n",indent);
    fprintf(output,"%s    <o3cpu.commitToFetchDelay> 1 </o3cpu.commitToFetchDelay>\n",indent);
    fprintf(output,"%s    <o3cpu.commitToIEWDelay> 1 </o3cpu.commitToIEWDelay>\n",indent);
    fprintf(output,"%s    <o3cpu.commitToRenameDelay> 1 </o3cpu.commitToRenameDelay>\n",indent);
    fprintf(output,"%s    <o3cpu.commitWidth> 8 </o3cpu.commitWidth>\n",indent);
    fprintf(output,"%s    <o3cpu.decodeToFetchDelay> 1 </o3cpu.decodeToFetchDelay>\n",indent);
    fprintf(output,"%s    <o3cpu.decodeToRenameDelay> 1 </o3cpu.decodeToRenameDelay>\n",indent);
    fprintf(output,"%s    <o3cpu.decodeWidth> 8 </o3cpu.decodeWidth>\n",indent);
    fprintf(output,"%s    <o3cpu.dispatchWidth> 8 </o3cpu.dispatchWidth>\n",indent);
    fprintf(output,"%s    <o3cpu.fetchToDecodeDelay> 1 </o3cpu.fetchToDecodeDelay>\n",indent);
    fprintf(output,"%s    <o3cpu.fetchWidth> 8 </o3cpu.fetchWidth>\n",indent);
    fprintf(output,"%s    <o3cpu.forwardComSize> 5 </o3cpu.forwardComSize>\n",indent);
    fprintf(output,"%s    <o3cpu.globalCtrBits> 2 </o3cpu.globalCtrBits>\n",indent);
    fprintf(output,"%s    <o3cpu.globalHistoryBits> 13 </o3cpu.globalHistoryBits>\n",indent);
    fprintf(output,"%s    <o3cpu.globalPredictorSize> 8192 </o3cpu.globalPredictorSize>\n",indent);
    fprintf(output,"%s\n",indent);

    fprintf(output,"%s    <o3cpu.iewToCommitDelay> 1 </o3cpu.iewToCommitDelay>\n",indent);
    fprintf(output,"%s    <o3cpu.iewToDecodeDelay> 1 </o3cpu.iewToDecodeDelay>\n",indent);
    fprintf(output,"%s    <o3cpu.iewToFetchDelay> 1 </o3cpu.iewToFetchDelay>\n",indent);
    fprintf(output,"%s    <o3cpu.iewToRenameDelay> 1 </o3cpu.iewToRenameDelay>\n",indent);
    fprintf(output,"%s    <o3cpu.instShiftAmt> 2 </o3cpu.instShiftAmt>\n",indent);
    fprintf(output,"%s    <o3cpu.issueToExecuteDelay> 1 </o3cpu.issueToExecuteDelay>\n",indent);
    fprintf(output,"%s    <o3cpu.issueWidth> 8 </o3cpu.issueWidth>\n",indent);
    fprintf(output,"%s    <o3cpu.localCtrBits> 2 </o3cpu.localCtrBits>\n",indent);
    fprintf(output,"%s    <o3cpu.localHistoryBits> 11 </o3cpu.localHistoryBits>\n",indent);
    fprintf(output,"%s    <o3cpu.localHistoryTableSize> 2048 </o3cpu.localHistoryTableSize>\n",indent);
    fprintf(output,"%s    <o3cpu.localPredictorSize> 2048 </o3cpu.localPredictorSize>\n",indent);
    fprintf(output,"%s    <o3cpu.numIQEntries> 64 </o3cpu.numIQEntries>\n",indent);
    fprintf(output,"%s    <o3cpu.numPhysFloatRegs> 256 </o3cpu.numPhysFloatRegs>\n",indent);
    fprintf(output,"%s    <o3cpu.numPhysIntRegs> 256 </o3cpu.numPhysIntRegs>\n",indent);
    fprintf(output,"%s    <o3cpu.numROBEntries> 192 </o3cpu.numROBEntries>\n",indent);
    fprintf(output,"%s    <o3cpu.numRobs> 1 </o3cpu.numRobs>\n",indent);
    fprintf(output,"%s    <o3cpu.renameToDecodeDelay> 1 </o3cpu.renameToDecodeDelay>\n",indent);
    fprintf(output,"%s    <o3cpu.renameToFetchDelay> 1 </o3cpu.renameToFetchDelay>\n",indent);
    fprintf(output,"%s    <o3cpu.renameToIEWDelay> 2 </o3cpu.renameToIEWDelay>\n",indent);
    fprintf(output,"%s    <o3cpu.renameToROBDelay> 1 </o3cpu.renameToROBDelay>\n",indent);
    fprintf(output,"%s    <o3cpu.renameWidth> 8 </o3cpu.renameWidth>\n",indent);
    fprintf(output,"%s    <o3cpu.smtNumFetchingThreads> 1 </o3cpu.smtNumFetchingThreads>\n",indent);
    fprintf(output,"%s    <o3cpu.squashWidth> 8 </o3cpu.squashWidth>\n",indent);
    fprintf(output,"%s    <o3cpu.wbDepth> 1 </o3cpu.wbDepth>\n",indent);
    fprintf(output,"%s    <o3cpu.wbWidth> 8 </o3cpu.wbWidth>\n",indent);
    fprintf(output,"\n");

    fprintf(output,"%s    <base.dtb.size> 64 </base.dtb.size>\n",indent);
    fprintf(output,"%s    <base.itb.size> 48 </base.itb.size>\n",indent);
    fprintf(output,"%s    <base.max_insts_all_threads> 0 </base.max_insts_all_threads>\n",indent);
    fprintf(output,"%s    <base.max_insts_any_thread> 0 </base.max_insts_any_thread>\n",indent);
    fprintf(output,"%s    <base.max_loads_all_threads> 0 </base.max_loads_all_threads>\n",indent);
    fprintf(output,"%s    <base.max_loads_any_thread> 0 </base.max_loads_any_thread>\n",indent);
    fprintf(output,"%s    <base.clock> 1000 </base.clock>\n",indent);
    fprintf(output,"%s    <base.function_trace_start> 0 </base.function_trace_start>\n",indent);
    fprintf(output,"%s    <base.phase> 0 </base.phase>\n",indent);
    fprintf(output,"%s    <base.progress_interval> 0 </base.progress_interval>\n",indent);
    fprintf(output,"%s    <base.defer_registration> 0 </base.defer_registration>\n",indent); 
    fprintf(output,"%s    <base.do_checkpoint_insts> 1 </base.do_checkpoint_insts>\n",indent);
    fprintf(output,"%s    <base.do_statistics_insts> 1 </base.do_statistics_insts>\n",indent);
    fprintf(output,"%s    <base.function_trace> 0 </base.function_trace>\n",indent);
    fprintf(output,"%s    <base.id> 0 </base.id>\n",indent);
    fprintf(output,"%s</cpuParams>\n",indent);
    fprintf(output,"%s\n",indent);

    fprintf(output,"%s<cacheParams>\n",indent);
    fprintf(output,"%s\n",indent);

    fprintf(output,"%s    <trace_addr> 0 </trace_addr>\n",indent);
    fprintf(output,"%s    <max_miss_count> 0 </max_miss_count>\n",indent);
    fprintf(output,"%s    <prefetch_policy> none </prefetch_policy>\n",indent);
    fprintf(output,"%s    <addr_range.start> 0 </addr_range.start>\n",indent);
    fprintf(output,"%s    <addr_range.end> -1 </addr_range.end>\n",indent);
    fprintf(output,"%s    <latency> 1000 </latency>\n",indent);
    fprintf(output,"%s    <prefetch_latency> 10000 </prefetch_latency>\n",indent);
    fprintf(output,"%s    <forward_snoops> true </forward_snoops>\n",indent);
    fprintf(output,"%s    <prefetch_cache_check_push> true </prefetch_cache_check_push>\n",indent);
    fprintf(output,"%s    <prefetch_data_accesses_only> false </prefetch_data_accesses_only>\n",indent);
    fprintf(output,"%s    <prefetch_on_access> false </prefetch_on_access>\n",indent);
    fprintf(output,"%s    <prefetch_past_page> false </prefetch_past_page>\n",indent);
    fprintf(output,"%s    <prefetch_serial_squash> false </prefetch_serial_squash>\n",indent);
    fprintf(output,"%s    <prefetch_use_cpu_id> true </prefetch_use_cpu_id>\n",indent);
    fprintf(output,"%s    <prioritizeRequests> false </prioritizeRequests>\n",indent);
    fprintf(output,"%s    <two_queue> false </two_queue>\n",indent);
    fprintf(output,"%s    <assoc> 2 </assoc>\n",indent);
    fprintf(output,"%s    <block_size> 64 </block_size>\n",indent);
    fprintf(output,"%s    <hash_delay> 1 </hash_delay>\n",indent);
    fprintf(output,"%s    <mshrs> 10 </mshrs>\n",indent);
    fprintf(output,"%s    <prefetch_degree> 1  </prefetch_degree>\n",indent);
    fprintf(output,"%s    <prefetcher_size> 100 </prefetcher_size>\n",indent);
    fprintf(output,"%s    <subblock_size> 0 </subblock_size>\n",indent);
    fprintf(output,"%s    <tgts_per_mshr> 5 </tgts_per_mshr>\n",indent);
    fprintf(output,"%s    <write_buffers> 8 </write_buffers>\n",indent);
    fprintf(output,"%s    <size> 0x8000 </size>\n",indent);
    fprintf(output,"%s\n",indent);


    fprintf(output,"%s</cacheParams>\n",indent);
    fprintf(output,"%s\n",indent);

    fprintf(output,"%s<busParams>\n",indent);
    fprintf(output,"%s    <clock> 1000 </clock>\n",indent);
    fprintf(output,"%s    <responder_set> false </responder_set>\n",indent);
    fprintf(output,"%s    <block_size> 64 </block_size>\n",indent);
    fprintf(output,"%s    <bus_id> 0 </bus_id>\n",indent);
    fprintf(output,"%s    <header_cycles> 1 </header_cycles>\n",indent);
    fprintf(output,"%s    <width> 64 </width>\n",indent);
    fprintf(output,"%s</busParams>\n",indent);
    fprintf(output,"%s\n",indent);

    fprintf(output,"%s<bridgeParams>\n",indent);
    fprintf(output,"%s    <delay> 0 </delay>\n",indent);
    fprintf(output,"%s    <nack_delay> 0 </nack_delay>\n",indent);
    fprintf(output,"%s    <write_ack> false </write_ack>\n",indent);
    fprintf(output,"%s    <req_size_a> 16 </req_size_a>\n",indent);
    fprintf(output,"%s    <req_size_b> 16 </req_size_b>\n",indent);
    fprintf(output,"%s    <resp_size_a> 16 </resp_size_a>\n",indent);
    fprintf(output,"%s    <resp_size_b> 16 </resp_size_b>\n",indent);
    fprintf(output,"%s</bridgeParams>\n",indent);
    fprintf(output,"%s\n",indent);

    fprintf(output,"%s</param_include>\n",indent);
    fprintf(output,"%s\n",indent);

    fprintf(output,"<sst>\n");
    fprintf(output,"%s\n",indent);

    int i;
    for ( i = 0; i < numM5Nids; i++ ) {
        foo( output, i, exe ); 
        fprintf(output,"\n");
    }

    fprintf(output,"\n");
    fprintf(output,"</sst>\n");
}

static foo( FILE* output, int nid, const char* exe )
{
    char* indent = "    ";

    // CPU
    fprintf(output,"%s<component name=nid%d.cpu0 type=O3Cpu >\n",indent,nid);
    fprintf(output,"%s    <params include=cpuParams>\n",indent);
    fprintf(output,"%s    </params>\n",indent);
    fprintf(output,"%s    <link name=nid%d.cpu-dcache port=dcache_port latency=$lat/>\n",indent,nid);
    fprintf(output,"%s    <link name=nid%d.cpu-icache port=icache_port latency=$lat/>\n",indent,nid);
    fprintf(output,"%s</component>\n",indent);

    fprintf(output,"\n");

    // D-Cache
    fprintf(output,"%s<component name=nid%d.cpu0.dcache type=BaseCache >\n",indent,nid);
    fprintf(output,"%s    <params include=cacheParams>\n",indent);
    fprintf(output,"%s        <size>65536</size>\n",indent);
    fprintf(output,"%s    </params>\n",indent);
    fprintf(output,"%s    <link name=nid%d.cpu-dcache port=cpu_side latency=$lat />\n",indent,nid);
    fprintf(output,"%s    <link name=nid%d.dcache-bus port=mem_side latency=$lat />\n",indent,nid);
    fprintf(output,"%s</component>\n",indent);

    fprintf(output,"\n");

    // I-Cache
    fprintf(output,"%s<component name=nid%d.cpu0.icache type=BaseCache >\n",indent,nid);
    fprintf(output,"%s    <params include=cacheParams>\n",indent);
    fprintf(output,"%s        <size>32768</size>\n",indent);
    fprintf(output,"%s    </params>\n",indent);
    fprintf(output,"%s    <link name=nid%d.cpu-icache port=cpu_side latency=$lat />\n",indent,nid);
    fprintf(output,"%s    <link name=nid%d.icache-bus port=mem_side latency=$lat />\n",indent,nid);
    fprintf(output,"%s</component>\n",indent);

    fprintf(output,"\n");

    // L2-Cache
    fprintf(output,"%s<component name=nid%d.L2cache type=BaseCache >\n",indent,nid);
    fprintf(output,"%s    <params include=cacheParams>\n",indent);
    fprintf(output,"%s        <size>2097152</size>\n",indent);
    fprintf(output,"%s        <assoc> 8 </assoc>\n",indent);
    fprintf(output,"%s        <latency> 10000 </latency>\n",indent);
    fprintf(output,"%s        <mshrs> 20 </mshrs>\n",indent);
    fprintf(output,"%s        <tgts_per_mshr> 12 </tgts_per_mshr>\n",indent);
    fprintf(output,"%s    </params>\n",indent);
    fprintf(output,"%s    <link name=nid%d.L2cache-L2Bus port=cpu_side latency=$lat />\n",indent,nid);
    fprintf(output,"%s    <link name=nid%d.L2cache-MemBus port=mem_side latency=$lat />\n",indent,nid);
    fprintf(output,"%s</component>\n",indent);

    fprintf(output,"\n");

    // L2 Bus
    fprintf(output,"%s<component name=nid%d.L2Bus type=Bus >\n",indent,nid);
    fprintf(output,"%s    <params include=busParams>\n",indent);
    fprintf(output,"%s    </params>\n",indent);
    fprintf(output,"%s    <link name=nid%d.L2cache-L2Bus port=port latency=$lat />\n",indent,nid);
    fprintf(output,"%s    <link name=nid%d.dcache-bus port=port latency=$lat />\n",indent,nid);
    fprintf(output,"%s    <link name=nid%d.icache-bus port=port latency=$lat />\n",indent,nid);
    fprintf(output,"%s</component>\n",indent);

    fprintf(output,"\n");

    // Memory Bus
    fprintf(output,"%s<component name=nid%d.MemBus type=Bus >\n",indent,nid);
    fprintf(output,"%s    <params include=busParams>\n",indent);
    fprintf(output,"%s    </params>\n",indent);
    fprintf(output,"%s    <link name=nid%d.Bridge-MemBus port=port latency=$lat />\n",indent,nid);
    fprintf(output,"%s    <link name=nid%d.IOcache-MemBus port=port latency=$lat />\n",indent,nid);
    fprintf(output,"%s    <link name=nid%d.L2cache-MemBus port=port latency=$lat />\n",indent,nid);
    fprintf(output,"%s    <link name=nid%d.mem-bus port=port latency=$lat />\n",indent,nid);
    fprintf(output,"%s</component>\n",indent);

    fprintf(output,"\n");

    // MemBus-IOBus Bridge

    fprintf(output,"%s<component name=nid%d.Bridge type=Bridge >\n",indent,nid);
    fprintf(output,"%s    <params include=bridgeParams>\n",indent);
    fprintf(output,"%s        <size>1024</size>\n",indent);
    fprintf(output,"%s    </params>\n",indent);
    fprintf(output,"%s    <link name=nid%d.Bridge-IOBus port=side_a latency=$lat />\n",indent,nid);
    fprintf(output,"%s    <link name=nid%d.Bridge-MemBus port=side_b latency=$lat />\n",indent,nid);
    fprintf(output,"%s</component>\n",indent);

    fprintf(output,"\n");

    // IO-Cache
    fprintf(output,"%s<component name=nid%d.cpu0.IOcache type=BaseCache >\n",indent,nid);
    fprintf(output,"%s    <params include=cacheParams>\n",indent);
    fprintf(output,"%s        <size>1024</size>\n",indent);
    fprintf(output,"%s        <assoc> 8 </assoc>\n",indent);
    fprintf(output,"%s        <latency> 10000 </latency>\n",indent);
    fprintf(output,"%s        <mshrs> 20 </mshrs>\n",indent);
    fprintf(output,"%s        <tgts_per_mshr> 12 </tgts_per_mshr>\n",indent);
    fprintf(output,"%s        <forward_snoops> false </forward_snoops>\n",indent);
    fprintf(output,"%s        <addr_range.start> 0x00100000 </addr_range.start>\n",indent);
    fprintf(output,"%s        <addr_range.end>   0xffffffff </addr_range.end>\n",indent);
    fprintf(output,"%s    </params>\n",indent);
    fprintf(output,"%s    <link name=nid%d.IOcache-MemBus port=mem_side latency=$lat />\n",indent,nid);
    fprintf(output,"%s    <link name=nid%d.IOCache-IOBus port=cpu_side latency=$lat />\n",indent,nid);
    fprintf(output,"%s</component>\n",indent);

    fprintf(output,"\n");

    // IO Bus
    fprintf(output,"%s<component name=nid%d.IOBus type=Bus >\n",indent,nid);
    fprintf(output,"%s    <params include=busParams>\n",indent);
    fprintf(output,"%s    </params>\n",indent);
    fprintf(output,"%s    <link name=nid%d.Bridge-IOBus port=port latency=$lat />\n",indent,nid);
    fprintf(output,"%s    <link name=nid%d.IOCache-IOBus port=port latency=$lat />\n",indent,nid);
    fprintf(output,"%s    <link name=nid%d.bus-syscallDMA port=port latency=$lat />\n",indent,nid);
    fprintf(output,"%s    <link name=nid%d.bus-syscallPIO port=port latency=$lat />\n",indent,nid);
    fprintf(output,"%s    <link name=nid%d.bus-nicDMA port=port latency=$lat />\n",indent,nid);
    fprintf(output,"%s    <link name=nid%d.bus-nicPIO port=port latency=$lat />\n",indent,nid);
    fprintf(output,"%s</component>\n",indent);

    fprintf(output,"\n");

    // Physical Memory
    fprintf(output,"%s<component name=nid%d.physmem type=PhysicalMemory >\n",indent,nid);
    fprintf(output,"%s    <params>\n",indent);
    fprintf(output,"%s        <range.start> 0x00100000 </range.start>\n",indent);
    fprintf(output,"%s        <range.end>   0xffffffff </range.end>\n",indent);

    fprintf(output,"%s        <latency> 28000 </latency>\n",indent);
    fprintf(output,"%s        <latency_var> 0 </latency_var>\n",indent);
    fprintf(output,"%s        <null> false </null>\n",indent);
    fprintf(output,"%s        <zero> false </zero>\n",indent);

    fprintf(output,"%s        <exe.0.process.executable> %s </exe.0.process.executable>\n",indent,exe);
    fprintf(output,"%s        <exe.0.process.cmd.0> hello  </exe.0.process.cmd.0>\n",indent);
    fprintf(output,"%s        <exe.0.process.env.0> PTLNIC_CMD_QUEUE_ADDR=0x2000  </exe.0.process.env.0>\n",indent);

    fprintf(output,"%s        <exe.0.physicalMemory.start> 0x100000 </exe.0.physicalMemory.start>\n",indent);
    fprintf(output,"%s        <exe.0.physicalMemory.end>   0x1fffffff </exe.0.physicalMemory.end>\n",indent);
    fprintf(output,"%s    </params>\n",indent);
    fprintf(output,"%s    <link name=nid%d.mem-bus port=port latency=$lat />\n",indent,nid);
    fprintf(output,"%s</component>\n",indent);

    fprintf(output,"\n");

    fprintf(output,"%s<component name=nid%d.cpu0.syscall type=Syscall >\n",indent,nid);
    fprintf(output,"%s    <params>\n",indent);
    fprintf(output,"%s        <startAddr> 0x00000000 </startAddr>\n",indent);
    fprintf(output,"%s    </params>\n",indent);
    fprintf(output,"%s    <link name=nid%d.bus-syscallDMA port=dma latency=$lat />\n",indent,nid);
    fprintf(output,"%s    <link name=nid%d.bus-syscallPIO port=pio latency=$lat />\n",indent,nid);
    fprintf(output,"%s</component>\n",indent);

    fprintf(output,"\n");

    fprintf(output,"%s<component name=nid%d.cpu0.PtlNic type=PtlNicMMIF >\n",indent,nid);
    fprintf(output,"%s    <params>\n",indent);
    fprintf(output,"%s        <startAddr> 0x00002000 </startAddr>\n",indent);
    fprintf(output,"%s        <id> %d </id>\n",indent, nid);
    fprintf(output,"%s    </params>\n",indent);
    fprintf(output,"%s    <link name=nid%d.bus-nicDMA port=dma latency=$lat />\n",indent,nid);
    fprintf(output,"%s    <link name=nid%d.bus-nicPIO port=pio latency=$lat />\n",indent,nid);
    fprintf(output,"%s</component>\n",indent);
}
