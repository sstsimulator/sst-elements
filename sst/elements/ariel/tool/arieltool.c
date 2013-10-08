
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
KNOB<UINT32> MaxCoreCount(KNOB_MODE_WRITEONCE, "pintool",
    "c", "1", "Maximum core count to use for data pipes.");

//PIN_LOCK pipe_lock;
UINT32 core_count;
int* pipe_id;

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
	write(pipe_id[0], &command, sizeof(command));

	for(int i = 0; i < core_count; ++i) {
		// Close the pipe and clean up
		close(pipe_id[i]);
	}
}

VOID WriteInstructionRead(ADDRINT* address, UINT32 readSize, THREADID thr) {
	const uint8_t read_marker = PERFORM_READ;
	uint64_t addr64 = (uint64_t) address;
	uint32_t thrID = (uint32_t) thr;

	write(pipe_id[thr], &read_marker, sizeof(read_marker));
	write(pipe_id[thr], &addr64, sizeof(addr64));
	write(pipe_id[thr], &readSize, sizeof(readSize));
	write(pipe_id[thr], &thrID, sizeof(thrID));
}

VOID WriteInstructionWrite(ADDRINT* address, UINT32 writeSize, THREADID thr) {
	const uint8_t writer_marker = PERFORM_WRITE;
	uint64_t addr64 = (uint64_t) address;
	uint32_t thrID = (uint32_t) thr;

	size_t write_size;

	write_size = write(pipe_id[thr], &writer_marker, sizeof(writer_marker));

	if(write_size == -1) {
		perror("Write error to pipe.");
		printf("Error writing to file for thread: %d\n", thr);
		exit(-1);
	} else {
		printf("SUCCESSFULLY wrote to the pipe for thread: %d\n", thr);
	}

	write(pipe_id[thr], &addr64, sizeof(addr64));
	write(pipe_id[thr], &writeSize, sizeof(writeSize));
	write(pipe_id[thr], &thrID, sizeof(thrID));
}

VOID WriteStartInstructionMarker(UINT32 thr) {
	const uint8_t inst_marker = START_INSTRUCTION;
	write(pipe_id[thr], &inst_marker, sizeof(inst_marker));
}

VOID WriteEndInstructionMarker(UINT32 thr) {
	const uint8_t inst_marker = END_INSTRUCTION;
	write(pipe_id[thr], &inst_marker, sizeof(inst_marker));
}

VOID WriteInstructionReadWrite(THREADID thr, ADDRINT* readAddr, UINT32 readSize,
	ADDRINT* writeAddr, UINT32 writeSize) {

//	GetLock(&pipe_lock, (INT32) 0);

	std::cout << "Issuing an instruction R/W for thread: " << thr << std::endl;

	WriteStartInstructionMarker(thr);
	WriteInstructionRead(readAddr, readSize, thr);
	WriteInstructionWrite(writeAddr, writeSize, thr);
	WriteEndInstructionMarker(thr);

//	sync();

//	ReleaseLock(&pipe_lock);
}

VOID WriteInstructionReadOnly(THREADID thr, ADDRINT* readAddr, UINT32 readSize) {

//	GetLock(&pipe_lock, (INT32) 0);

	std::cout << "Issuing an instruction R for thread: " << thr << std::endl;

	WriteStartInstructionMarker(thr);
	WriteInstructionRead(readAddr, readSize, thr);
	WriteEndInstructionMarker(thr);

//	sync();

//	ReleaseLock(&pipe_lock);
}

VOID WriteInstructionWriteOnly(THREADID thr, ADDRINT* writeAddr, UINT32 writeSize) {

//	GetLock(&pipe_lock, (INT32) 0);

	std::cout << "Issuing an instruction W for thread: " << thr << std::endl;

	WriteStartInstructionMarker(thr);
	WriteInstructionWrite(writeAddr, writeSize, thr);
	WriteEndInstructionMarker(thr);

//	sync();

//	ReleaseLock(&pipe_lock);
}

VOID InstrumentInstruction(INS ins, VOID *v)
{
	if( INS_IsMemoryRead(ins) && INS_IsMemoryWrite(ins) ) {
		INS_InsertPredicatedCall(ins, IPOINT_BEFORE, (AFUNPTR)
			WriteInstructionReadWrite,
			IARG_THREAD_ID,
			IARG_MEMORYREAD_EA, IARG_UINT32, INS_MemoryReadSize(ins),
			IARG_MEMORYWRITE_EA, IARG_UINT32, INS_MemoryWriteSize(ins),
			IARG_END);
	} else if( INS_IsMemoryRead(ins) ) {
		INS_InsertPredicatedCall(ins, IPOINT_BEFORE, (AFUNPTR)
			WriteInstructionReadOnly,
			IARG_THREAD_ID,
			IARG_MEMORYREAD_EA, IARG_UINT32, INS_MemoryReadSize(ins),
			IARG_END);
	} else if( INS_IsMemoryWrite(ins) ) {
		INS_InsertPredicatedCall(ins, IPOINT_BEFORE, (AFUNPTR)
			WriteInstructionWriteOnly,
			IARG_THREAD_ID,
			IARG_MEMORYWRITE_EA, IARG_UINT32, INS_MemoryWriteSize(ins),
			IARG_END);
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
    pipe_id = (int*) malloc(sizeof(int) * core_count);

    for(int i = 0; i < core_count; ++i) {
	    const char* named_pipe_path = SSTNamedPipe.Value().c_str();
	    char* named_pipe_path_core = (char*) malloc(sizeof(char) * FILENAME_MAX);
	    sprintf(named_pipe_path_core, "%s-%d", named_pipe_path, i);

	    pipe_id[i] = open(named_pipe_path_core, O_WRONLY);

	    if(pipe_id[i] < 0) {
		fprintf(stderr, "ERROR: Unable to connect to pipe: %s from ARIEL\n",
			named_pipe_path_core);
		exit(-1);
    	    } else {
		printf("Successfully created write pipe for: %s\n", named_pipe_path_core);
            }
    }

//    InitLock(&pipe_lock);
    INS_AddInstrumentFunction(InstrumentInstruction, 0);
    PIN_StartProgram();

    return 0;
}

