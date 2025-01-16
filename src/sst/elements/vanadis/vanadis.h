// Copyright 2009-2024 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2024, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _VANADIS_COMPONENT_H
#define _VANADIS_COMPONENT_H

#include "datastruct/cqueue.h"
#include "decoder/vdecoder.h"
#include "inst/isatable.h"
#include "inst/regfile.h"
#include "inst/regstack.h"
#include "inst/vinst.h"
#include "lsq/vlsq.h"
#include "lsq/vbasiclsq.h"
#include "velf/velfinfo.h"
#include "vfpflags.h"
#include "vfuncunit.h"

#include "os/vgetthreadstate.h"
#include "os/vdumpregsreq.h"
#include "os/vcheckpointreq.h"

#include <array>
#include <limits>
#include <set>
#include <sst/core/component.h>
#include <sst/core/interfaces/stdMem.h>
#include <sst/core/link.h>
#include <sst/core/output.h>
#include <sst/core/params.h>


namespace SST {
namespace Vanadis {

#ifdef VANADIS_BUILD_DEBUG
#define VANADIS_COMPONENT VanadisDebugComponent
#else
#define VANADIS_COMPONENT VanadisComponent
#endif

class VanadisInsCacheLoadRecord
{
public:
    VanadisInsCacheLoadRecord(const uint32_t thr, const uint64_t addrStart, const uint16_t len) :
        hw_thr(thr),
        addr(addrStart),
        width(len),
        hasPayload(false)
    {

        data = new uint8_t[len];

        for ( uint16_t i = 0; i < width; ++i ) {
            data[i] = 0;
        }
    }

    uint64_t getAddress() const { return addr; }
    uint32_t getHWThread() const { return hw_thr; }
    uint16_t getWidth() const { return width; }
    bool     hasData() const { return hasPayload; }
    uint8_t* getPayload() { return data; }

    void setPayload(uint8_t* ptr)
    {
        hasPayload = true;

        for ( uint16_t i = 0; i < width; ++i ) {
            data[i] = ptr[i];
        }
    }

private:
    bool           hasPayload;
    uint8_t*       data;
    const uint64_t addr;
    const uint16_t width;
    const uint32_t hw_thr;
};

#ifdef VANADIS_BUILD_DEBUG
class VanadisDebugComponent : public SST::Component
{
#else
class VanadisComponent : public SST::Component
{
#endif

public:
    SST_ELI_REGISTER_COMPONENT(
#ifdef VANADIS_BUILD_DEBUG

        VanadisDebugComponent, "vanadis", "dbg_VanadisCPU", SST_ELI_ELEMENT_VERSION(1, 0, 0),
        "Vanadis Debug Processor Component",
#else
        VanadisComponent, "vanadis", "VanadisCPU", SST_ELI_ELEMENT_VERSION(1, 0, 0), "Vanadis Processor Component",
#endif
        COMPONENT_CATEGORY_PROCESSOR)

    SST_ELI_DOCUMENT_PARAMS(
        { "verbose", "Set the level of output verbosity, 0 is no output, higher "
                     "is more output", "0" },
        { "dbg_mask", "Mask for output. Default is to not mask anything out (0) and defer to 'verbose'.", "0"},
        { "start_verbose_when_issue_address", "Set verbose to 0 until the specified instruction "
                                        "address is issued, then set to 'verbose' parameter", ""},
        { "stop_verbose_when_retire_address", "When the specified instruction "
                                        "address is retired, set verbose to 0", ""},
        { "pause_when_retire_address", "If specified, the simulation will stop when this address is retired.", "0"},
        { "pipeline_trace_file", "If specified, a trace of the pipeline activity will be generated to this file.", ""},
        { "max_cycle", "Maximum number of cycles to execute. The core will halt after this many cycles." , "std::numeric_limits<uint64_t>::max()"},
        { "node_id", "Identifier for the node this core belongs to. Each node in the system needs a unique ID between 0 and (number of nodes) - 1. Used to tag output.", "0"},
        { "core_id", "Identifier for this core. Each core in the system needs a unique ID between 0 and (number of cores) - 1.", 0 },
        { "hardware_threads", "Number of hardware threads in this core", "1" },
        { "clock", "Core clock frequency", "1GHz" },
        { "reorder_slots", "Number of slots in the reorder buffer", "64"}, 
        { "physical_integer_registers", "Number of physical integer registers per hardware thread", "128" },
        { "physical_fp_registers", "Number of physical floating point registers per hardware thread", "128" },
        { "integer_arith_units", "Number of integer arithemetic units", "2" },
        { "integer_arith_cycles", "Cycles per instruction for integer arithmetic", "2" },
        { "integer_div_units", "Number of integer division units", "1" },
        { "integer_div_cycles", "Cycles per instruction for integer division", "4" },
        { "fp_arith_units", "Number of floating point arithmetic units", "2" },
        { "fp_arith_cycles", "Cycles per floating point arithmetic", "8" },
        { "fp_div_units", "Number of floating point division units", "1" },
        { "fp_div_cycles", "Cycles per floating point division", "80" }, 
        { "branch_units", "Number of branch units", "1" }, 
        { "branch_unit_cycles", "Cycles per branch", "int_arith_cycles"},
        { "issues_per_cycle", "Number of instruction issues per cycle", "2" },
        { "fetches_per_cycle", "Number of instruction fetches per cycle", "2" },
        { "retires_per_cycle", "Number of instruction retires per cycle", "2" },
        { "decodes_per_cycle", "Number of instruction decodes per cycle", "2" },
        { "dcache_line_width", "Width of a line for the data cache, in bytes. (Currently not used but may be in the future).", "64"},
        { "icache_line_width", "Width of a line for the instruction cache, in bytes", "64"},
        { "print_retire_tables", "Print registers during retirement step (default is yes)", "true" },
        { "print_issue_tables", "Print registers during issue step (default is yes)", "true" },
        { "print_int_reg", "Print integer registers true/false, auto set to true if verbose > 16", "false" },
        { "print_fp_reg", "Print floating-point registers true/false, auto set to "
                          "true if verbose > 16", "false" },
        { "print_rob", "Print reorder buffer state during issue and retire", "true"},
        { "enable_simt", "Implement SIMT pipeline for multithread kernels", "false"}  )

    SST_ELI_DOCUMENT_STATISTICS(
        { "cycles", "Number of cycles the core executed", "cycles", 1 },
        { "syscall-cycles",
          "Number of cycles spent waiting on execution of SYSCALL in OS "
          "components",
          "cycles", 1 },
        { "rob_slots_in_use", "Number of micro-ops in the ROB each cycle", "instructions", 1 },
        { "rob_cleared_entries", "Number of micro-ops that are cleared during a pipeline clear", "instructions", 1 },
        { "instructions_issued", "Number of instructions issued", "instructions", 1 },
        { "instructions_retired", "Number of instructions retired", "instructions", 1 },
        { "instructions_decoded", "Number of instructions decoded", "instructions", 1 },
        { "branch_mispredicts", "Number of retired branches which were mis-predicted", "instructions", 1 },
        { "branches", "Number of retired branches", "instructions", 1 },
        { "loads_issued", "Number of load instructions issued to the LSQ", "instructions", 1 },
        { "stores_issued", "Number of store instructions issued to the LSQ", "instructions", 1 },
        { "phys_int_reg_in_use", "Number of physical integer registers that are in use each cycle", "registers", 1 },
        { "phys_fp_reg_in_use", "Number of physical floating point registers than are in use each cycle", "registers",
          1 })

    SST_ELI_DOCUMENT_PORTS({ "icache_link", "Connects the CPU to the instruction cache", {} },
                           { "dcache_link", "Connects the CPU to the data cache", {} },
                           { "os_link", "Connects this handler to the main operating system of the node", {}} )

    // Optional since there is nothing to document
    SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS(
        { "lsq", "Load-Store Queue for Memory Access", "SST::Vanadis::VanadisLoadStoreQueue" },
        { "mem_interface_inst", "Interface to memory system for instructions", "SST::Interfaces::StandardMem" },
    )

#ifdef VANADIS_BUILD_DEBUG
    VanadisDebugComponent(SST::ComponentId_t id, SST::Params& params);
    ~VanadisDebugComponent();
#else
    VanadisComponent(SST::ComponentId_t id, SST::Params& params);
    ~VanadisComponent();
#endif

    virtual void init(unsigned int phase);

    void setup();
    void finish();

    void printStatus(SST::Output& output);

    //    void handleIncomingDataCacheEvent( StandardMem::Request* ev );
    void handleIncomingInstCacheEvent(StandardMem::Request* ev);
    void recvOSEvent(SST::Event* ev);

    void handleMisspeculate(const uint32_t hw_thr, const uint64_t new_ip);
    void clearROBMisspeculate(const uint32_t hw_thr);
    
    void clearFuncUnit(const uint32_t hw_thr, std::vector<VanadisFunctionalUnit*>& unit);

    void syscallReturn(uint32_t thr);
    void setHalt(uint32_t thr, int64_t halt_code);
    void startThread(int thr, uint64_t stackStart, uint64_t instructionPointer );
    void startThreadFork( VanadisStartThreadForkReq* req );
    void startThreadClone( VanadisStartThreadCloneReq* req );
    void getThreadState( VanadisGetThreadStateReq* req );
    void dumpRegs( VanadisDumpRegsReq* req );

private:
#ifdef VANADIS_BUILD_DEBUG
    VanadisDebugComponent();                             // for serialization only
    VanadisDebugComponent(const VanadisDebugComponent&); // do not implement
    void operator=(const VanadisDebugComponent&);        // do not implement
#else
    VanadisComponent();                        // for serialization only
    VanadisComponent(const VanadisComponent&); // do not implement
    void operator=(const VanadisComponent&);   // do not implement
#endif

    virtual bool tick(SST::Cycle_t);

    void resetRegisterUseTemps(const uint16_t i_reg, const uint16_t f_reg);

    int assignRegistersToInstruction(
        const uint16_t int_reg_count, const uint16_t fp_reg_count, VanadisInstruction* ins,
        VanadisRegisterStack* int_regs, VanadisRegisterStack* fp_regs, VanadisISATable* isa_table);

    int checkInstructionResources(
        VanadisInstruction* ins, VanadisRegisterStack* int_regs, VanadisRegisterStack* fp_regs,
        VanadisISATable* isa_table);

    int recoverRetiredRegisters(
        VanadisInstruction* ins, VanadisRegisterStack* int_regs, VanadisRegisterStack* fp_regs,
        VanadisISATable* issue_isa_table, VanadisISATable* retire_isa_table);
    
    int recoverRetiredRegisters(
        VanadisInstruction* ins, VanadisRegisterStack* int_regs, VanadisRegisterStack* fp_regs,
        VanadisISATable* issue_isa_table, VanadisISATable* retire_isa_table, uint16_t sw_thr);

    int  performFetch(const uint64_t cycle);
    int  performDecode(const uint64_t cycle);
    int  performIssue(const uint64_t cycle, int hwThr, uint32_t& rob_start, int& unallocated_memory_op_seen);
    int  performExecute(const uint64_t cycle);
    int  performRetire(int rob_num, VanadisCircularQueue<VanadisInstruction*>* rob, const uint64_t cycle);
    int  allocateFunctionalUnit(VanadisInstruction* ins);
    bool mapInstructiontoFunctionalUnit(VanadisInstruction* ins, std::vector<VanadisFunctionalUnit*>& functional_units);
    void printRob(int rob_num, VanadisCircularQueue<VanadisInstruction*>* rob);
    
    bool checkVerboseAddr( uint64_t addr ) {
        for ( auto& it : start_verbose_when_issue_address ) {
            if ( it == addr ) return true;
        }
        return false;
    }

    void setVerboseWhenIssueAddress( std::string addrs ) {
        while ( ! addrs.empty() ) {
            auto pos = addrs.find(',');
            std::string addr;
            if ( pos == std::string::npos ) {
                addr = addrs;
                addrs.clear();
            } else  {
                addr = addrs.substr(0,pos);
                addrs = addrs.substr(pos+1);
            }
            start_verbose_when_issue_address.push_back(  strtol( addr.c_str(), NULL , 16 ) );
        }
    }

    void resetHwThread(uint32_t thr);

    SST::Output* output;

    uint16_t core_id;
    uint64_t current_cycle;
    uint64_t max_cycle;
    uint32_t hw_threads;

    uint32_t fetches_per_cycle;
    uint32_t decodes_per_cycle;
    uint32_t issues_per_cycle;
    uint32_t retires_per_cycle;

    uint32_t m_curRetireHwThread;
    uint32_t m_curIssueHwThread;

    std::vector<VanadisCircularQueue<VanadisInstruction*>*> rob;
    std::vector<VanadisCircularQueue<VanadisInstruction*>*> v_warp_rob;
    std::vector<VanadisDecoder*>                            thread_decoders;
    std::vector<const VanadisDecoderOptions*>               isa_options;

    std::vector<VanadisFunctionalUnit*> fu_int_arith;
    std::vector<VanadisFunctionalUnit*> fu_int_div;
    std::vector<VanadisFunctionalUnit*> fu_branch;
    std::vector<VanadisFunctionalUnit*> fu_fp_arith;
    std::vector<VanadisFunctionalUnit*> fu_fp_div;

    std::vector<VanadisRegisterFile*>  register_files;
    VanadisRegisterStack* int_register_stack;
    VanadisRegisterStack* fp_register_stack;

    std::vector<VanadisISATable*> issue_isa_tables;
    std::vector<VanadisISATable*> retire_isa_tables;

    std::vector<uint8_t*> tmp_not_issued_int_reg_read;
    std::vector<uint8_t*> tmp_int_reg_write;
    std::vector<uint8_t*> tmp_not_issued_fp_reg_read;
    std::vector<uint8_t*> tmp_fp_reg_write;

    std::list<VanadisInsCacheLoadRecord*>* icache_load_records;

    VanadisLoadStoreQueue* lsq;
    StandardMem*           memInstInterface;

    bool* halted_masks;
    bool  print_int_reg;
    bool  print_fp_reg;
    bool  print_issue_tables;
    bool  print_retire_tables;
    bool  print_rob;
    bool enable_simt; //for future use

    char*    instPrintBuffer;
    uint64_t nextInsID;
    uint64_t dCacheLineWidth;
    uint64_t iCacheLineWidth;

    TimeConverter*                     cpuClockTC;
    Clock::Handler<VANADIS_COMPONENT>* cpuClockHandler;

    FILE*           pipelineTrace;

    Statistic<uint64_t>* stat_ins_retired;
    Statistic<uint64_t>* stat_ins_decoded;
    Statistic<uint64_t>* stat_ins_issued;
    Statistic<uint64_t>* stat_loads_issued;
    Statistic<uint64_t>* stat_stores_issued;
    Statistic<uint64_t>* stat_branch_mispredicts;
    Statistic<uint64_t>* stat_branches;
    Statistic<uint64_t>* stat_cycles;
    Statistic<uint64_t>* stat_rob_entries;
    Statistic<uint64_t>* stat_rob_cleared_entries;
    Statistic<uint64_t>* stat_syscall_cycles;
    Statistic<uint64_t>* stat_int_phys_regs_in_use;
    Statistic<uint64_t>* stat_fp_phys_regs_in_use;

    uint32_t ins_issued_this_cycle;
    uint32_t ins_retired_this_cycle;
    uint32_t ins_decoded_this_cycle;

    uint64_t pause_on_retire_address;
    std::deque<uint64_t> start_verbose_when_issue_address;
    uint64_t stop_verbose_when_retire_address;

    std::vector<VanadisFloatingPointFlags*> fp_flags;
    std::vector<VanadisStartThreadCloneReq*> cloneReqs;

    SST::Link* os_link;

    bool* m_checkpointing;
    std::string m_checkpointDir;
    enum { NO_CHECKPOINT, CHECKPOINT_LOAD, CHECKPOINT_SAVE } m_checkpoint;
    void checkpoint(FILE*);
    void checkpointLoad(FILE*);
};

} // namespace Vanadis
} // namespace SST

#endif /* _VANADIS_COMPONENT_H */
