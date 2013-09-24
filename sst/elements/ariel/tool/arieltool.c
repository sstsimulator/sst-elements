
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

KNOB<string> SSTNamedPipe(KNOB_MODE_WRITEONCE, "pintool",
    "p", "", "Named pipe to connect to SST simulator");
KNOB<UINT64> MaxInstructions(KNOB_MODE_WRITEONCE, "pintool",
    "i", "10000000000", "Maximum number of instructions to run");
KNOB<UINT32> SSTVerbosity(KNOB_MODE_WRITEONCE, "pintool",
    "v", "0", "SST verbosity level");

int pipe_id;

#define PERFORM_EXIT 1
#define PERFORM_READ 2
#define PERFORM_WRITE 4
#define START_INSTRUCTION 32
#define END_INSTRUCTION 64

VOID Fini(INT32 code, VOID *v)
{
	if(SSTVerbosity.Value() > 0) {
		std::cout << "SSTARIEL: Execution completed, shutting down." << std::endl;
	}

	uint8_t command = PERFORM_EXIT;
	write(pipe_id, &command, sizeof(command));

	// Close the pipe and clean up
	close(pipe_id);
}

VOID WriteInstructionRead(ADDRINT* address, UINT32 readSize) {
	const uint8_t read_marker = PERFORM_READ;
	uint64_t addr64 = (uint64_t) address;

	write(pipe_id, &read_marker, sizeof(read_marker));
	write(pipe_id, &addr64, sizeof(addr64));
	write(pipe_id, &readSize, sizeof(readSize));
}

VOID WriteInstructionWrite(ADDRINT* address, UINT32 writeSize) {
	const uint8_t writer_marker = PERFORM_WRITE;
	uint64_t addr64 = (uint64_t) address;

	write(pipe_id, &writer_marker, sizeof(writer_marker));
	write(pipe_id, &addr64, sizeof(addr64));
	write(pipe_id, &writeSize, sizeof(writeSize));
}

VOID WriteStartInstructionMarker() {
	const uint8_t inst_marker = START_INSTRUCTION;
	write(pipe_id, &inst_marker, sizeof(inst_marker));
}

VOID WriteEndInstructionMarker() {
	const uint8_t inst_marker = END_INSTRUCTION;
	write(pipe_id, &inst_marker, sizeof(inst_marker));
}

VOID InstrumentInstruction(INS ins, VOID *v)
{
	INS_InsertPredicatedCall(ins, IPOINT_BEFORE, (AFUNPTR)
		WriteStartInstructionMarker, IARG_END);

	if(INS_IsMemoryRead(ins)) {
		INS_InsertPredicatedCall(ins, IPOINT_BEFORE, (AFUNPTR)
			WriteInstructionRead, IARG_MEMORYREAD_EA, 
			IARG_UINT32, INS_MemoryReadSize(ins), IARG_END);
	}

	if(INS_IsMemoryWrite(ins)) {
		INS_InsertPredicatedCall(ins, IPOINT_BEFORE, (AFUNPTR)
			WriteInstructionWrite, IARG_MEMORYWRITE_EA, 
			IARG_UINT32, INS_MemoryWriteSize(ins), IARG_END);
	}

	INS_InsertPredicatedCall(ins, IPOINT_BEFORE, (AFUNPTR)
		WriteEndInstructionMarker, IARG_END);
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
		MaxInstructions.Value() << std::endl;
    }

    const char* named_pipe_path = SSTNamedPipe.Value().c_str();
    pipe_id = open(named_pipe_path, O_WRONLY);

    if(pipe_id < 0) {
	fprintf(stderr, "ERROR: Unable to connect to pipe: %s from ARIEL\n",
		named_pipe_path);
	exit(-1);
    }

    INS_AddInstrumentFunction(InstrumentInstruction, 0);
    PIN_StartProgram();

    return 0;
}

