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


#include <malloc.h>
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
int* pipe_id;
bool enable_output;
std::vector<void*> allocated_list;

#define PERFORM_EXIT 1
#define PERFORM_READ 2
#define PERFORM_WRITE 4
#define START_DMA 8
#define ISSUE_TLM_MAP 80
#define ISSUE_TLM_FREE 100
#define START_INSTRUCTION 32
#define END_INSTRUCTION 64
#define ISSUE_NOOP 128
#define SWITCH_POOL 110

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

VOID copy(void* dest, const void* input, UINT32 length) {
	for(UINT32 i = 0; i < length; ++i) {
		((char*) dest)[i] = ((char*) input)[i];
	}
}

VOID WriteInstructionRead(ADDRINT* address, UINT32 readSize, THREADID thr) {
	if(enable_output) {
		const uint8_t read_marker = PERFORM_READ;
		uint64_t addr64 = (uint64_t) address;
		uint32_t thrID = (uint32_t) thr;

		const UINT32 BUFFER_LENGTH = sizeof(read_marker) + sizeof(addr64) + sizeof(readSize);

		char buffer[BUFFER_LENGTH];
		copy(&buffer[0], &read_marker, sizeof(read_marker));
		copy(&buffer[sizeof(read_marker)], &addr64, sizeof(addr64));
		copy(&buffer[sizeof(read_marker) + sizeof(addr64)], &readSize, sizeof(readSize));

		assert(thr < core_count);
		write(pipe_id[thr], buffer, BUFFER_LENGTH);
	}
}

VOID WriteInstructionWrite(ADDRINT* address, UINT32 writeSize, THREADID thr) {
	if(enable_output) {
		const uint8_t writer_marker = PERFORM_WRITE;
		uint64_t addr64 = (uint64_t) address;
		uint32_t thrID = (uint32_t) thr;

		size_t write_size;

		const UINT32 BUFFER_LENGTH = sizeof(writer_marker) + sizeof(addr64) + sizeof(writeSize);
		char buffer[BUFFER_LENGTH];

		copy(&buffer[0], &writer_marker, sizeof(writer_marker));
		copy(&buffer[sizeof(writer_marker)], &addr64, sizeof(addr64));
		copy(&buffer[sizeof(writer_marker) + sizeof(addr64)], &writeSize, sizeof(writeSize));

		write(pipe_id[thrID], buffer, BUFFER_LENGTH);
	}
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

	if(enable_output) {
		if(thr < core_count) {
			const uint8_t start_ins     = (uint8_t) START_INSTRUCTION;
		        const uint8_t end_ins       = (uint8_t) END_INSTRUCTION;
			const uint8_t writer_marker = (uint8_t) PERFORM_WRITE;
		        const uint8_t read_marker   = (uint8_t) PERFORM_READ;

			const uint64_t wAddr64 = (uint64_t) writeAddr;
			const uint32_t wSize   = (uint32_t) writeSize;
		        const uint64_t rAddr64 = (uint64_t) readAddr;
		        const uint32_t rSize   = (uint32_t) readSize;

			const uint32_t thrID = (uint32_t) thr;

			const UINT32 BUFFER_LENGTH = (uint32_t) (sizeof(start_ins) + sizeof(end_ins) +
				sizeof(writer_marker) + sizeof(wAddr64) + sizeof(wSize) +
				sizeof(read_marker) + sizeof(rAddr64) + sizeof(rSize));

			char* buffer = (char*) malloc(sizeof(char) * BUFFER_LENGTH);
			int index = 0;

			copy(&buffer[index], &start_ins, sizeof(start_ins));
			index += sizeof(start_ins);
			copy(&buffer[index], &read_marker, sizeof(read_marker));
			index += sizeof(read_marker);
			copy(&buffer[index], &rAddr64, sizeof(rAddr64));
			index += sizeof(rAddr64);
			copy(&buffer[index], &rSize, sizeof(rSize));
			index += sizeof(rSize);
			copy(&buffer[index], &writer_marker, sizeof(writer_marker));
			index += sizeof(writer_marker);
			copy(&buffer[index], &wAddr64, sizeof(wAddr64));
			index += sizeof(wAddr64);
			copy(&buffer[index], &wSize, sizeof(wSize));
			index += sizeof(wSize);
			copy(&buffer[index], &end_ins, sizeof(end_ins));

			assert(thr < core_count);
			write(pipe_id[thrID], buffer, BUFFER_LENGTH);

			free(buffer);
		}
	}

}

VOID WriteInstructionReadOnly(THREADID thr, ADDRINT* readAddr, UINT32 readSize) {

	if(enable_output) {
		if(thr < core_count) {
			const uint8_t start_ins     = (uint8_t) START_INSTRUCTION;
		        const uint8_t end_ins       = (uint8_t) END_INSTRUCTION;
			const uint8_t writer_marker = (uint8_t) PERFORM_WRITE;
		        const uint8_t read_marker   = (uint8_t) PERFORM_READ;

		       	const uint64_t rAddr64 = (uint64_t) readAddr;
		       	const uint32_t rSize   = (uint32_t) readSize;

			const uint32_t thrID = (uint32_t) thr;

		       	const UINT32 BUFFER_LENGTH = (uint32_t) (sizeof(start_ins) + sizeof(end_ins) +
		               	sizeof(read_marker) + sizeof(rAddr64) + sizeof(rSize));

			char* buffer = (char*) malloc(sizeof(char) * BUFFER_LENGTH);
			int index = 0;

		       	copy(&buffer[index], &start_ins, sizeof(start_ins));
			index += sizeof(start_ins);
		       	copy(&buffer[index], &read_marker, sizeof(read_marker));
			index += sizeof(read_marker);
		       	copy(&buffer[index], &rAddr64, sizeof(rAddr64));
			index += sizeof(rAddr64);
		       	copy(&buffer[index], &rSize, sizeof(rSize));
			index += sizeof(rSize);
		        copy(&buffer[index], &end_ins, sizeof(end_ins));

			write(pipe_id[thrID], buffer, BUFFER_LENGTH);
			free(buffer);
		}
	}

}

VOID WriteNoOp(THREADID thr) {
	if(enable_output) {
		if(thr < core_count) {
			const uint8_t noop_marker = ISSUE_NOOP;
			const UINT32 BUFFER_LENGTH = sizeof(noop_marker);

			char* buffer = (char*) malloc(sizeof(char) * BUFFER_LENGTH);
			copy(&buffer[0], &noop_marker, sizeof(noop_marker));

			write(pipe_id[thr], buffer, BUFFER_LENGTH);
			free(buffer);
		}
	}
}

VOID WriteInstructionWriteOnly(THREADID thr, ADDRINT* writeAddr, UINT32 writeSize) {

	if(enable_output) {
		if(thr < core_count) {
			const uint8_t start_ins     = (uint8_t) START_INSTRUCTION;
		        const uint8_t end_ins       = (uint8_t) END_INSTRUCTION;
			const uint8_t writer_marker = (uint8_t) PERFORM_WRITE;
		        const uint8_t read_marker   = (uint8_t) PERFORM_READ;

		       	const uint64_t wAddr64 = (uint64_t) writeAddr;
		       	const uint32_t wSize   = (uint32_t) writeSize;

			const uint32_t thrID = (uint32_t) thr;

		       	const UINT32 BUFFER_LENGTH = (uint32_t) (sizeof(start_ins) + sizeof(end_ins) +
		               	sizeof(writer_marker) + sizeof(wAddr64) + sizeof(wSize));

			char* buffer = (char*) malloc(sizeof(char) * BUFFER_LENGTH);
			int index = 0;

		       	copy(&buffer[index], &start_ins, sizeof(start_ins));
			index += sizeof(start_ins);
		       	copy(&buffer[index], &writer_marker, sizeof(writer_marker));
			index += sizeof(writer_marker);
		       	copy(&buffer[index], &wAddr64, sizeof(wAddr64));
			index += sizeof(wAddr64);
		        copy(&buffer[index], &wSize, sizeof(wSize));
			index += sizeof(wSize);
		        copy(&buffer[index], &end_ins, sizeof(end_ins));

			assert(thr < core_count);
			write(pipe_id[thrID], buffer, BUFFER_LENGTH);
			free(buffer);
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
	} else {
		INS_InsertPredicatedCall(ins, IPOINT_BEFORE, (AFUNPTR)
			WriteNoOp,
			IARG_THREAD_ID,
			IARG_END);
	}

}

int ariel_tlvl_memcpy(void* dest, void* source, size_t size) {
#ifdef ARIEL_DEBUG
	fprintf(stderr, "Perform a tlvl_memcpy from Ariel from %p to %p length %llu\n",
		source, dest, size);
#endif

	char* dest_c = (char*) dest;
	char* src_c  = (char*) source;

	// Perform the memory copy on behalf of the application
	for(size_t i = 0; i < size; ++i) {
		dest_c[i] = src_c[i];
	}

	THREADID currentThread = PIN_ThreadId();
	UINT32 thr = (UINT32) currentThread;

	if(thr >= core_count) {
		fprintf(stderr, "Thread ID: %lu is greater than core count.\n", thr);
		exit(-4);
	}

	const uint8_t issueDMAMarker = (uint8_t) START_DMA;
	const uint64_t ariel_src     = (uint64_t) source;
        const uint64_t ariel_dest    = (uint64_t) dest;
        const uint32_t length        = (uint32_t) size;

        const int BUFFER_LENGTH = sizeof(issueDMAMarker) + sizeof(ariel_src) +
		sizeof(ariel_dest) + sizeof(length);

	char* buffer = (char*) malloc(sizeof(char) * BUFFER_LENGTH);

	copy(&buffer[0], &issueDMAMarker, sizeof(issueDMAMarker));
	copy(&buffer[sizeof(issueDMAMarker)], &ariel_src, sizeof(ariel_src));
	copy(&buffer[sizeof(issueDMAMarker) + sizeof(ariel_src)], &ariel_dest, sizeof(ariel_dest));
	copy(&buffer[sizeof(issueDMAMarker) + sizeof(ariel_src) + sizeof(ariel_dest)],
		&length, sizeof(length));

	write(pipe_id[thr], buffer, BUFFER_LENGTH);

	free(buffer);

#ifdef ARIEL_DEBUG
	fprintf(stderr, "Done with ariel memcpy.\n");
#endif

	return 0;
}

void ariel_tlvl_set_pool(int new_pool) {
#ifdef ARIEL_DEBUG
	fprintf(stderr, "Ariel perform a tlvl_switch_pool to level %d\n", new_pool);
#endif

	THREADID currentThread = PIN_ThreadId();
        UINT32 thr = (UINT32) currentThread;

#ifdef ARIEL_DEBUG
        fprintf(stderr, "Requested: %llu, but expanded to: %llu (on thread: %lu) \n", size, real_req_size,
                thr);
#endif

        const uint8_t  issueSwitch    = (uint8_t) SWITCH_POOL;
        const uint32_t newDefaultPool = (uint32_t) new_pool;

	const size_t BUFFER_LENGTH = sizeof(issueSwitch) + sizeof(newDefaultPool);
	char* buffer = (char*) malloc(sizeof(char) * BUFFER_LENGTH);

	copy(&buffer[0], &issueSwitch, sizeof(issueSwitch));
	copy(&buffer[sizeof(issueSwitch)], &newDefaultPool, sizeof(newDefaultPool));

	write(pipe_id[thr], buffer, BUFFER_LENGTH);
        free(buffer);

	// Keep track of the default pool
	default_pool = (UINT32) new_pool;
}

void* ariel_tlvl_malloc(size_t size, int level) {
#ifdef ARIEL_DEBUG
	fprintf(stderr, "Perform a tlvl_malloc from Ariel %zu, level %d\n", size, level);
#endif
	if(0 == size) {
		fprintf(stderr, "YOU REQUESTED ZERO BYTES\n");
		void *bt_entries[64];
		int entry_returned = backtrace(bt_entries, 64);
		backtrace_symbols_fd(bt_entries, entry_returned, 1);
		exit(-8);
	}

	size_t page_diff = (4096 - (size % ((size_t) 4096)));
	size_t real_req_size = size;

	if(page_diff > 0) {
		real_req_size = size + page_diff;
	}

	THREADID currentThread = PIN_ThreadId();
	UINT32 thr = (UINT32) currentThread;

#ifdef ARIEL_DEBUG
	fprintf(stderr, "Requested: %llu, but expanded to: %llu (on thread: %lu) \n", size, real_req_size,
		thr);
#endif

	void* real_ptr = 0;
	posix_memalign(&real_ptr, 4096, real_req_size);

	const uint8_t  issueTLMMarker = (uint8_t) ISSUE_TLM_MAP;
	const uint64_t virtualAddress = (uint64_t) real_ptr;
	const uint64_t allocationLength = (uint64_t) real_req_size;
	const uint32_t allocationLevel = (uint32_t) level;

	const int BUFFER_LENGTH = sizeof(issueTLMMarker) +
		sizeof(virtualAddress) + sizeof(allocationLength) + sizeof(allocationLevel);

	char* buffer = (char*) malloc(sizeof(char) * BUFFER_LENGTH);

	copy(&buffer[0], &issueTLMMarker, sizeof(issueTLMMarker));
	copy(&buffer[sizeof(issueTLMMarker)], &virtualAddress, sizeof(virtualAddress));
	copy(&buffer[sizeof(issueTLMMarker) + sizeof(virtualAddress)], &allocationLength,
		sizeof(allocationLength));
	copy(&buffer[sizeof(issueTLMMarker) + sizeof(virtualAddress) + sizeof(allocationLength)],
		&allocationLevel, sizeof(allocationLevel));

        write(pipe_id[thr], buffer, BUFFER_LENGTH);

	free(buffer);

#ifdef ARIEL_DEBUG
	fprintf(stderr, "Ariel tlvl_malloc call allocates data at address: 0x%llx\n",
		(uint64_t) real_ptr);
#endif

	allocated_list.push_back(real_ptr);
	return real_ptr;
}

void ariel_tlvl_free(void* ptr) {
	THREADID currentThread = PIN_ThreadId();
	UINT32 thr = (UINT32) currentThread;

#ifdef ARIEL_DEBUG
	fprintf(stderr, "Perform a tlvl_free from Ariel (pointer = %p) on thread %lu\n", ptr, thr);
#endif

	bool found = false;
	std::vector<void*>::iterator ptr_list_itr;
	for(ptr_list_itr = allocated_list.begin(); ptr_list_itr != allocated_list.end(); ptr_list_itr++) {
		if(*ptr_list_itr == ptr) {
			found = true;
			allocated_list.erase(ptr_list_itr);
			break;
		}
	}

	if(found) {
		fprintf(stderr, "ARIEL: Matched call to free, passing to Ariel free routine.\n");
		free(ptr);

		const uint8_t issueFree = (uint8_t) ISSUE_TLM_FREE;
		const uint64_t virtAddr = (uint64_t) ptr;
		const int BUFFER_LENGTH = sizeof(issueFree) + sizeof(virtAddr);

		char* buffer = (char*) malloc(sizeof(char) * BUFFER_LENGTH);
		copy(&buffer[0], &issueFree, sizeof(issueFree));
		copy(&buffer[sizeof(issueFree)], &virtAddr, sizeof(virtAddr));

		write(pipe_id[thr], buffer, BUFFER_LENGTH);
		free(buffer);
	} else {
		fprintf(stderr, "ARIEL: Call to free in Ariel did not find a matching local allocation, this memory will be leaked.\n");
	}

}

void mapped_ariel_enable() {
	fprintf(stderr, "ARIEL: Enabling memory and instruction tracing from program control.\n");
 	enable_output = true;
}

void* ariel_malloc_intercept(size_t size) {
	return ariel_tlvl_malloc(size, default_pool);
}

void* ariel_calloc_intercept(size_t nmembers, size_t member_size) {
	const size_t total_bytes = nmembers * member_size;
	void* result = ariel_tlvl_malloc(total_bytes, default_pool);
	char* result_char = (char*) result;

	// Zero out the memory, as required by the C spec.
	for(size_t i = 0; i < total_bytes; ++i) {
		result_char[i] = (char) 0;
	}

	return result;
}

int ariel_posix_memalign_intercept(void** ptr, size_t alignment, size_t size) {
	*ptr = ariel_tlvl_malloc(size, default_pool);
}

void* ariel_memalign_intercept(size_t alignment, size_t size) {
	return ariel_tlvl_malloc(size, default_pool);
}

void ariel_free_intercept(void* ptr) {
	ariel_tlvl_free(ptr);
}

void* ariel_valloc_intercept(size_t size) {
	return ariel_tlvl_malloc(size, default_pool);
}

void* ariel_pvalloc_intercept(size_t size) {
	return ariel_tlvl_malloc(size, default_pool);
}

void* ariel_aligned_alloc_intercept(size_t alignment, size_t size) {
	return ariel_tlvl_malloc(size, default_pool);
}

void* ariel_realloc_intercept(void* ptr, size_t size) {
	void* realloc_result = realloc(ptr, size);

	// A reallocation event occured, means memory was moved
	if(realloc_result != ptr) {
		std::vector<void*>::iterator ptr_list_itr;

		for(ptr_list_itr = allocated_list.begin(); ptr_list_itr != allocated_list.end(); ptr_list_itr++) {
			if(*ptr_list_itr == ptr) {
				allocated_list.erase(ptr_list_itr);
			}
		}
	}
}

VOID InstrumentRoutine(RTN rtn, VOID* args) {
//	if(SSTVerbosity.Value() > 0) {
//		fprintf(stderr, "ARIEL: Examining routine [%s] for instrumentation\n", RTN_Name(rtn).c_str());
//	}

/*	if(RTN_Name(rtn) == "malloc") {
		// We need to replace with something here
		std::cout << "Identified a malloc replacement function." << std::endl;
	} else if (RTN_Name(rtn) == "tlvl_malloc") {
		// This means malloc far away.
		fprintf(stderr, "Identified routine: tlvl_malloc, replacing with Ariel equivalent...\n");
		RTN_Replace(rtn, (AFUNPTR) ariel_tlvl_malloc);
		fprintf(stderr, "Replacement complete.\n");
	} else if (RTN_Name(rtn) == "tlvl_free") {
		fprintf(stderr, "Identified routine: tlvl_free, replacing with Ariel equivalent...\n");
		RTN_Replace(rtn, (AFUNPTR) ariel_tlvl_free);
		fprintf(stderr, "Replacement complete.\n");
	} else*/ 

	if (RTN_Name(rtn) == "ariel_enable") {
		fprintf(stderr,"Identified routine: ariel_enable, replacing with Ariel equivalent...\n");
		RTN_Replace(rtn, (AFUNPTR) mapped_ariel_enable);
		fprintf(stderr,"Replacement complete.\n");
		return;
    } else if ((InterceptMultiLevelMemory.Value() > 0) && RTN_Name(rtn) == "tlvl_malloc") {
        // This means we want a special malloc to be used (needs a TLB map inside the virtual core)
        fprintf(stderr,"Identified routine: tlvl_malloc, replacing with Ariel equivalent...\n");
        AFUNPTR ret = RTN_Replace(rtn, (AFUNPTR) ariel_tlvl_malloc);
        fprintf(stderr,"Replacement complete. (%p)\n", ret);
    } else if ((InterceptMultiLevelMemory.Value() > 0) && RTN_Name(rtn) == "tlvl_free") {
        fprintf(stderr,"Identified routine: tlvl_free, replacing with Ariel equivalent...\n");
        RTN_Replace(rtn, (AFUNPTR) ariel_tlvl_free);
        fprintf(stderr, "Replacement complete.\n");
    } else if ((InterceptMultiLevelMemory.Value() > 0) && RTN_Name(rtn) == "tlvl_set_pool") {
	fprintf(stderr, "Identifier routine: tlvl_set_pool, replacing with Ariel equivalent...\n");
	RTN_Replace(rtn, (AFUNPTR) ariel_tlvl_set_pool);
	fprintf(stderr, "Replacement complete.\n");
    }
/*
    } else if ((InterceptMultiLevelMemory.Value() > 0) && RTN_Name(rtn) == "malloc") {
	fprintf(stderr, "Identifier routine: malloc, replacing with an Ariel management function...\n");
	RTN_Replace(rtn, (AFUNPTR) ariel_malloc_intercept);
	fprintf(stderr, "Replacement complete.\n");
    } else if ((InterceptMultiLevelMemory.Value() > 0) && RTN_Name(rtn) == "free") {
	fprintf(stderr, "Identifier routine: free, replacing with an Ariel management function...\n");
	RTN_Replace(rtn, (AFUNPTR) ariel_free_intercept);
	fprintf(stderr, "Replacement complete.\n");
    } else if ((InterceptMultiLevelMemory.Value() > 0) && RTN_Name(rtn) == "calloc") {
	fprintf(stderr, "Identifed routine: calloc, replacing with an Ariel management function...\n");
	RTN_Replace(rtn, (AFUNPTR) ariel_calloc_intercept);
	fprintf(stderr, "Replacement complete.\n");
    } else if ((InterceptMultiLevelMemory.Value() > 0) && RTN_Name(rtn) == "posix_memalign") {
	fprintf(stderr, "Identifed routine: posix_memalign, replacing with an Ariel management function...\n");
	RTN_Replace(rtn, (AFUNPTR) ariel_posix_memalign_intercept);
	fprintf(stderr, "Replacement complete.\n");
    } else if ((InterceptMultiLevelMemory.Value() > 0) && RTN_Name(rtn) == "memalign") {
	fprintf(stderr, "Identifed routine: memalign, replacing with an Ariel management function...\n");
	RTN_Replace(rtn, (AFUNPTR) ariel_memalign_intercept);
	fprintf(stderr, "Replacement complete.\n");
    } else if ((InterceptMultiLevelMemory.Value() > 0) && RTN_Name(rtn) == "realloc") {
	fprintf(stderr, "Identifed routine: realloc, replacing with an Ariel management function...\n");
	RTN_Replace(rtn, (AFUNPTR) ariel_realloc_intercept);
	fprintf(stderr, "Replacement complete.\n");
    } else if ((InterceptMultiLevelMemory.Value() > 0) && RTN_Name(rtn) == "valloc") {
	fprintf(stderr, "Identifed routine: valloc, replacing with an Ariel management function...\n");
	RTN_Replace(rtn, (AFUNPTR) ariel_valloc_intercept);
	fprintf(stderr, "Replacement complete.\n");
    } else if ((InterceptMultiLevelMemory.Value() > 0) && RTN_Name(rtn) == "pvalloc") {
	fprintf(stderr, "Identifed routine: pvalloc, replacing with an Ariel management function...\n");
	RTN_Replace(rtn, (AFUNPTR) ariel_pvalloc_intercept);
	fprintf(stderr, "Replacement complete.\n");
    } else if ((InterceptMultiLevelMemory.Value() > 0) && RTN_Name(rtn) == "aligned_alloc") {
	fprintf(stderr, "Identifed routine: aligned_alloc, replacing with an Ariel management function...\n");
	RTN_Replace(rtn, (AFUNPTR) ariel_aligned_alloc_intercept);
	fprintf(stderr, "Replacement complete.\n");
    }
*/
 /*else if (RTN_Name(rtn) == "tlvl_memcpy" ) {
	//	fprintf(stderr, "Identified routine: tlvl_memcpy, replacing with Ariel equivalent...\n");
	//	RTN_Replace(rtn, (AFUNPTR) ariel_tlvl_memcpy);
	//	fprintf(stderr, "Replacement complete.\n");
	}*/
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

    if(InterceptMultiLevelMemory.Value() > 0) {
	mallopt(M_CHECK_ACTION, (int) 1);
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
		fprintf(stderr, "Successfully created write pipe for: %s\n", named_pipe_path_core);
            }
    }

	fprintf(stderr, "ARIEL-SST PIN tool activating with %lu threads\n", core_count);
	fflush(stdout);

    sleep(1);

    default_pool = DefaultMemoryPool.Value();
    fprintf(stderr, "ARIEL: Default memory pool set to %lu\n", default_pool);

    if(StartupMode.Value() == 1) {
	fprintf(stderr, "ARIEL: Tool is configured to begin with profiling immediately.\n");
	enable_output = true;
    } else if (StartupMode.Value() == 0) {
	fprintf(stderr, "ARIEL: Tool is configured to suspend profiling until program control\n");
	enable_output = false;
    }

//    InitLock(&pipe_lock);
    INS_AddInstrumentFunction(InstrumentInstruction, 0);
    RTN_AddInstrumentFunction(InstrumentRoutine, 0);

    fprintf(stderr, "ARIEL: Starting program.\n");
    fflush(stdout);
    PIN_StartProgram();

    return 0;
}

