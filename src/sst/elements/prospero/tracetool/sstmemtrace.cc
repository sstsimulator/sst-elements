// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2018, NTESS
// All rights reserved.
// 
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

// Uses PIN Tool Memory Trace as a basis for provide SST Memory Tracing output
// directly to the tracing framework.

#include <sst_config.h>
#include "pin.H"

#include <stdio.h>
#include <stdlib.h>
#include <cstring>
#include <iostream>
#include <inttypes.h>

#ifdef HAVE_LIBZ
#include <zlib.h>
#endif

using namespace std;

uint32_t max_thread_count;
uint32_t trace_format;
uint64_t instruction_count;
uint32_t traceEnabled __attribute__((aligned(64)));
uint64_t nextFileTrip;

const char READ_OPERATION_CHAR = 'R';
const char WRITE_OPERATION_CHAR = 'W';

char RECORD_BUFFER[ sizeof(uint64_t) + sizeof(uint64_t) + sizeof(uint32_t) + sizeof(char) ];

// We have two file pointers, one for compressed traces and one for
// "normal" (binary or text) traces
FILE** trace;

#ifdef HAVE_LIBZ
gzFile* traceZ;
#endif

typedef struct {
	UINT64 threadInit;
	UINT64 insCount;
	UINT64 readCount;
	UINT64 writeCount;
	UINT64 currentFile;
	UINT64 padD;
	UINT64 padE;
	UINT64 padF;
} threadRecord;

char** fileBuffers;
threadRecord* thread_instr_id;

KNOB<string> KnobInsRoutine(KNOB_MODE_WRITEONCE, "pintool",
    "r", "", "Instrument only a specific routine (if not specified all instructions are instrumented");
KNOB<string> KnobTraceFile(KNOB_MODE_WRITEONCE, "pintool",
    "o", "sstprospero", "Output analysis to trace file.");
KNOB<string> KnobTraceFormat(KNOB_MODE_WRITEONCE, "pintool",
    "f", "text", "Output format, \'text\' = Plain text, \'binary\' = Binary, \'compressed\' = zlib compressed");
KNOB<UINT32> KnobMaxThreadCount(KNOB_MODE_WRITEONCE, "pintool",
    "t", "1", "Maximum number of threads to record memory patterns");
KNOB<UINT32> KnobFileBufferSize(KNOB_MODE_WRITEONCE, "pintool",
    "b", "32768", "Size in bytes for each trace buffer");
KNOB<UINT32> KnobTraceEnabled(KNOB_MODE_WRITEONCE, "pintool",
    "d", "1", "Disable until application says that tracing can start, 0=disable until app, 1=start enabled, default=1");
KNOB<UINT64> KnobFileTrip(KNOB_MODE_WRITEONCE, "pintool",
    "l", "1125899906842624", "Trip into a new trace file at this instruction count, default=1125899906842624 (2**50)");

void prospero_enable() {
	printf("PROSPERO: Tracing enabled\n");
	traceEnabled = 1;
}

void prospero_disable() {
	printf("PROSPERO: Tracing disabled.\n");
	traceEnabled = 0;
}

void copy(VOID* dest, const VOID* source, int destoffset, int count) {
	char* dest_c = (char*) dest;
	char* source_c = (char*) source;
	int counter;

	for(counter = 0; counter < count; counter++) {
		dest_c[destoffset + counter] = source_c[counter];
	}
}

VOID PerformInstrumentCountCheck(THREADID id) {
	if(id >= max_thread_count) {
		return;
	}

	if(thread_instr_id[id].threadInit == 0) {
		std::cout << "PROSPERO: Thread " << id << " starts at instruction " << thread_instr_id[0].insCount << std::endl;
		// Copy over instructions from thread zero and mark started;
		thread_instr_id[id].insCount = thread_instr_id[0].insCount;
		thread_instr_id[id].threadInit = 1;
	}
}

// Print a memory read record
VOID RecordMemRead(VOID * addr, UINT32 size, THREADID thr)
{
#ifdef PROSPERO_DEBUG
     printf("PROSPERO: Calling into RecordMemRead...\n");
#endif

    UINT64 ma_addr = (UINT64) addr;

    PerformInstrumentCountCheck(thr);

    if(0 == trace_format) {
	fprintf(trace[thr], "%llu R %llu %d\n",
		(unsigned long long int) thread_instr_id[thr].insCount,
		(unsigned long long int) ma_addr,
		(int) size);
	thread_instr_id[thr].readCount++;
    } else if (1 == trace_format || 2 == trace_format) {
	copy(RECORD_BUFFER, &(thread_instr_id[thr].insCount), 0, sizeof(uint64_t) );
    	copy(RECORD_BUFFER, &READ_OPERATION_CHAR, sizeof(uint64_t), sizeof(char) );
    	copy(RECORD_BUFFER, &ma_addr, sizeof(uint64_t) + sizeof(char), sizeof(uint64_t) );
    	copy(RECORD_BUFFER, &size, sizeof(uint64_t) + sizeof(char) + sizeof(uint64_t), sizeof(uint32_t) );

	if(1 == trace_format) {
		if(thr < max_thread_count && (traceEnabled > 0)) {
    			fwrite(RECORD_BUFFER, sizeof(uint64_t) + sizeof(uint64_t) + sizeof(uint32_t) + sizeof(char), 1, trace[thr]);
			thread_instr_id[thr].readCount++;
		}
	} else {
#ifdef HAVE_LIBZ
		if(thr < max_thread_count && (traceEnabled > 0)) {
			gzwrite(traceZ[thr], RECORD_BUFFER, sizeof(uint64_t) + sizeof(uint64_t) + sizeof(uint32_t) + sizeof(char));
			thread_instr_id[thr].readCount++;
		}
#endif
	}
     }

#ifdef PROSPERO_DEBUG
     printf("PROSPERO: Completed into RecordMemRead...\n");
#endif

}

// Print a memory write record
VOID RecordMemWrite(VOID * addr, UINT32 size, THREADID thr)
{
#ifdef PROSPERO_DEBUG
     printf("PROSPERO: Calling into RecordMemWrite...\n");
#endif

    PerformInstrumentCountCheck(thr);

    UINT64 ma_addr = (UINT64) addr;

    if(0 == trace_format) {
	fprintf(trace[thr], "%llu W %llu %d\n",
		(unsigned long long int) thread_instr_id[thr].insCount,
		(unsigned long long int) ma_addr,
		(int) size);
	thread_instr_id[thr].writeCount++;
    } else if(1 == trace_format || 2 == trace_format) {
    	copy(RECORD_BUFFER, &(thread_instr_id[thr].insCount), 0, sizeof(uint64_t) );
    	copy(RECORD_BUFFER, &WRITE_OPERATION_CHAR, sizeof(uint64_t), sizeof(char) );
    	copy(RECORD_BUFFER, &ma_addr, sizeof(uint64_t) + sizeof(char), sizeof(uint64_t) );
    	copy(RECORD_BUFFER, &size, sizeof(uint64_t) + sizeof(char) + sizeof(uint64_t), sizeof(uint32_t) );

	if(1 == trace_format) {
		if(thr < max_thread_count && (traceEnabled > 0)) {
	   		fwrite(RECORD_BUFFER, sizeof(uint64_t) + sizeof(uint64_t) + sizeof(uint32_t) + sizeof(char), 1, trace[thr]);
			thread_instr_id[thr].writeCount++;
		}
	} else {
#ifdef HAVE_LIBZ
		if(thr < max_thread_count && (traceEnabled > 0)) {
			gzwrite(traceZ[thr], RECORD_BUFFER, sizeof(uint64_t) + sizeof(uint64_t) + sizeof(uint32_t) + sizeof(char));
			thread_instr_id[thr].writeCount++;
		}
#endif
	}
     }
#ifdef PROSPERO_DEBUG
     printf("PROSPERO: Completed into RecordMemWrite...\n");
#endif

}

VOID IncrementInstructionCount(THREADID id) {
	thread_instr_id[id].insCount++;

	if(thread_instr_id[id].insCount >= (nextFileTrip * thread_instr_id[id].currentFile)) {
		char buffer[256];

		if(trace_format == 0 || trace_format == 1) {
			fclose(trace[id]);

			if(trace_format == 0) {
				sprintf(buffer, "%s-%lu-%lu.trace",
					KnobTraceFile.Value().c_str(),
					(unsigned long) id,
					(unsigned long) thread_instr_id[id].currentFile);
				trace[id] = fopen(buffer, "wt");
			} else {
				sprintf(buffer,	"%s-%lu-%lu-bin.trace",
                                        KnobTraceFile.Value().c_str(),
					(unsigned long) id,
					(unsigned long) thread_instr_id[id].currentFile);
                                trace[id] = fopen(buffer, "wb");
			}
		}
#ifdef HAVE_LIBZ
		else if(trace_format == 2) {
			gzclose(traceZ[id]);
			sprintf(buffer, "%s-%lu-%lu-gz.trace",
				KnobTraceFile.Value().c_str(),
				(unsigned long) id,
				(unsigned long) thread_instr_id[id].currentFile);
			traceZ[id] = gzopen(buffer, "wb");
		}
#endif
		thread_instr_id[id].currentFile++;
	}
}

// Is called for every instruction and instruments reads and writes
VOID Instruction(INS ins, VOID *v)
{
    // Instruments memory accesses using a predicated call, i.e.
    // the instrumentation is called iff the instruction will actually be executed.
    //
    // The IA-64 architecture has explicitly predicated instructions. 
    // On the IA-32 and Intel(R) 64 architectures conditional moves and REP 
    // prefixed instructions appear as predicated instructions in Pin.
    UINT32 memOperands = INS_MemoryOperandCount(ins);

    // Iterate over each memory operand of the instruction.
    for (UINT32 memOp = 0; memOp < memOperands; memOp++)
    {
	USIZE mem_size = INS_MemoryOperandSize(ins, memOp);

        if (INS_MemoryOperandIsRead(ins, memOp))
        {
            INS_InsertPredicatedCall(
                ins, IPOINT_BEFORE, (AFUNPTR)RecordMemRead,
                IARG_MEMORYOP_EA, memOp,
		IARG_UINT32, (UINT32) mem_size,
		IARG_THREAD_ID,
                IARG_END);
        }
        // Note that in some architectures a single memory operand can be 
        // both read and written (for instance incl (%eax) on IA-32)
        // In that case we instrument it once for read and once for write.
        if (INS_MemoryOperandIsWritten(ins, memOp))
        {
            INS_InsertPredicatedCall(
                ins, IPOINT_BEFORE, (AFUNPTR)RecordMemWrite,
                IARG_MEMORYOP_EA, memOp,
		IARG_UINT32, (UINT32) mem_size,
		IARG_THREAD_ID,
                IARG_END);
        }
    }

    INS_InsertPredicatedCall(
	ins, IPOINT_BEFORE, (AFUNPTR) IncrementInstructionCount, IARG_THREAD_ID, IARG_END);
}

VOID InstrumentSpecificRoutine(RTN rtn, VOID* v) {
	if(RTN_Name(rtn) == "prospero_enable_tracing") {
		RTN_Replace(rtn, (AFUNPTR) prospero_enable);
	} else if(RTN_Name(rtn) == "prospero_disable_tracing") {
		RTN_Replace(rtn, (AFUNPTR) prospero_disable);
	} else if(RTN_Name(rtn) == "_prospero_enable_tracing") {
		RTN_Replace(rtn, (AFUNPTR) prospero_enable);
	} else if(RTN_Name(rtn) == "_prospero_disable_tracing") {
		RTN_Replace(rtn, (AFUNPTR) prospero_disable);
	} else if(KnobInsRoutine.Value() == "") {
		// User wants us to instrument every routine in the application do it instruction at a time
                RTN_Open(rtn);

                for(INS ins = RTN_InsHead(rtn); INS_Valid(ins); ins = INS_Next(ins)) {
                        Instruction(ins, v);
                }

                RTN_Close(rtn);
	} else if(RTN_Name(rtn) == KnobInsRoutine.Value()) {
		// User wants only a specific routine to be instrumented
		std::cout << "PROSPERO: Found routine: " << RTN_Name(rtn) << ", instrumenting for tracing..." << std::endl;

		RTN_Open(rtn);

		for(INS ins = RTN_InsHead(rtn); INS_Valid(ins); ins = INS_Next(ins)) {
			Instruction(ins, v);
		}

		std::cout << "PROSPERO: Instrumentation completed." << std::endl;
		RTN_Close(rtn);
	}
}

VOID Fini(INT32 code, VOID *v)
{
    printf("PROSPERO: Tracing is complete, closing trace files...\n");
    std::cout << "PROSPERO: Main thread exists with " << thread_instr_id[0].insCount << " instructions" << std::endl;

    if( (0 == trace_format) || (1 == trace_format)) {
	for(UINT32 i = 0; i < max_thread_count; ++i) {
    		fclose(trace[i]);
	}
#ifdef HAVE_LIBZ
    } else if (2 == trace_format) {
	for(UINT32 i = 0; i < max_thread_count; ++i) {
		gzclose(traceZ[i]);
	}
#endif
    }

    printf("PROSPERO: Thread read entries:     %llu\n", thread_instr_id[0].readCount);
    printf("PROSPERO: Thread write entries:    %llu\n", thread_instr_id[0].writeCount);
    printf("PROSPERO: Done.\n");
}

/* ===================================================================== */
/* Print Help Message                                                    */
/* ===================================================================== */
   
INT32 Usage()
{
    PIN_ERROR( "This Pintool prints a trace of memory addresses\n" 
              + KNOB_BASE::StringKnobSummary() + "\n");
    return -1;
}

/* ===================================================================== */
/* Main                                                                  */
/* ===================================================================== */

int main(int argc, char *argv[])
{
    if (PIN_Init(argc, argv)) return Usage();
    PIN_InitSymbols();

    traceEnabled = KnobTraceEnabled.Value();

    max_thread_count = KnobMaxThreadCount.Value();
    std::cout << "PROSPERO: User requests that a maximum of " << max_thread_count << " threads are instrumented" << std::endl;
    std::cout << "PROSPERO: File buffer per thread is " << KnobFileBufferSize.Value() << " bytes" << std::endl;

    if(traceEnabled == 0) {
    	std::cout << "PROSPERO: Trace is disabled from startup" << std::endl;
    } else {
    	std::cout << "PROSPERO: Trace is enabled from startup" << std::endl;
    }

    trace  = (FILE**) malloc(sizeof(FILE*) * max_thread_count);
#ifdef HAVE_LIBZ
    traceZ = (gzFile*) malloc(sizeof(gzFile) * max_thread_count);
#endif
    fileBuffers = (char**) malloc(sizeof(char*) * max_thread_count);

    char nameBuffer[256];

    if(KnobTraceFormat.Value() == "text") {
	printf("PROSPERO: Tracing will be recorded in text format.\n");
	trace_format = 0;

	for(UINT32 i = 0; i < max_thread_count; ++i) {
		sprintf(nameBuffer, "%s-%lu-0.trace", KnobTraceFile.Value().c_str(), (unsigned long) i);
		trace[i] = fopen(nameBuffer, "wt");
	}

	for(UINT32 i = 0; i < max_thread_count; ++i) {
		fileBuffers[i] = (char*) malloc(sizeof(char) * KnobFileBufferSize.Value());
		setvbuf(trace[i], fileBuffers[i], _IOFBF, (size_t) KnobFileBufferSize.Value());
	}
    } else if(KnobTraceFormat.Value() == "binary") {
	printf("PROSPERO: Tracing will be recorded in uncompressed binary format.\n");
	trace_format = 1;

	for(UINT32 i = 0; i < max_thread_count; ++i) {
		sprintf(nameBuffer, "%s-%lu-0-bin.trace", KnobTraceFile.Value().c_str(), (unsigned long) i);
		trace[i] = fopen(nameBuffer, "wb");
	}

	for(UINT32 i = 0; i < max_thread_count; ++i) {
		fileBuffers[i] = (char*) malloc(sizeof(char) * KnobFileBufferSize.Value());
		setvbuf(trace[i], fileBuffers[i], _IOFBF, (size_t) KnobFileBufferSize.Value());
	}
#ifdef HAVE_LIBZ
    } else if(KnobTraceFormat.Value() == "compressed") {
	printf("PROSPERO: Tracing will be recorded in compressed binary format.\n");
	trace_format = 2;

	for(UINT32 i = 0; i < max_thread_count; ++i) {
		sprintf(nameBuffer, "%s-%lu-0-gz.trace", KnobTraceFile.Value().c_str(), (unsigned long) i);
		traceZ[i] = gzopen(nameBuffer, "wb");
	}
#endif
    } else {
	std::cerr << "Error: Unknown trace format: " << KnobTraceFormat.Value() << "." << std::endl;
        exit(-1);
    }

    posix_memalign((void**) &thread_instr_id, 64, sizeof(threadRecord) * max_thread_count);
    for(UINT32 i = 0; i < max_thread_count; ++i) {
	thread_instr_id[i].insCount = 0;
	thread_instr_id[i].threadInit = 0;

	// Next file is going to be marked as 1 (we are really on file 0).
	thread_instr_id[i].currentFile = 1;
    }

    nextFileTrip = KnobFileTrip.Value();
    printf("PROSPERO: Next file trip count set to %llu instructions.\n", nextFileTrip);

    // Thread zero is always started
    thread_instr_id[0].threadInit = 1;

    //std::cout << "PROSPERO: Checking for specific routine instrumentation...";

    //if(KnobInsRoutine.Value() == "") {
//	std::cout << "not found, instrument all routines." << std::endl;
 //   	INS_AddInstrumentFunction(Instruction, 0);
  //  } else {
//	std::cout << "found, instrumenting: " << KnobInsRoutine.Value() << std::endl;
	RTN_AddInstrumentFunction(InstrumentSpecificRoutine, 0);
 //   }

    PIN_AddFiniFunction(Fini, 0);

    // Never returns
    PIN_StartProgram();

    return 0;
}
