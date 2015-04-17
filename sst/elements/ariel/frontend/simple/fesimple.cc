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

//#include <malloc.h>
#include <execinfo.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include "pin.H"
#include <time.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <map>
#include <stack>
#include <ctime>
#include <bitset>
//This must be defined before inclusion of intttypes.h
#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif

#include <sst/elements/ariel/ariel_shmem.h>
#undef __STDC_FORMAT_MACROS
using namespace SST::ArielComponent;

KNOB<string> SSTNamedPipe(KNOB_MODE_WRITEONCE, "pintool",
    "p", "", "Named pipe to connect to SST simulator");
KNOB<UINT64> MaxInstructions(KNOB_MODE_WRITEONCE, "pintool",
    "i", "10000000000", "Maximum number of instructions to run");
KNOB<UINT32> SSTVerbosity(KNOB_MODE_WRITEONCE, "pintool",
    "v", "0", "SST verbosity level");
KNOB<UINT32> MaxCoreCount(KNOB_MODE_WRITEONCE, "pintool",
    "c", "1", "Maximum core count to use for data pipes.");
KNOB<UINT32> StartupMode(KNOB_MODE_WRITEONCE, "pintool",
    "s", "1", "Mode for configuring profile behavior, 1 = start enabled, 0 = start disabled");
KNOB<UINT32> InterceptMultiLevelMemory(KNOB_MODE_WRITEONCE, "pintool",
    "m", "1", "Should intercept multi-level memory allocations, copies and frees, 1 = start enabled, 0 = start disabled");
KNOB<UINT32> DefaultMemoryPool(KNOB_MODE_WRITEONCE, "pintool",
    "d", "0", "Default SST Memory Pool");

//PIN_LOCK pipe_lock;
UINT32 core_count;
UINT32 default_pool;
ArielTunnel *tunnel = NULL;
bool enable_output;
std::vector<void*> allocated_list;


VOID Fini(INT32 code, VOID* v)
{
	if(SSTVerbosity.Value() > 0) {
		std::cout << "SSTARIEL: Execution completed, shutting down." << std::endl;
	}

    ArielCommand ac;
    ac.command = ARIEL_PERFORM_EXIT;
    ac.instPtr = (uint64_t) 0;
    tunnel->writeMessage(0, ac);

    delete tunnel;
}

VOID copy(void* dest, const void* input, UINT32 length) {
	for(UINT32 i = 0; i < length; ++i) {
		((char*) dest)[i] = ((char*) input)[i];
	}
}

VOID WriteInstructionRead(ADDRINT* address, UINT32 readSize, THREADID thr, ADDRINT ip) {
	if(enable_output) {
		uint64_t addr64 = (uint64_t) address;

		assert(thr < core_count);

        ArielCommand ac;
        ac.command = ARIEL_PERFORM_READ;
	ac.instPtr = (uint64_t) ip;
        ac.inst.addr = addr64;
        ac.inst.size = readSize;
        tunnel->writeMessage(thr, ac);
	}
}

VOID WriteInstructionWrite(ADDRINT* address, UINT32 writeSize, THREADID thr, ADDRINT ip) {
	if(enable_output) {
		uint64_t addr64 = (uint64_t) address;

        ArielCommand ac;
        ac.command = ARIEL_PERFORM_WRITE;
	ac.instPtr = (uint64_t) ip;
        ac.inst.addr = addr64;
        ac.inst.size = writeSize;
        tunnel->writeMessage(thr, ac);
	}
}

VOID WriteStartInstructionMarker(UINT32 thr, ADDRINT ip) {
    ArielCommand ac;
    ac.command = ARIEL_START_INSTRUCTION;
    ac.instPtr = (uint64_t) ip;
    tunnel->writeMessage(thr, ac);
}

VOID WriteEndInstructionMarker(UINT32 thr, ADDRINT ip) {
    ArielCommand ac;
    ac.command = ARIEL_END_INSTRUCTION;
    ac.instPtr = (uint64_t) ip;
    tunnel->writeMessage(thr, ac);
}

VOID WriteInstructionReadWrite(THREADID thr, ADDRINT* readAddr, UINT32 readSize,
	ADDRINT* writeAddr, UINT32 writeSize, ADDRINT ip) {

	if(enable_output) {
		if(thr < core_count) {

            const uint64_t wAddr64 = (uint64_t) writeAddr;
            const uint32_t wSize   = (uint32_t) writeSize;
            const uint64_t rAddr64 = (uint64_t) readAddr;
            const uint32_t rSize   = (uint32_t) readSize;

			const uint32_t thrID = (uint32_t) thr;

            ArielCommand acStart, acRead, acWrite, acEnd;

            acStart.command = ARIEL_START_INSTRUCTION;
	    acStart.instPtr = (uint64_t) ip;
            tunnel->writeMessage(thrID, acStart);
            acRead.command = ARIEL_PERFORM_READ;
            acRead.instPtr = (uint64_t) ip;
            acRead.inst.addr = rAddr64;
            acRead.inst.size = rSize;
            tunnel->writeMessage(thrID, acRead);
            acWrite.command = ARIEL_PERFORM_WRITE;
            acWrite.instPtr = (uint64_t) ip;
            acWrite.inst.addr = wAddr64;
            acWrite.inst.size = wSize;
            tunnel->writeMessage(thrID, acWrite);
            acEnd.command = ARIEL_END_INSTRUCTION;
            acEnd.instPtr = (uint64_t) ip;
            tunnel->writeMessage(thrID, acEnd);
		}
	}

}

VOID WriteInstructionReadOnly(THREADID thr, ADDRINT* readAddr, UINT32 readSize, ADDRINT ip) {

	if(enable_output) {
		if(thr < core_count) {

            const uint64_t rAddr64 = (uint64_t) readAddr;
            const uint32_t rSize   = (uint32_t) readSize;

            const uint32_t thrID = (uint32_t) thr;

            ArielCommand acStart, acRead, acEnd;
            acStart.command = ARIEL_START_INSTRUCTION;
            acStart.instPtr = (uint64_t) ip;
            tunnel->writeMessage(thrID, acStart);
            acRead.command = ARIEL_PERFORM_READ;
  	    acRead.instPtr = (uint64_t) ip;
            acRead.inst.addr = rAddr64;
            acRead.inst.size = rSize;
            tunnel->writeMessage(thrID, acRead);
            acEnd.command = ARIEL_END_INSTRUCTION;
            acEnd.instPtr = (uint64_t) ip;
            tunnel->writeMessage(thrID, acEnd);
		}
	}

}

VOID WriteNoOp(THREADID thr, ADDRINT ip) {
	if(enable_output) {
		if(thr < core_count) {
            ArielCommand ac;
            ac.command = ARIEL_NOOP;
            ac.instPtr = (uint64_t) ip;
            tunnel->writeMessage(thr, ac);
		}
	}
}

VOID WriteInstructionWriteOnly(THREADID thr, ADDRINT* writeAddr, UINT32 writeSize,
	ADDRINT ip) {

	if(enable_output) {
		if(thr < core_count) {

            const uint64_t wAddr64 = (uint64_t) writeAddr;
            const uint32_t wSize   = (uint32_t) writeSize;

            const uint32_t thrID = (uint32_t) thr;

            ArielCommand acStart, acWrite, acEnd;
            acStart.command = ARIEL_START_INSTRUCTION;
            acStart.instPtr = (uint64_t) ip;
            tunnel->writeMessage(thrID, acStart);
            acWrite.command = ARIEL_PERFORM_WRITE;
            acWrite.instPtr = (uint64_t) ip;
            acWrite.inst.addr = wAddr64;
            acWrite.inst.size = wSize;
            tunnel->writeMessage(thrID, acWrite);
            acEnd.command = ARIEL_END_INSTRUCTION;
            acEnd.instPtr = (uint64_t) ip;
            tunnel->writeMessage(thrID, acEnd);
		}
	}

}

VOID InstrumentInstruction(INS ins, VOID *v)
{
	if( INS_IsMemoryRead(ins) && INS_IsMemoryWrite(ins) ) {
		INS_InsertPredicatedCall(ins, IPOINT_BEFORE, (AFUNPTR)
			WriteInstructionReadWrite,
			IARG_THREAD_ID,
			IARG_MEMORYREAD_EA, IARG_UINT32, INS_MemoryReadSize(ins),
			IARG_MEMORYWRITE_EA, IARG_UINT32, INS_MemoryWriteSize(ins),
			IARG_INST_PTR,
			IARG_END);
	} else if( INS_IsMemoryRead(ins) ) {
		INS_InsertPredicatedCall(ins, IPOINT_BEFORE, (AFUNPTR)
			WriteInstructionReadOnly,
			IARG_THREAD_ID,
			IARG_MEMORYREAD_EA, IARG_UINT32, INS_MemoryReadSize(ins),
			IARG_INST_PTR,
			IARG_END);
	} else if( INS_IsMemoryWrite(ins) ) {
		INS_InsertPredicatedCall(ins, IPOINT_BEFORE, (AFUNPTR)
			WriteInstructionWriteOnly,
			IARG_THREAD_ID,
			IARG_MEMORYWRITE_EA, IARG_UINT32, INS_MemoryWriteSize(ins),
			IARG_INST_PTR,
			IARG_END);
	} else {
		INS_InsertPredicatedCall(ins, IPOINT_BEFORE, (AFUNPTR)
			WriteNoOp,
			IARG_THREAD_ID,
			IARG_INST_PTR,
			IARG_END);
	}

}

void mapped_ariel_enable() {
    fprintf(stderr, "ARIEL: Enabling memory and instruction tracing from program control.\n");
    fflush(stdout);
    fflush(stderr);
    enable_output = true;
}

uint64_t mapped_ariel_cycles() {
    return tunnel->getCycles();
}

int mapped_gettimeofday(struct timeval *tp, void *tzp) {
    if ( tp == NULL ) { errno = EINVAL ; return -1; }

    tunnel->getTime(tp);
    return 0;
}

VOID InstrumentRoutine(RTN rtn, VOID* args) {

    if (RTN_Name(rtn) == "ariel_enable" ||
		RTN_Name(rtn) == "_ariel_enable") {
	fprintf(stderr,"Identified routine: ariel_enable, replacing with Ariel equivalent...\n");
	RTN_Replace(rtn, (AFUNPTR) mapped_ariel_enable);
	fprintf(stderr,"Replacement complete.\n");
	return;
    } else if (RTN_Name(rtn) == "gettimeofday" ||
		RTN_Name(rtn) == "__gettimeofday") {
	fprintf(stderr,"Identified routine: gettimeofday, replacing with Ariel equivalent...\n");
	RTN_Replace(rtn, (AFUNPTR) mapped_gettimeofday);
	fprintf(stderr,"Replacement complete.\n");
	return;
    } else if (RTN_Name(rtn) == "ariel_cycles" ||
		RTN_Name(rtn) == "_ariel_cycles") {
	fprintf(stderr, "Identified routine: ariel_cycles, replacing with Ariel equivalent..\n");
	RTN_Replace(rtn, (AFUNPTR) mapped_ariel_cycles);
	fprintf(stderr, "Replacement complete\n");
	return;
    }

}

/* ===================================================================== */
/* Print Help Message                                                    */
/* ===================================================================== */

INT32 Usage()
{
    PIN_ERROR( "This Pintool collects statistics for instructions.\n" 
              + KNOB_BASE::StringKnobSummary() + "\n");
    return -1;
}

/* ===================================================================== */
/* Main                                                                  */
/* ===================================================================== */

int main(int argc, char *argv[])
{
    if (PIN_Init(argc, argv)) return Usage();

    // Load the symbols ready for us to mangle functions.
    PIN_InitSymbols();
    PIN_AddFiniFunction(Fini, 0);

    if(SSTVerbosity.Value() > 0) {
        std::cout << "SSTARIEL: Loading Ariel Tool to connect to SST on pipe: " <<
            SSTNamedPipe.Value() << " max instruction count: " <<
            MaxInstructions.Value() <<
            " max core count: " << MaxCoreCount.Value() << std::endl;
    }

    core_count = MaxCoreCount.Value();

    tunnel = new ArielTunnel(SSTNamedPipe.Value());

	fprintf(stderr, "ARIEL-SST PIN tool activating with %" PRIu32 " threads\n", core_count);
	fflush(stdout);

    sleep(1);

    default_pool = DefaultMemoryPool.Value();
    fprintf(stderr, "ARIEL: Default memory pool set to %" PRIu32 "\n", default_pool);

    if(StartupMode.Value() == 1) {
        fprintf(stderr, "ARIEL: Tool is configured to begin with profiling immediately.\n");
        enable_output = true;
    } else if (StartupMode.Value() == 0) {
        fprintf(stderr, "ARIEL: Tool is configured to suspend profiling until program control\n");
        enable_output = false;
    }

    INS_AddInstrumentFunction(InstrumentInstruction, 0);
    RTN_AddInstrumentFunction(InstrumentRoutine, 0);

    fprintf(stderr, "ARIEL: Starting program.\n");
    fflush(stdout);
    PIN_StartProgram();

    return 0;
}

