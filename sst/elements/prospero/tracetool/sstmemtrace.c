// Uses PIN Tool Memory Trace as a basis for provide SST Memory Tracing output
// directly to the tracing framework.

#include <stdio.h>
#include "pin.H"
#include <cstring>

FILE * trace;

uint64_t instruction_count;

const char READ_OPERATION_CHAR = 0;
const char WRITE_OPERATION_CHAR = 1;

char RECORD_BUFFER[ sizeof(uint64_t) + sizeof(uint64_t) + sizeof(uint32_t) + sizeof(char) ];

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

/*    fwrite(&instruction_count, sizeof(instruction_count), 1, trace);
    fwrite(&READ_OPERATION_CHAR, sizeof(char), 1, trace);
    fwrite(&ma_addr, sizeof(ma_addr), 1, trace);
    fwrite(&size, sizeof(UINT32), 1, trace); */

    copy(RECORD_BUFFER, &instruction_count, 0, sizeof(uint64_t) );
    copy(RECORD_BUFFER, &READ_OPERATION_CHAR, sizeof(uint64_t), sizeof(char) );
    copy(RECORD_BUFFER, &ma_addr, sizeof(uint64_t) + sizeof(char), sizeof(uint64_t) );
    copy(RECORD_BUFFER, &size, sizeof(uint64_t) + sizeof(char) + sizeof(uint64_t), sizeof(uint32_t) );

   fwrite( RECORD_BUFFER, sizeof(uint64_t) + sizeof(uint64_t) + sizeof(uint32_t) + sizeof(char), 1, trace);
}

// Print a memory write record
VOID RecordMemWrite(VOID * addr, UINT32 size)
{
    UINT64 ma_addr = (UINT64) addr;

    /*fwrite(&instruction_count, sizeof(instruction_count), 1, trace);
    fwrite(&WRITE_OPERATION_CHAR, sizeof(char), 1, trace);
    fwrite(&ma_addr, sizeof(ma_addr), 1, trace);
    fwrite(&size, sizeof(UINT32), 1, trace);*/

    copy(RECORD_BUFFER, &instruction_count, 0, sizeof(uint64_t) );
    copy(RECORD_BUFFER, &WRITE_OPERATION_CHAR, sizeof(uint64_t), sizeof(char) );
    copy(RECORD_BUFFER, &ma_addr, sizeof(uint64_t) + sizeof(char), sizeof(uint64_t) );
    copy(RECORD_BUFFER, &size, sizeof(uint64_t) + sizeof(char) + sizeof(uint64_t), sizeof(uint32_t) );

   fwrite( RECORD_BUFFER, sizeof(uint64_t) + sizeof(uint64_t) + sizeof(uint32_t) + sizeof(char), 1, trace);

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

VOID Fini(INT32 code, VOID *v)
{
    fclose(trace);
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

#ifdef TRACE_STDOUT
    trace = stdout;
#else
    trace = fopen("pinatrace.out", "wb");
#endif

    instruction_count = 0;

    INS_AddInstrumentFunction(Instruction, 0);
    PIN_AddFiniFunction(Fini, 0);

    // Never returns
    PIN_StartProgram();

    return 0;
}
