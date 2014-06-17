// Copyright 2009-2014 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2014, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

// Uses PIN Tool Memory Trace as a basis for provide SST Memory Tracing output
// directly to the tracing framework.

#include <stdio.h>
#include "pin.H"
#include <cstring>
#include <iostream>

#include <zlib.h>

using namespace std;

FILE * trace;

uint32_t trace_format;
uint64_t instruction_count;

const char READ_OPERATION_CHAR = 0;
const char WRITE_OPERATION_CHAR = 1;

char RECORD_BUFFER[ sizeof(uint64_t) + sizeof(uint64_t) + sizeof(uint32_t) + sizeof(char) ];

KNOB<string> KnobInsRoutine(KNOB_MODE_WRITEONCE, "pintool",
    "r", "", "Instrument only a specific routine (if not specified all instructions are instrumented");
KNOB<string> KnobTraceFile(KNOB_MODE_WRITEONCE, "pintool",
    "o", "pinatrace.out", "Output analysis to trace file.");
KNOB<string> KnobTraceFormat(KNOB_MODE_WRITEONCE, "pintool",
    "f", "text", "Output format, \'text\' = Plain text, \'binary\' = Binary, \'compressed\' = zlib compressed");

void copy(VOID* dest, const VOID* source, int destoffset, int count) {
	char* dest_c = (char*) dest;
	char* source_c = (char*) source;
	int counter;

	for(counter = 0; counter < count; counter++) {
		dest_c[destoffset + counter] = source_c[counter];
	}
}

// Print a memory read record
VOID RecordMemRead(VOID * addr, UINT32 size)
{
    UINT64 ma_addr = (UINT64) addr;

    if(0 == trace_format) {

    } else if (1 == trace_format) {
	copy(RECORD_BUFFER, &instruction_count, 0, sizeof(uint64_t) );
    	copy(RECORD_BUFFER, &READ_OPERATION_CHAR, sizeof(uint64_t), sizeof(char) );
    	copy(RECORD_BUFFER, &ma_addr, sizeof(uint64_t) + sizeof(char), sizeof(uint64_t) );
    	copy(RECORD_BUFFER, &size, sizeof(uint64_t) + sizeof(char) + sizeof(uint64_t), sizeof(uint32_t) );

    	fwrite( RECORD_BUFFER, sizeof(uint64_t) + sizeof(uint64_t) + sizeof(uint32_t) + sizeof(char), 1, trace);
    } else if (2 == trace_format) {
	copy(RECORD_BUFFER, &instruction_count, 0, sizeof(uint64_t) );
    	copy(RECORD_BUFFER, &READ_OPERATION_CHAR, sizeof(uint64_t), sizeof(char) );
    	copy(RECORD_BUFFER, &ma_addr, sizeof(uint64_t) + sizeof(char), sizeof(uint64_t) );
    	copy(RECORD_BUFFER, &size, sizeof(uint64_t) + sizeof(char) + sizeof(uint64_t), sizeof(uint32_t) );

	gzwrite((gzFile) trace, RECORD_BUFFER, sizeof(uint64_t) + sizeof(uint64_t) + sizeof(uint32_t) + sizeof(char));
   }
}

// Print a memory write record
VOID RecordMemWrite(VOID * addr, UINT32 size)
{
    UINT64 ma_addr = (UINT64) addr;

    /*fwrite(&instruction_count, sizeof(instruction_count), 1, trace);
    fwrite(&WRITE_OPERATION_CHAR, sizeof(char), 1, trace);
    fwrite(&ma_addr, sizeof(ma_addr), 1, trace);
    fwrite(&size, sizeof(UINT32), 1, trace);*/

    if(0 == trace_format) {

    } else if(1 == trace_format) {
    	copy(RECORD_BUFFER, &instruction_count, 0, sizeof(uint64_t) );
    	copy(RECORD_BUFFER, &WRITE_OPERATION_CHAR, sizeof(uint64_t), sizeof(char) );
    	copy(RECORD_BUFFER, &ma_addr, sizeof(uint64_t) + sizeof(char), sizeof(uint64_t) );
    	copy(RECORD_BUFFER, &size, sizeof(uint64_t) + sizeof(char) + sizeof(uint64_t), sizeof(uint32_t) );

   	fwrite( RECORD_BUFFER, sizeof(uint64_t) + sizeof(uint64_t) + sizeof(uint32_t) + sizeof(char), 1, trace);
   } else if(2 == trace_format) {
    	copy(RECORD_BUFFER, &instruction_count, 0, sizeof(uint64_t) );
    	copy(RECORD_BUFFER, &WRITE_OPERATION_CHAR, sizeof(uint64_t), sizeof(char) );
    	copy(RECORD_BUFFER, &ma_addr, sizeof(uint64_t) + sizeof(char), sizeof(uint64_t) );
    	copy(RECORD_BUFFER, &size, sizeof(uint64_t) + sizeof(char) + sizeof(uint64_t), sizeof(uint32_t) );

	gzwrite((gzFile) trace, RECORD_BUFFER, sizeof(uint64_t) + sizeof(uint64_t) + sizeof(uint32_t) + sizeof(char));
   }
}

VOID IncrementInstructionCount() {
    instruction_count++;
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
                IARG_END);
        }
    }

    INS_InsertPredicatedCall(
	ins, IPOINT_BEFORE, (AFUNPTR) IncrementInstructionCount, IARG_END);
}

VOID InstrumentSpecificRoutine(RTN rtn, VOID* v) {
	if(RTN_Name(rtn) == KnobInsRoutine.Value()) {
		std::cout << "TRACE: Found routine: " << RTN_Name(rtn) << ", instrumenting for tracing..." << std::endl;

		RTN_Open(rtn);

		for(INS ins = RTN_InsHead(rtn); INS_Valid(ins); ins = INS_Next(ins)) {
			Instruction(ins, v);
		}

		std::cout << "TRACE: Instrumentation completed." << std::endl;
		RTN_Close(rtn);
	}
}

VOID Fini(INT32 code, VOID *v)
{
    if( (0 == trace_format) || (1 == trace_format)) {
    	fclose(trace);
    } else if (2 == trace_format) {
	gzclose(trace);
    }
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

    if(KnobTraceFormat.Value() == "text") {
	trace_format = 0;
	trace = fopen(KnobTraceFile.Value().c_str(), "wt");
    } else if(KnobTraceFormat.Value() == "binary") {
	trace_format = 1;
	trace = fopen(KnobTraceFile.Value().c_str(), "wb");
    } else if(KnobTraceFormat.Value() == "compressed") {
	trace_format = 2;
	trace = (FILE*) gzopen(KnobTraceFile.Value().c_str(), "wb");
    } else {
	std::cerr << "Error: Unknown trace format: " << KnobTraceFormat.Value() << "." << std::endl;
        exit(-1);
    }

    instruction_count = 0;

    std::cout << "TRACE: Checking for specific routine instrumentation...";

    if(KnobInsRoutine.Value() == "") {
	std::cout << "not found, instrument all routines." << std::endl;
    	INS_AddInstrumentFunction(Instruction, 0);
    } else {
	std::cout << "found, instrumenting: " << KnobInsRoutine.Value() << std::endl;
	RTN_AddInstrumentFunction(InstrumentSpecificRoutine, 0);
    }

    PIN_AddFiniFunction(Fini, 0);

    // Never returns
    PIN_StartProgram();

    return 0;
}
