// Copyright 2009-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2015, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <stdio.h>

static char *str = "<cpuParams>\n\
    <system> nid0.system </system>\n\
    <base.process> nid0.process </base.process>\n\
\n\
    <base.process.registerExit> yes </base.process.registerExit>\n\
\n\
    <base.process.pid> 1 </base.process.pid>\n\
    <base.process.uid> 100 </base.process.uid>\n\
    <base.process.euid> 100 </base.process.euid>\n\
    <base.process.gid> 100 </base.process.gid>\n\
    <base.process.egid> 100 </base.process.egid>\n\
    <base.process.cwd> / </base.process.cwd>\n\
    <base.process.executable> ${M5_EXE} </base.process.executable>\n\
    <base.process.simpoint> 0 </base.process.simpoint>\n\
    <base.process.errout> cerr </base.process.errout>\n\
    <base.process.input> cin </base.process.input>\n\
    <base.process.output> cout </base.process.output>\n\
    <base.process.max_stack_size> 0x4000000 </base.process.max_stack_size>\n\
\n\
    <o3cpu.fetchTrapLatency> 1 </o3cpu.fetchTrapLatency>\n\
    <o3cpu.trapLatency> 13 </o3cpu.trapLatency>\n\
    <o3cpu.smtIQThreshold> 100 </o3cpu.smtIQThreshold>\n\
    <o3cpu.smtLSQThreshold> 100 </o3cpu.smtLSQThreshold>\n\
    <o3cpu.smtROBThreshold> 100 </o3cpu.smtROBThreshold>\n\
    <o3cpu.predType> tournament </o3cpu.predType>\n\
    <o3cpu.smtCommitPolicy> RoundRobin </o3cpu.smtCommitPolicy>\n\
    <o3cpu.smtFetchPolicy> SingleThread </o3cpu.smtFetchPolicy>\n\
    <o3cpu.smtIQPolicy> Partitioned </o3cpu.smtIQPolicy>\n\
    <o3cpu.smtLSQPolicy> Partitioned </o3cpu.smtLSQPolicy>\n\
    <o3cpu.smtROBPolicy> Partitioned </o3cpu.smtROBPolicy>\n\
\n\
    <o3cpu.BTBEntries> 4096</o3cpu.BTBEntries>\n\
    <o3cpu.BTBTagSize> 16 </o3cpu.BTBTagSize>\n\
    <o3cpu.LFSTSize> 1024 </o3cpu.LFSTSize>\n\
    <o3cpu.LQEntries> 128 </o3cpu.LQEntries>\n\
    <o3cpu.RASSize> 32 </o3cpu.RASSize>\n\
    <o3cpu.SQEntries> 128 </o3cpu.SQEntries>\n\
    <o3cpu.SSITSize> 1024 </o3cpu.SSITSize>\n\
    <o3cpu.activity> 0 </o3cpu.activity>\n\
    <o3cpu.backComSize> 5 </o3cpu.backComSize>\n\
    <o3cpu.cachePorts> 8 </o3cpu.cachePorts>\n\
    <o3cpu.choiceCtrBits> 2 </o3cpu.choiceCtrBits>\n\
    <o3cpu.choicePredictorSize> 8192 </o3cpu.choicePredictorSize>\n\
\n\
    <o3cpu.commitToDecodeDelay> 1 </o3cpu.commitToDecodeDelay>\n\
    <o3cpu.commitToFetchDelay> 1 </o3cpu.commitToFetchDelay>\n\
    <o3cpu.commitToIEWDelay> 1 </o3cpu.commitToIEWDelay>\n\
    <o3cpu.commitToRenameDelay> 1 </o3cpu.commitToRenameDelay>\n\
    <o3cpu.commitWidth> 4 </o3cpu.commitWidth>\n\
    <o3cpu.decodeToFetchDelay> 1 </o3cpu.decodeToFetchDelay>\n\
    <o3cpu.decodeToRenameDelay> 1 </o3cpu.decodeToRenameDelay>\n\
    <o3cpu.decodeWidth> 4 </o3cpu.decodeWidth>\n\
    <o3cpu.dispatchWidth> 4 </o3cpu.dispatchWidth>\n\
    <o3cpu.fetchToDecodeDelay> 1 </o3cpu.fetchToDecodeDelay>\n\
    <o3cpu.fetchWidth> 4 </o3cpu.fetchWidth>\n\
    <o3cpu.forwardComSize> 5 </o3cpu.forwardComSize>\n\
    <o3cpu.globalCtrBits> 2 </o3cpu.globalCtrBits>\n\
    <o3cpu.globalHistoryBits> 13 </o3cpu.globalHistoryBits>\n\
    <o3cpu.globalPredictorSize> 8192 </o3cpu.globalPredictorSize>\n\
\n\
    <o3cpu.iewToCommitDelay> 1 </o3cpu.iewToCommitDelay>\n\
    <o3cpu.iewToDecodeDelay> 1 </o3cpu.iewToDecodeDelay>\n\
    <o3cpu.iewToFetchDelay> 1 </o3cpu.iewToFetchDelay>\n\
    <o3cpu.iewToRenameDelay> 1 </o3cpu.iewToRenameDelay>\n\
    <o3cpu.instShiftAmt> 2 </o3cpu.instShiftAmt>\n\
    <o3cpu.issueToExecuteDelay> 1 </o3cpu.issueToExecuteDelay>\n\
    <o3cpu.issueWidth> 4 </o3cpu.issueWidth>\n\
    <o3cpu.localCtrBits> 2 </o3cpu.localCtrBits>\n\
    <o3cpu.localHistoryBits> 11 </o3cpu.localHistoryBits>\n\
    <o3cpu.localHistoryTableSize> 2048 </o3cpu.localHistoryTableSize>\n\
    <o3cpu.localPredictorSize> 2048 </o3cpu.localPredictorSize>\n\
\n\
    <o3cpu.LSQDepCheckShift> 4 </o3cpu.LSQDepCheckShift>\n\
    <o3cpu.LSQCheckLoads> true </o3cpu.LSQCheckLoads>\n\
\n\
    <o3cpu.numIQEntries> 16 </o3cpu.numIQEntries>\n\
    <o3cpu.numPhysFloatRegs> 256 </o3cpu.numPhysFloatRegs>\n\
    <o3cpu.numPhysIntRegs> 256 </o3cpu.numPhysIntRegs>\n\
    <o3cpu.numROBEntries> 16 </o3cpu.numROBEntries>\n\
    <o3cpu.numRobs> 1 </o3cpu.numRobs>\n\
    <o3cpu.renameToDecodeDelay> 1 </o3cpu.renameToDecodeDelay>\n\
    <o3cpu.renameToFetchDelay> 1 </o3cpu.renameToFetchDelay>\n\
    <o3cpu.renameToIEWDelay> 2 </o3cpu.renameToIEWDelay>\n\
    <o3cpu.renameToROBDelay> 1 </o3cpu.renameToROBDelay>\n\
    <o3cpu.renameWidth> 4 </o3cpu.renameWidth>\n\
    <o3cpu.smtNumFetchingThreads> 1 </o3cpu.smtNumFetchingThreads>\n\
    <o3cpu.squashWidth> 4 </o3cpu.squashWidth>\n\
    <o3cpu.wbDepth> 1 </o3cpu.wbDepth>\n\
    <o3cpu.wbWidth> 2 </o3cpu.wbWidth>\n\
\n\
    <base.dtb.size> 64 </base.dtb.size>\n\
    <base.itb.size> 64 </base.itb.size>\n\
    <base.max_insts_all_threads> 0 </base.max_insts_all_threads>\n\
    <base.max_insts_any_thread> 0 </base.max_insts_any_thread>\n\
    <base.max_loads_all_threads> 0 </base.max_loads_all_threads>\n\
    <base.max_loads_any_thread> 0 </base.max_loads_any_thread>\n\
    <base.clock> 2.0Ghz </base.clock>\n\
    <base.function_trace_start> 0 </base.function_trace_start>\n\
    <base.phase> 0 </base.phase>\n\
    <base.progress_interval> 0 </base.progress_interval>\n\
    <base.defer_registration> 0 </base.defer_registration>\n\
    <base.do_checkpoint_insts> 1 </base.do_checkpoint_insts>\n\
    <base.do_statistics_insts> 1 </base.do_statistics_insts>\n\
    <base.function_trace> 0 </base.function_trace>\n\
    <base.id> 0 </base.id>\n\
</cpuParams>\n";

void genCpuParams( FILE* fp )
{
    fprintf(fp,"%s",str);
}
