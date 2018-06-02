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

//#include <malloc.h>
#include <execinfo.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include "pin.H"
#include <time.h>
#include <string.h>
#include <sstream>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <map>
#include <stack>
#include <ctime>
#include <bitset>
#include <set>
#include <sst_config.h>

#ifdef HAVE_LIBZ

#include "zlib.h"
#define BT_PRINTF(fmt, args...) gzprintf(btfiles[thr], fmt, ##args);

#else

#define BT_PRINTF(fmt, args...) fprintf(btfiles[thr], fmt, ##args);

#endif

//This must be defined before inclusion of intttypes.h
#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif

#include "ariel_shmem.h"
#include "ariel_inst_class.h"

#undef __STDC_FORMAT_MACROS


using namespace SST::ArielComponent;

KNOB<UINT32> PerformWriteTrace(KNOB_MODE_WRITEONCE, "pintool",
    "w", "0", "Perform write tracing (i.e copy values directly into SST memory operations) (0 = disabled, 1 = enabled)");
KNOB<UINT32> TrapFunctionProfile(KNOB_MODE_WRITEONCE, "pintool",
    "t", "0", "Function profiling level (0 = disabled, 1 = enabled)");
KNOB<string> SSTNamedPipe(KNOB_MODE_WRITEONCE, "pintool",
    "p", "", "Named pipe to connect to SST simulator");
KNOB<UINT64> MaxInstructions(KNOB_MODE_WRITEONCE, "pintool",
    "i", "10000000000", "Maximum number of instructions to run");
KNOB<UINT32> SSTVerbosity(KNOB_MODE_WRITEONCE, "pintool",
    "v", "0", "SST verbosity level");
KNOB<UINT32> MaxCoreCount(KNOB_MODE_WRITEONCE, "pintool",
    "c", "1", "Maximum core count to use for data pipes.");
KNOB<UINT32> StartupMode(KNOB_MODE_WRITEONCE, "pintool",
    "s", "1", "Mode for configuring profile behavior, 1 = start enabled, 0 = start disabled, 2 = attempt auto detect");
KNOB<UINT32> InterceptMemAllocations(KNOB_MODE_WRITEONCE, "pintool",
    "m", "1", "Should intercept multi-level memory allocations, mallocs, and frees, 1 = start enabled, 0 = start disabled");
KNOB<string> UseMallocMap(KNOB_MODE_WRITEONCE, "pintool",
    "u", "", "Should intercept ariel_malloc_flag() and interpret using a malloc map: specify filename or leave blank for disabled");
KNOB<UINT32> KeepMallocStackTrace(KNOB_MODE_WRITEONCE, "pintool",
    "k", "1", "Should keep shadow stack and dump on malloc calls. 1 = enabled, 0 = disabled");
KNOB<UINT32> DefaultMemoryPool(KNOB_MODE_WRITEONCE, "pintool",
    "d", "0", "Default SST Memory Pool");

#define ARIEL_MAX(a,b) \
   ({ __typeof__ (a) _a = (a); __typeof__ (b) _b = (b); _a > _b ? _a : _b; })
   
#define ARIEL_MIN(a,b) \
   ({ __typeof__ (a) _a = (a); __typeof__ (b) _b = (b); _a < _b ? _a : _b; })

typedef struct {
    int64_t insExecuted;
} ArielFunctionRecord;

UINT32 funcProfileLevel;
UINT32 core_count;
UINT32 default_pool;
ArielTunnel *tunnel = NULL;
bool enable_output;
std::vector<void*> allocated_list;
PIN_LOCK mainLock;
PIN_LOCK mallocIndexLock;
UINT64* lastMallocSize;
std::map<std::string, ArielFunctionRecord*> funcProfile;
UINT64* lastMallocLoc;
std::vector< std::set<ADDRINT> > instPtrsList;
UINT32 overridePool;
bool shouldOverride;
bool writeTrace;

// For mlm stuff
// Map each location ID to the set of repeats that should go to fast mem
set<int64_t> fastmemlocs;
// Flag whether to send next intercept malloc to fast memory or not
struct mallocFlagInfo {
    bool valid;
    int count;
    int level;
    int id;
    mallocFlagInfo(bool a, int b, int c, int d) : valid(a), count(b), level(c), id(d) {}
};
std::vector<mallocFlagInfo> toFast;

/****************************************************************/
/********************** SHADOW STACK ****************************/
/* Used by 'sieve' to associate mallocs to the code they        */
/* are called from. Turn on by turning on malloc_stack_trace    */
/****************************************************************/
/****************************************************************/

// Per-thread malloc file -> we don't have to lock the file this way
// Compress it if possible
#ifdef HAVE_LIBZ
std::vector<gzFile> btfiles;
#else
std::vector<FILE*> btfiles;
#endif

UINT64 mallocIndex;
FILE * rtnNameMap;
/* This is a record for each function call */
class StackRecord {
    private:
        ADDRINT stackPtr;
        ADDRINT target;
        ADDRINT instPtr;
    public:
        StackRecord(ADDRINT sp, ADDRINT targ, ADDRINT ip) : stackPtr(sp), target(targ), instPtr(ip) {}
        ADDRINT getStackPtr() const { return stackPtr; }
        ADDRINT getTarget() {return target;}
        ADDRINT getInstPtr() { return instPtr; }
};


std::vector<std::vector<StackRecord> > arielStack; // Per-thread stacks

/* Instrumentation function to be called on function calls */
VOID ariel_stack_call(THREADID thr, ADDRINT stackPtr, ADDRINT target, ADDRINT ip)
{
    // Handle longjmp
    while (arielStack[thr].size() > 0 && stackPtr >= arielStack[thr].back().getStackPtr()) {
        //fprintf(btfiles[thr], "RET ON CALL %s (0x%" PRIx64 ", 0x%" PRIx64 ")\n", RTN_FindNameByAddress(arielStack[thr].back().getTarget()).c_str(), arielStack[thr].back().getInstPtr(), arielStack[thr].back().getStackPtr());
        arielStack[thr].pop_back();
    }
    // Add new record
    arielStack[thr].push_back(StackRecord(stackPtr, target, ip));
    //fprintf(btfiles[thr], "CALL %s (0x%" PRIx64 ", 0x%" PRIx64 ")\n", RTN_FindNameByAddress(target).c_str(), ip, stackPtr);
}

/* Instrumentation function to be called on function returns */
VOID ariel_stack_return(THREADID thr, ADDRINT stackPtr) {
    // Handle longjmp
    while (arielStack[thr].size() > 0 && stackPtr >= arielStack[thr].back().getStackPtr()) {
        //fprintf(btfiles[thr], "RET ON RET %s (0x%" PRIx64 ", 0x%" PRIx64 ")\n", RTN_FindNameByAddress(arielStack[thr].back().getTarget()).c_str(), arielStack[thr].back().getInstPtr(), arielStack[thr].back().getStackPtr());
        arielStack[thr].pop_back();
    }
    // Remove last record
    //fprintf(btfiles[thr], "RET %s (0x%" PRIx64 ", 0x%" PRIx64 ")\n", RTN_FindNameByAddress(arielStack[thr].back().getTarget()).c_str(), arielStack[thr].back().getInstPtr(), arielStack[thr].back().getStackPtr());
    arielStack[thr].pop_back();
}

/* Function to print stack, called by malloc instrumentation code */
VOID ariel_print_stack(UINT32 thr, UINT64 allocSize, UINT64 allocAddr, UINT64 allocIndex)
{

    unsigned int depth = arielStack[thr].size() - 1;
    BT_PRINTF("Malloc,0x%" PRIx64 ",%lu,%" PRIu64 "\n", allocAddr, allocSize, allocIndex);

    vector<ADDRINT> newMappings;
    for (vector<StackRecord>::reverse_iterator rit = arielStack[thr].rbegin(); rit != arielStack[thr].rend(); rit++) {

        // Note this only works if app is compiled with debug on
        if (instPtrsList[thr].find(rit->getInstPtr()) == instPtrsList[thr].end()) {
            newMappings.push_back(rit->getInstPtr());
            instPtrsList[thr].insert(rit->getInstPtr());
        }

        BT_PRINTF("0x%" PRIx64 ",0x%" PRIx64 ",%s", rit->getTarget(), rit->getInstPtr(), ((depth == 0) ? "\n" : ""));
        depth--;
    }
    // Generate any new mappings
    for (std::vector<ADDRINT>::iterator it = newMappings.begin(); it != newMappings.end(); it++) {
        string file;
        int line;
        PIN_LockClient();
            PIN_GetSourceLocation(*it, NULL, &line, &file);
        PIN_UnlockClient();
        BT_PRINTF("MAP: 0x%" PRIx64 ", %s:%d\n", *it, file.c_str(), line);

    }
}

/* Instrument traces to pick up calls and returns */
VOID InstrumentTrace (TRACE trace, VOID* args)
{

    // For checking for jumps into shared libraries
    RTN rtn = TRACE_Rtn(trace);

    // Check each basic block tail
    for (BBL bbl = TRACE_BblHead(trace); BBL_Valid(bbl); bbl = BBL_Next(bbl)) {
        INS tail = BBL_InsTail(bbl);

        if (INS_IsCall(tail)) {
            if (INS_IsDirectBranchOrCall(tail)) {
                ADDRINT target = INS_DirectBranchOrCallTargetAddress(tail);
                INS_InsertPredicatedCall(tail, IPOINT_BEFORE, (AFUNPTR)
                    ariel_stack_call,
                    IARG_THREAD_ID,
                    IARG_REG_VALUE, REG_STACK_PTR,
                    IARG_ADDRINT, target,
                IARG_INST_PTR,
                    IARG_END);
            } else if (!RTN_Valid(rtn) || ".plt" != SEC_Name(RTN_Sec(rtn))) {
                INS_InsertPredicatedCall(tail, IPOINT_BEFORE,
                    (AFUNPTR) ariel_stack_call,
                    IARG_THREAD_ID,
                    IARG_REG_VALUE, REG_STACK_PTR,
                    IARG_BRANCH_TARGET_ADDR,
                IARG_INST_PTR,
                    IARG_END);
            }

        }

        if (RTN_Valid(rtn) && ".plt" == SEC_Name(RTN_Sec(rtn))) {
            INS_InsertCall(tail, IPOINT_BEFORE, (AFUNPTR)
                ariel_stack_call,
                    IARG_THREAD_ID,
                    IARG_REG_VALUE, REG_STACK_PTR,
                    IARG_BRANCH_TARGET_ADDR,
                IARG_INST_PTR,
                    IARG_END);
        }

        if (INS_IsRet(tail)) {
            INS_InsertPredicatedCall(tail, IPOINT_BEFORE, (AFUNPTR)
                    ariel_stack_return,
                    IARG_THREAD_ID,
                    IARG_REG_VALUE, REG_STACK_PTR,
                    IARG_END);
        }
    }
}

/****************************************************************/
/******************** END SHADOW STACK **************************/
/****************************************************************/

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

    if(funcProfileLevel > 0) {
        FILE* funcProfileOutput = fopen("func.profile", "wt");

        for(std::map<std::string, ArielFunctionRecord*>::iterator funcItr = funcProfile.begin();
                                                    funcItr != funcProfile.end(); funcItr++) {
            fprintf(funcProfileOutput, "%s %" PRId64 "\n", funcItr->first.c_str(),
                    funcItr->second->insExecuted);
        }

        fclose(funcProfileOutput);
    }

    // Close backtrace files if needed
    if (KeepMallocStackTrace.Value() == 1) {
        fclose(rtnNameMap);
        for (int i = 0; i < core_count; i++) {
            if (btfiles[i] != NULL)
#ifdef HAVE_LIBZ
                gzclose(btfiles[i]);
#else
                fclose(btfiles[i]);
#endif
        }
    }
}

VOID copy(void* dest, const void* input, UINT32 length)
{
    for(UINT32 i = 0; i < length; ++i) {
        ((char*) dest)[i] = ((char*) input)[i];
    }
}

VOID WriteFlushInstructionMarker(UINT32 thr, ADDRINT ip, ADDRINT vaddr)
{
    ArielCommand ac;
    ac.command = ARIEL_FLUSHLINE_INSTRUCTION; /*  Send the Flush instruction commmand */
    ac.instPtr = (uint64_t) ip;
    ac.flushline.vaddr = (uint32_t) vaddr;

    tunnel->writeMessage(thr, ac);
}

VOID WriteFenceInstructionMarker(UINT32 thr, ADDRINT ip)
{
    ArielCommand ac;
    ac.command = ARIEL_FENCE_INSTRUCTION;
    ac.instPtr = (uint64_t) ip;

    tunnel->writeMessage(thr, ac);
}

VOID WriteInstructionRead(ADDRINT* address, UINT32 readSize, THREADID thr, ADDRINT ip,
            UINT32 instClass, UINT32 simdOpWidth)
{

    const uint64_t addr64 = (uint64_t) address;

    ArielCommand ac;

    ac.command = ARIEL_PERFORM_READ;
    ac.instPtr = (uint64_t) ip;
    ac.inst.addr = addr64;
    ac.inst.size = readSize;
    ac.inst.instClass = instClass;
    ac.inst.simdElemCount = simdOpWidth;

    tunnel->writeMessage(thr, ac);
}

VOID WriteInstructionWrite(ADDRINT* address, UINT32 writeSize, THREADID thr, ADDRINT ip,
            UINT32 instClass, UINT32 simdOpWidth)
{

    const uint64_t addr64 = (uint64_t) address;

    ArielCommand ac;

    ac.command = ARIEL_PERFORM_WRITE;
    ac.instPtr = (uint64_t) ip;
    ac.inst.addr = addr64;
    ac.inst.size = writeSize;
    ac.inst.instClass = instClass;
    ac.inst.simdElemCount = simdOpWidth;
    
    if( writeTrace ) {
//    	if( writeSize > ARIEL_MAX_PAYLOAD_SIZE ) {
//    		fprintf(stderr, "Error: Payload exceeds maximum size (%d > %d)\n",
//    			writeSize, ARIEL_MAX_PAYLOAD_SIZE);
//    		exit(-1);
//    	}
    	PIN_SafeCopy( &ac.inst.payload[0], address, ARIEL_MIN( writeSize, ARIEL_MAX_PAYLOAD_SIZE ) );
	}
	
    tunnel->writeMessage(thr, ac);
}

VOID WriteStartInstructionMarker(UINT32 thr, ADDRINT ip)
{
    ArielCommand ac;
    ac.command = ARIEL_START_INSTRUCTION;
    ac.instPtr = (uint64_t) ip;
    tunnel->writeMessage(thr, ac);
}

VOID WriteEndInstructionMarker(UINT32 thr, ADDRINT ip)
{
    ArielCommand ac;
    ac.command = ARIEL_END_INSTRUCTION;
    ac.instPtr = (uint64_t) ip;
    tunnel->writeMessage(thr, ac);
}

VOID WriteInstructionReadWrite(THREADID thr, ADDRINT* readAddr, UINT32 readSize,
            ADDRINT* writeAddr, UINT32 writeSize, ADDRINT ip, UINT32 instClass,
            UINT32 simdOpWidth )
{

    if(enable_output) {
        if(thr < core_count) {
            WriteStartInstructionMarker( thr, ip );
            WriteInstructionRead(  readAddr,  readSize,  thr, ip, instClass, simdOpWidth );
            WriteInstructionWrite( writeAddr, writeSize, thr, ip, instClass, simdOpWidth );
            WriteEndInstructionMarker( thr, ip );
        }
    }
}

VOID WriteInstructionReadOnly(THREADID thr, ADDRINT* readAddr, UINT32 readSize, ADDRINT ip,
            UINT32 instClass, UINT32 simdOpWidth)
{

    if(enable_output) {
        if(thr < core_count) {
            WriteStartInstructionMarker(thr, ip);
            WriteInstructionRead(  readAddr,  readSize,  thr, ip, instClass, simdOpWidth );
            WriteEndInstructionMarker(thr, ip);
        }
    }

}

VOID WriteNoOp(THREADID thr, ADDRINT ip)
{
    if(enable_output) {
        if(thr < core_count) {
            ArielCommand ac;
            ac.command = ARIEL_NOOP;
            ac.instPtr = (uint64_t) ip;
            tunnel->writeMessage(thr, ac);
        }
    }
}

VOID WriteInstructionWriteOnly(THREADID thr, ADDRINT* writeAddr, UINT32 writeSize, ADDRINT ip,
            UINT32 instClass, UINT32 simdOpWidth)
{

    if(enable_output) {
        if(thr < core_count) {
            WriteStartInstructionMarker(thr, ip);
            WriteInstructionWrite(writeAddr, writeSize,  thr, ip, instClass, simdOpWidth);
            WriteEndInstructionMarker(thr, ip);
        }
    }

}

VOID IncrementFunctionRecord(VOID* funcRecord)
{
    ArielFunctionRecord* arielFuncRec = (ArielFunctionRecord*) funcRecord;

    __asm__ __volatile__(
        "lock incq %0"
        : /* no output registers */
        : "m" (arielFuncRec->insExecuted)
        : "memory"
    );
}

VOID InstrumentInstruction(INS ins, VOID *v)
{
    UINT32 simdOpWidth     = 1;
    UINT32 instClass       = ARIEL_INST_UNKNOWN;
    UINT32 maxSIMDRegWidth = 1;

    std::string instCode = INS_Mnemonic(ins);

    for(UINT32 i = 0; i < INS_MaxNumRRegs(ins); i++) {
        if( REG_is_xmm(INS_RegR(ins, i)) ) {
                maxSIMDRegWidth = ARIEL_MAX(maxSIMDRegWidth, 2);
        } else if ( REG_is_ymm(INS_RegR(ins, i)) ) {
                maxSIMDRegWidth = ARIEL_MAX(maxSIMDRegWidth, 4);
        } else if ( REG_is_zmm(INS_RegR(ins, i)) ) {
                maxSIMDRegWidth = ARIEL_MAX(maxSIMDRegWidth, 8);
        }
    }

    for(UINT32 i = 0; i < INS_MaxNumWRegs(ins); i++) {
        if( REG_is_xmm(INS_RegW(ins, i)) ) {
                maxSIMDRegWidth = ARIEL_MAX(maxSIMDRegWidth, 2);
        } else if ( REG_is_ymm(INS_RegW(ins, i)) ) {
                maxSIMDRegWidth = ARIEL_MAX(maxSIMDRegWidth, 4);
        } else if ( REG_is_zmm(INS_RegW(ins, i)) ) {
                maxSIMDRegWidth = ARIEL_MAX(maxSIMDRegWidth, 8);
        }
    }

    if( instCode.size() > 1 ) {
        std::string prefix = "";

        if( instCode.size() > 2 ) {
            prefix = instCode.substr(0, 3);
        }

        std::string suffix = instCode.substr(instCode.size() - 2);

        if("MOV" == prefix || "mov" == prefix) {
            // Do not found MOV as an FP instruction?
            simdOpWidth = 1;
        } else {
            if( (suffix == "PD") || (suffix == "pd") ) {
                simdOpWidth = maxSIMDRegWidth;
                instClass = ARIEL_INST_DP_FP;
            } else if( (suffix == "PS") || (suffix == "ps") ) {
                simdOpWidth = maxSIMDRegWidth * 2;
                instClass = ARIEL_INST_SP_FP;
            } else if( (suffix == "SD") || (suffix == "sd") ) {
                simdOpWidth = 1;
                instClass = ARIEL_INST_DP_FP;
            } else if ( (suffix == "SS") || (suffix == "ss") ) {
                simdOpWidth = 1;
                instClass = ARIEL_INST_SP_FP;
            } else {
                simdOpWidth = 1;
            }
        }
    }

    if( INS_IsMemoryRead(ins) && INS_IsMemoryWrite(ins) ) {
        INS_InsertPredicatedCall(ins, IPOINT_BEFORE, (AFUNPTR)
                WriteInstructionReadWrite,
                IARG_THREAD_ID,
                IARG_MEMORYREAD_EA, IARG_UINT32, INS_MemoryReadSize(ins),
                IARG_MEMORYWRITE_EA, IARG_UINT32, INS_MemoryWriteSize(ins),
                IARG_INST_PTR,
                IARG_UINT32, instClass,
                IARG_UINT32, simdOpWidth,
                IARG_END);
    } else if( INS_IsMemoryRead(ins) ) {
        INS_InsertPredicatedCall(ins, IPOINT_BEFORE, (AFUNPTR)
                WriteInstructionReadOnly,
                IARG_THREAD_ID,
                IARG_MEMORYREAD_EA, IARG_UINT32, INS_MemoryReadSize(ins),
                IARG_INST_PTR,
                IARG_UINT32, instClass,
                IARG_UINT32, simdOpWidth,
                IARG_END);
    } else if( INS_IsMemoryWrite(ins) ) {
        INS_InsertPredicatedCall(ins, IPOINT_BEFORE, (AFUNPTR)
                WriteInstructionWriteOnly,
                IARG_THREAD_ID,
                IARG_MEMORYWRITE_EA, IARG_UINT32, INS_MemoryWriteSize(ins),
                IARG_INST_PTR,
                IARG_UINT32, instClass,
                IARG_UINT32, simdOpWidth,
                IARG_END);
    } else {
        INS_InsertPredicatedCall(ins, IPOINT_BEFORE, (AFUNPTR)
                WriteNoOp,
                IARG_THREAD_ID,
                IARG_INST_PTR,
                IARG_END);
    }

    if(funcProfileLevel > 0) {
        RTN rtn = INS_Rtn(ins);
        std::string rtn_name = "Unknown Function";

        if(RTN_Valid(rtn)) {
            rtn_name = RTN_Name(rtn);
        }

        std::map<std::string, ArielFunctionRecord*>::iterator checkExists =
                funcProfile.find(rtn_name);
        ArielFunctionRecord* funcRecord = NULL;

        if(checkExists == funcProfile.end()) {
            funcRecord = new ArielFunctionRecord();
            funcProfile.insert( std::pair<std::string, ArielFunctionRecord*>(rtn_name,
                funcRecord) );
        } else {
            funcRecord = checkExists->second;
        }

        INS_InsertPredicatedCall(ins, IPOINT_BEFORE, (AFUNPTR) IncrementFunctionRecord,
                IARG_PTR, (void*) funcRecord, IARG_END);
    }
}

void mapped_ariel_enable()
{
    fprintf(stderr, "ARIEL: Enabling memory and instruction tracing from program control.\n");
    fflush(stdout);
    fflush(stderr);
    enable_output = true;
}

uint64_t mapped_ariel_cycles()
{
    return tunnel->getCycles();
}

int mapped_gettimeofday(struct timeval *tp, void *tzp)
{
    if ( tp == NULL ) { errno = EINVAL ; return -1; }

    tunnel->getTime(tp);
    return 0;
}

void mapped_ariel_output_stats()
{
    THREADID thr = PIN_ThreadId();
    ArielCommand ac;
    ac.command = ARIEL_OUTPUT_STATS;
    ac.instPtr = (uint64_t) 0;
    tunnel->writeMessage(thr, ac);
}

// same effect as mapped_ariel_output_stats(), but it also sends a user-defined reference number back
void mapped_ariel_output_stats_buoy(uint64_t marker)
{
    THREADID thr = PIN_ThreadId();
    ArielCommand ac;
    ac.command = ARIEL_OUTPUT_STATS;
    ac.instPtr = (uint64_t) marker; //user the instruction pointer slot to send the marker number
    tunnel->writeMessage(thr, ac);
}

#if ! defined(__APPLE__)
int mapped_clockgettime(clockid_t clock, struct timespec *tp)
{
    if (tp == NULL) { errno = EINVAL; return -1; }
    tunnel->getTimeNs(tp);
    return 0;
}
#endif

void mapped_ariel_flushline(void *virtualAddress)
{
    THREADID currentThread = PIN_ThreadId();
    UINT32 thr = (UINT32) currentThread;
    ADDRINT ip = IARG_INST_PTR;
    ADDRINT vaddr = (uint64_t) virtualAddress;

    WriteFlushInstructionMarker(thr, ip, vaddr);
}

void mapped_ariel_fence(void *virtualAddress)
{
    THREADID currentThread = PIN_ThreadId();
    UINT32 thr = (UINT32) currentThread;
    ADDRINT ip = IARG_INST_PTR;

    WriteFenceInstructionMarker(thr, ip);
}

int ariel_mlm_memcpy(void* dest, void* source, size_t size) {
#ifdef ARIEL_DEBUG
    fprintf(stderr, "Perform a mlm_memcpy from Ariel from %p to %p length %llu\n",
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
        fprintf(stderr, "Thread ID: %" PRIu32 " is greater than core count.\n", thr);
        exit(-4);
    }

    const uint64_t ariel_src     = (uint64_t) source;
    const uint64_t ariel_dest    = (uint64_t) dest;
    const uint32_t length        = (uint32_t) size;

    ArielCommand ac;
    ac.command = ARIEL_START_DMA;
    ac.dma_start.src = ariel_src;
    ac.dma_start.dest = ariel_dest;
    ac.dma_start.len = length;

    tunnel->writeMessage(thr, ac);

#ifdef ARIEL_DEBUG
    fprintf(stderr, "Done with ariel memcpy.\n");
#endif

    return 0;
}

void ariel_mlm_set_pool(int new_pool)
{
#ifdef ARIEL_DEBUG
    fprintf(stderr, "Ariel perform a mlm_switch_pool to level %d\n", new_pool);
#endif

    THREADID currentThread = PIN_ThreadId();
    UINT32 thr = (UINT32) currentThread;

// #ifdef ARIEL_DEBUG
//     fprintf(stderr, "Requested: %llu, but expanded to: %llu (on thread: %lu) \n", size, real_req_size,
//             thr);
// #endif

    const uint32_t newDefaultPool = (uint32_t) new_pool;

    ArielCommand ac;
    ac.command = ARIEL_SWITCH_POOL;
    ac.switchPool.pool = newDefaultPool;
    tunnel->writeMessage(thr, ac);

    // Keep track of the default pool
    default_pool = (UINT32) new_pool;
}



void* ariel_mmap_mlm(int fileID, size_t size, int level)
{

    THREADID currentThread = PIN_ThreadId();
    UINT32 thr = (UINT32) currentThread;

#ifdef ARIEL_DEBUG
    fprintf(stderr, "%u: Perform a mmap_mlm from Ariel %zu, level %d\n",
            thr, size, level);
#endif

    if(0 == size)
    {
        fprintf(stderr, "YOU REQUESTED ZERO BYTES\n");
        void *bt_entries[64];
        int entry_returned = backtrace(bt_entries, 64);
        backtrace_symbols(bt_entries, entry_returned);
        exit(-8);
    }

    size_t page_diff = size % ((size_t)4096);
    size_t npages = size / ((size_t)4096);

    size_t real_req_size = 4096 * (npages + ((page_diff == 0) ? 0 : 1));

#ifdef ARIEL_DEBUG
    fprintf(stderr, "Requested: %llu, but expanded to: %llu (on thread: %lu) \n",
            size, real_req_size, thr);
#endif

    void* real_ptr = 0;
    posix_memalign(&real_ptr, 4096, real_req_size);

    const uint64_t virtualAddress = (uint64_t) real_ptr;
    const uint64_t allocationLength = (uint64_t) real_req_size;
    const uint32_t allocationLevel = (uint32_t) level;

    ArielCommand ac;
    ac.command = ARIEL_ISSUE_TLM_MMAP;
    ac.mlm_mmap.vaddr = virtualAddress;
    ac.mlm_mmap.alloc_len = allocationLength;
    ac.mlm_mmap.alloc_level = allocationLevel;
    ac.mlm_mmap.fileID = fileID;
    std::cout<<"Before ******"<<std::endl;
    std::cout<<"File ID at FESIMPLE IS : "<<ac.mlm_mmap.fileID<<std::endl;
    std::cout<<"After ******"<<std::endl;

    tunnel->writeMessage(thr, ac);

#ifdef ARIEL_DEBUG
    fprintf(stderr, "%u: Ariel mmap_mlm call allocates data at address: 0x%llx\n",
            thr, (uint64_t) real_ptr);
#endif

    PIN_GetLock(&mainLock, thr);
        allocated_list.push_back(real_ptr);
    PIN_ReleaseLock(&mainLock);
        return real_ptr;
}


void* ariel_mlm_malloc(size_t size, int level) {
    THREADID currentThread = PIN_ThreadId();
    UINT32 thr = (UINT32) currentThread;

#ifdef ARIEL_DEBUG
    fprintf(stderr, "%u: Perform a mlm_malloc from Ariel %zu, level %d\n", thr, size, level);
#endif

    if(0 == size) {
        fprintf(stderr, "YOU REQUESTED ZERO BYTES\n");
        void *bt_entries[64];
        int entry_returned = backtrace(bt_entries, 64);
        backtrace_symbols(bt_entries, entry_returned);
        exit(-8);
    }

    size_t page_diff = size % ((size_t)4096);
    size_t npages = size / ((size_t)4096);

    size_t real_req_size = 4096 * (npages + ((page_diff == 0) ? 0 : 1));

#ifdef ARIEL_DEBUG
    fprintf(stderr, "Requested: %llu, but expanded to: %llu (on thread: %lu) \n",
            size, real_req_size, thr);
#endif

    void* real_ptr = 0;
    posix_memalign(&real_ptr, 4096, real_req_size);

    const uint64_t virtualAddress = (uint64_t) real_ptr;
    const uint64_t allocationLength = (uint64_t) real_req_size;
    const uint32_t allocationLevel = (uint32_t) level;

    ArielCommand ac;
    ac.command = ARIEL_ISSUE_TLM_MAP;
    ac.mlm_map.vaddr = virtualAddress;
    ac.mlm_map.alloc_len = allocationLength;

    if(shouldOverride) {
        ac.mlm_map.alloc_level = overridePool;
    } else {
        ac.mlm_map.alloc_level = allocationLevel;
    }

    tunnel->writeMessage(thr, ac);

#ifdef ARIEL_DEBUG
    fprintf(stderr, "%u: Ariel mlm_malloc call allocates data at address: 0x%llx\n",
            thr, (uint64_t) real_ptr);
#endif

    PIN_GetLock(&mainLock, thr);
    allocated_list.push_back(real_ptr);
    PIN_ReleaseLock(&mainLock);
    return real_ptr;
}

void ariel_mlm_free(void* ptr)
{
    THREADID currentThread = PIN_ThreadId();
    UINT32 thr = (UINT32) currentThread;

#ifdef ARIEL_DEBUG
    fprintf(stderr, "Perform a mlm_free from Ariel (pointer = %p) on thread %lu\n", ptr, thr);
#endif

    bool found = false;
    std::vector<void*>::iterator ptr_list_itr;
    PIN_GetLock(&mainLock, thr);
    for(ptr_list_itr = allocated_list.begin(); ptr_list_itr != allocated_list.end(); ptr_list_itr++) {
        if(*ptr_list_itr == ptr) {
            found = true;
            allocated_list.erase(ptr_list_itr);
            break;
        }
    }
    PIN_ReleaseLock(&mainLock);

    if(found) {
#ifdef ARIEL_DEBUG
        fprintf(stderr, "ARIEL: Matched call to free, passing to Ariel free routine.\n");
#endif
        free(ptr);

        const uint64_t virtAddr = (uint64_t) ptr;

        ArielCommand ac;
        ac.command = ARIEL_ISSUE_TLM_FREE;
        ac.mlm_free.vaddr = virtAddr;
        tunnel->writeMessage(thr, ac);

    } else {
        fprintf(stderr, "ARIEL: Call to free in Ariel did not find a matching local allocation, this memory will be leaked.\n");
    }
}

VOID ariel_premalloc_instrument(ADDRINT allocSize, ADDRINT ip)
{
    THREADID currentThread = PIN_ThreadId();
    UINT32 thr = (UINT32) currentThread;

    lastMallocSize[thr] = (UINT64) allocSize;
    lastMallocLoc[thr] = (UINT64) ip;
}

VOID ariel_postmalloc_instrument(ADDRINT allocLocation)
{
    if(lastMallocSize >= 0) {
        THREADID currentThread = PIN_ThreadId();
        UINT32 thr = (UINT32) currentThread;

        const uint64_t virtualAddress = (uint64_t) allocLocation;
        const uint64_t allocationLength = (uint64_t) lastMallocSize[thr];
        const uint32_t allocationLevel = (uint32_t) default_pool;

        uint64_t myIndex = 0;
        // Dump stack if we need it
        if (KeepMallocStackTrace.Value() == 1) {
            PIN_GetLock(&mallocIndexLock, thr);
            myIndex = mallocIndex;
            mallocIndex++;
            PIN_ReleaseLock(&mallocIndexLock);
            ariel_print_stack(thr, allocationLength, allocLocation, myIndex);
        }

        ArielCommand ac;
        ac.command = ARIEL_ISSUE_TLM_MAP;
        ac.instPtr = myIndex;
        ac.mlm_map.vaddr = virtualAddress;
        ac.mlm_map.alloc_len = allocationLength;


        if (UseMallocMap.Value() != "") {
            if (toFast[thr].valid) {
                ac.mlm_map.alloc_level = toFast[thr].level;
                ac.instPtr = toFast[thr].id;
                toFast[thr].count--;

                if (toFast[thr].count == 0) {
                    toFast[thr].valid = false;
                }
                tunnel->writeMessage(thr, ac);
            }
        } else if (shouldOverride) {
            ac.mlm_map.alloc_level = overridePool;
            tunnel->writeMessage(thr, ac);
        } else if (InterceptMemAllocations.Value()) {
            ac.mlm_map.alloc_level = allocationLevel;
            tunnel->writeMessage(thr, ac);
        }

        /*printf("ARIEL: Created a malloc of size: %" PRIu64 " in Ariel\n",
         * (UINT64) allocationLength);*/
    }
}

VOID ariel_postfree_instrument(ADDRINT allocLocation)
{
    THREADID currentThread = PIN_ThreadId();
    UINT32 thr = (UINT32) currentThread;

    const uint64_t virtAddr = (uint64_t) allocLocation;

    ArielCommand ac;
    ac.command = ARIEL_ISSUE_TLM_FREE;
    ac.mlm_free.vaddr = virtAddr;
    tunnel->writeMessage(thr, ac);
}

void mapped_ariel_malloc_flag_fortran(int* mallocLocId, int* count, int* level)
{
    THREADID thr = PIN_ThreadId();
    int64_t id = (int64_t) *mallocLocId;
    // Set malloc flag for this thread
    if (fastmemlocs.find(id) != fastmemlocs.end()) {
        toFast[thr].valid = true;
        toFast[thr].count = *count;
        toFast[thr].level = *level;
        toFast[thr].id = id;
    } else {
        toFast[thr].valid = false;
    }
}

void mapped_ariel_malloc_flag(int64_t mallocLocId, int count, int level)
{
    THREADID thr = PIN_ThreadId();

    // Set malloc flag for this thread
    if (fastmemlocs.find(mallocLocId) != fastmemlocs.end()) {
        toFast[thr].valid = true;
        toFast[thr].count = count;
        toFast[thr].level = level;
        toFast[thr].id = mallocLocId;
    } else {
        toFast[thr].valid = false;
    }
}

VOID InstrumentRoutine(RTN rtn, VOID* args)
{
    if (KeepMallocStackTrace.Value() == 1) {
        fprintf(rtnNameMap, "0x%" PRIx64 ", %s\n", RTN_Address(rtn), RTN_Name(rtn).c_str());
    }

    if (RTN_Name(rtn) == "ariel_enable" || RTN_Name(rtn) == "_ariel_enable" || RTN_Name(rtn) == "__arielfort_MOD_ariel_enable") {
        fprintf(stderr,"Identified routine: ariel_enable, replacing with Ariel equivalent...\n");
        RTN_Replace(rtn, (AFUNPTR) mapped_ariel_enable);
        fprintf(stderr,"Replacement complete.\n");
        if (StartupMode.Value() == 2) {
            fprintf(stderr, "Tool was called with auto-detect enable mode, setting initial output to not be traced.\n");
            enable_output = false;
        }
        return;
    } else if (RTN_Name(rtn) == "gettimeofday" || RTN_Name(rtn) == "_gettimeofday") {
        fprintf(stderr,"Identified routine: gettimeofday, replacing with Ariel equivalent...\n");
        RTN_Replace(rtn, (AFUNPTR) mapped_gettimeofday);
        fprintf(stderr,"Replacement complete.\n");
        return;
    } else if (RTN_Name(rtn) == "ariel_cycles" || RTN_Name(rtn) == "_ariel_cycles") {
        fprintf(stderr, "Identified routine: ariel_cycles, replacing with Ariel equivalent..\n");
        RTN_Replace(rtn, (AFUNPTR) mapped_ariel_cycles);
        fprintf(stderr, "Replacement complete\n");
        return;
#if ! defined(__APPLE__)
    } else if (RTN_Name(rtn) == "clock_gettime" || RTN_Name(rtn) == "_clock_gettime" ||
        RTN_Name(rtn) == "__clock_gettime") {
        fprintf(stderr,"Identified routine: clock_gettime, replacing with Ariel equivalent...\n");
        RTN_Replace(rtn, (AFUNPTR) mapped_clockgettime);
        fprintf(stderr,"Replacement complete.\n");
        return;
#endif
    } else if ((InterceptMemAllocations.Value() > 0) && RTN_Name(rtn) == "mlm_malloc") {
        // This means we want a special malloc to be used (needs a TLB map inside the virtual core)
        fprintf(stderr,"Identified routine: mlm_malloc, replacing with Ariel equivalent...\n");
        AFUNPTR ret = RTN_Replace(rtn, (AFUNPTR) ariel_mlm_malloc);
        fprintf(stderr,"Replacement complete. (%p)\n", ret);
        return;
    } else if ((InterceptMemAllocations.Value() > 0) && RTN_Name(rtn) == "mlm_free") {
        fprintf(stderr,"Identified routine: mlm_free, replacing with Ariel equivalent...\n");
        RTN_Replace(rtn, (AFUNPTR) ariel_mlm_free);
        fprintf(stderr, "Replacement complete.\n");
        return;
    } else if ((InterceptMemAllocations.Value() > 0) && RTN_Name(rtn) == "mlm_set_pool") {
        fprintf(stderr, "Identified routine: mlm_set_pool, replacing with Ariel equivalent...\n");
        RTN_Replace(rtn, (AFUNPTR) ariel_mlm_set_pool);
        fprintf(stderr, "Replacement complete.\n");
        return;
    } else if ((InterceptMemAllocations.Value() > 0) && (
                RTN_Name(rtn) == "malloc" || RTN_Name(rtn) == "_malloc" || RTN_Name(rtn) == "__libc_malloc" || RTN_Name(rtn) == "__libc_memalign" || RTN_Name(rtn) == "_gfortran_malloc")) {

        fprintf(stderr, "Identified routine: malloc/_malloc, replacing with Ariel equivalent...\n");
        RTN_Open(rtn);

        RTN_InsertCall(rtn, IPOINT_BEFORE,
            (AFUNPTR) ariel_premalloc_instrument,
                IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
                IARG_INST_PTR,
                IARG_END);

        RTN_InsertCall(rtn, IPOINT_AFTER,
                       (AFUNPTR) ariel_postmalloc_instrument,
                       IARG_FUNCRET_EXITPOINT_VALUE,
                       IARG_END);

        RTN_Close(rtn);
    } else if ((InterceptMemAllocations.Value() > 0) && (
                RTN_Name(rtn) == "free" || RTN_Name(rtn) == "_free" || RTN_Name(rtn) == "__libc_free" || RTN_Name(rtn) == "_gfortran_free")) {

        fprintf(stderr, "Identified routine: free/_free, replacing with Ariel equivalent...\n");
        RTN_Open(rtn);

        RTN_InsertCall(rtn, IPOINT_BEFORE,
            (AFUNPTR) ariel_postfree_instrument,
                IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
                IARG_END);

        RTN_Close(rtn);
    } else if (RTN_Name(rtn) == "ariel_output_stats" || RTN_Name(rtn) == "_ariel_output_stats" || RTN_Name(rtn) == "__arielfort_MOD_ariel_output_stats") {
        fprintf(stderr, "Identified routine: ariel_output_stats, replacing with Ariel equivalent..\n");
        RTN_Replace(rtn, (AFUNPTR) mapped_ariel_output_stats);
        fprintf(stderr, "Replacement complete\n");
        return;
    } else if (RTN_Name(rtn) == "ariel_output_stats_buoy" || RTN_Name(rtn) == "_ariel_output_stats_buoy") {
        fprintf(stderr, "Identified routine: ariel_output_stats_buoy, replacing with Ariel equivalent..\n");
        RTN_Replace(rtn, (AFUNPTR) mapped_ariel_output_stats_buoy);
        fprintf(stderr, "Replacement complete\n");
        return;
    } else if (RTN_Name(rtn) == "ariel_flushline" || RTN_Name(rtn) == "_ariel_flushline") {
        fprintf(stderr, "Identified routine: ariel_flushline, replacing with Ariel equivalent..\n");
        RTN_Replace(rtn, (AFUNPTR) mapped_ariel_flushline);
        fprintf(stderr, "Replacement complete\n");
        return;
    } else if (RTN_Name(rtn) == "ariel_fence" || RTN_Name(rtn) == "_ariel_fence") {
        fprintf(stderr, "Identified routine: ariel_fence, replacing with Ariel equivalent..\n");
        RTN_Replace(rtn, (AFUNPTR) mapped_ariel_fence);
        fprintf(stderr, "Replacement complete\n");
        return;
    } else if (RTN_Name(rtn) == "ariel_mmap_mlm" || RTN_Name(rtn) == "_ariel_mmap_mlm") {
        AFUNPTR ret = RTN_Replace(rtn, (AFUNPTR) ariel_mmap_mlm);
        return;
    } else if (RTN_Name(rtn) == "ariel_munmap_mlm" || RTN_Name(rtn) == "_ariel_munmap_mlm") {

        return;
    } else if (UseMallocMap.Value() != "") {
        if (RTN_Name(rtn) == "ariel_malloc_flag" || RTN_Name(rtn) == "_ariel_malloc_flag") {

            fprintf(stderr, "Identified routine: ariel_malloc_flag, replacing with Ariel equivalent..\n");
            RTN_Replace(rtn, (AFUNPTR) mapped_ariel_malloc_flag);
            return;

        } else if (RTN_Name(rtn) == "__arielfort_MOD_ariel_malloc_flag") {
            fprintf(stderr, "Identified routine: ariel_malloc_flag, replacing with Ariel equivalent..\n");
            RTN_Replace(rtn, (AFUNPTR) mapped_ariel_malloc_flag_fortran);
            return;
        }
    }
}

void loadFastMemLocations()
{
    std::ifstream infile(UseMallocMap.Value().c_str());
    int64_t val;
    while (infile >> val) {
        fastmemlocs.insert(val);
    }
    infile.close();
}

/*(===================================================================== */
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
    //PIN_InitSymbolsAlt(IFUNC_SYMBOLS);
    PIN_InitSymbols();
    PIN_AddFiniFunction(Fini, 0);

    PIN_InitLock(&mainLock);
    PIN_InitLock(&mallocIndexLock);

    if(SSTVerbosity.Value() > 0) {
        std::cout << "SSTARIEL: Loading Ariel Tool to connect to SST on pipe: " <<
            SSTNamedPipe.Value() << " max instruction count: " <<
            MaxInstructions.Value() <<
            " max core count: " << MaxCoreCount.Value() << std::endl;
    }

    funcProfileLevel = TrapFunctionProfile.Value();

    if(funcProfileLevel > 0) {
        std::cout << "SSTARIEL: Function profile level is configured to: " << funcProfileLevel << std::endl;
    } else {
        std::cout << "SSTARIEL: Function profiling is disabled." << std::endl;
    }

    char* override_pool_name = getenv("ARIEL_OVERRIDE_POOL");
    if(NULL != override_pool_name) {
        fprintf(stderr, "ARIEL-SST: Override for memory pools\n");
        shouldOverride = true;
        overridePool = (UINT32) atoi(getenv("ARIEL_OVERRIDE_POOL"));
        fprintf(stderr, "ARIEL-SST: Use pool: %" PRIu32 " instead of application provided\n", overridePool);
    } else {
        fprintf(stderr, "ARIEL-SST: Did not find ARIEL_OVERRIDE_POOL in the environment, no override applies.\n");
    }
    
    if(PerformWriteTrace.Value() > 0) {
    	writeTrace = true;
    }
    
    if( writeTrace ) {
    	if( SSTVerbosity.Value() > 0 ) {
    		printf("SSTARIEL: Performing write tracing (this is an expensive operation.)\n");
    	}
    }

    core_count = MaxCoreCount.Value();

    tunnel = new ArielTunnel(SSTNamedPipe.Value());
    lastMallocSize = (UINT64*) malloc(sizeof(UINT64) * core_count);
    lastMallocLoc = (UINT64*) malloc(sizeof(UINT64) * core_count);
    mallocIndex = 0;

    if (KeepMallocStackTrace.Value() == 1) {
        arielStack.resize(core_count);  // Need core_count stacks
        rtnNameMap = fopen("routine_name_map.txt", "wt");
        instPtrsList.resize(core_count);    // Need core_count sets of instruction pointers (to avoid locks)
    }

    for(int i = 0; i < core_count; i++) {
        lastMallocSize[i] = (UINT64) 0;
        lastMallocLoc[i] = (UINT64) 0;

        // Shadow stack - open per-thread backtrace file
        if (KeepMallocStackTrace.Value() == 1) {
            stringstream fn;
            fn << "backtrace_" << i << ".txt";
            string filename(fn.str());
#ifdef HAVE_LIBZ
            filename += ".gz";
            btfiles.push_back(gzopen(filename.c_str(), "w"));
#else
            btfiles.push_back(fopen(filename.c_str(), "w"));
#endif
            if (btfiles.back() == NULL) {
                fprintf(stderr, "ARIEL ERROR: could not create and/or open backtrace file: %s\n", filename.c_str());
                return 73;  // EX_CANTCREATE
            }
        }
    }

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
    } else if (StartupMode.Value() == 2) {
        fprintf(stderr, "ARIEL: Tool is configured to attempt auto detect of profiling\n");
        fprintf(stderr, "ARIEL: Initial mode will be to enable profiling unless ariel_enable function is located\n");
        enable_output = true;
    }

    INS_AddInstrumentFunction(InstrumentInstruction, 0);
    RTN_AddInstrumentFunction(InstrumentRoutine, 0);

    // Instrument traces to capture stack
    if (KeepMallocStackTrace.Value() == 1)
        TRACE_AddInstrumentFunction(InstrumentTrace, 0);

    if (UseMallocMap.Value() != "") {
        loadFastMemLocations();
        for (int i = 0; i < core_count; i++) {
            toFast.push_back(mallocFlagInfo(false, 0, 0, 0));
        }
    }

    fprintf(stderr, "ARIEL: Starting program.\n");
    fflush(stdout);
    PIN_StartProgram();

    return 0;
}

