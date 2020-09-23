// Copyright 2009-2020 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2020, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _VANADIS_COMPONENT_H
#define _VANADIS_COMPONENT_H

#include <sst/core/component.h>
#include <sst/core/link.h>
#include <sst/core/output.h>
#include <sst/core/params.h>

#include <array>
#include <set>
#include <limits>

#include "velf/velfinfo.h"

#include "decoder/vdecoder.h"
#include "datastruct/cqueue.h"
#include "inst/vinst.h"
#include "inst/regfile.h"
#include "inst/regstack.h"
#include "inst/isatable.h"
#include "vfuncunit.h"
#include "lsq/vlsq.h"
#include "lsq/vlsqstd.h"

namespace SST {
namespace Vanadis {

class VanadisInsCacheLoadRecord {
public:
	VanadisInsCacheLoadRecord(
		const uint32_t thr,
		const uint64_t addrStart,
		const uint16_t len) :
		hw_thr(thr), addr(addrStart), width(len), hasPayload(false) {

		data = new uint8_t[len];

		for( uint16_t i = 0; i < width; ++i ) {
			data[i] = 0;
		}
	}

	uint64_t getAddress() const  { return addr;       }
	uint32_t getHWThread() const { return hw_thr;     }
	uint16_t getWidth() const    { return width;      }
	bool     hasData() const     { return hasPayload; } 
	uint8_t* getPayload()        { return data;       }

	void     setPayload( uint8_t* ptr ) {
		hasPayload = true;

		for( uint16_t i = 0; i < width; ++i ) {
			data[i] = ptr[i];
		}
	}
private:
	bool  hasPayload;
	uint8_t* data;
	const uint64_t addr;
	const uint16_t width;
	const uint32_t hw_thr;

};

class VanadisComponent : public SST::Component {
public:

    SST_ELI_REGISTER_COMPONENT(
        VanadisComponent,
        "vanadis",
        "VanadisCPU",
        SST_ELI_ELEMENT_VERSION(1,0,0),
	"Vanadis Processor Component",
        COMPONENT_CATEGORY_PROCESSOR
    )

    SST_ELI_DOCUMENT_PARAMS(
	{ "verbose", 		"Set the level of output verbosity, 0 is no output, higher is more output" },
	{ "max_cycles", 	"Maximum number of cycles to execute" },
	{ "reorder_slots", 	"Number of slots in the reorder buffer" },
	{ "core_id", 		"Identifier for this core" },
	{ "hardware_threads", 	"Number of hardware threads in this core" },
    { "physical_integer_registers", "Number of physical integer registers per hardware thread" },
	{ "physical_fp_registers", "Number of physical floating point registers per hardware thread" },
	{ "integer_arith_units", "Number of integer arithemetic units" },
    { "integer_arith_cycles", "Cycles per instruction for integer arithmetic" },
    { "integer_div_units", "Number of integer division units" },
    { "integer_div_cycles", "Cycles per instruction for integer division" },
    { "fp_arith_units",     "Number of floating point arithmetic units" },
    { "fp_arith_cycles",    "Cycles per floating point arithmetic" },
	{ "fp_div_units",       "Number of floating point division units" },
    { "fp_div_cycles",      "Cycles per floating point division" },
    { "load_units",         "Number of memory load units" },
    { "store_units",        "Number of memory store units" },
	{ "max_loads_per_cycle",    "Maximum number of loads that can issue to the cache per cycle" },
	{ "max_stores_per_cycle",   "Maximum number of stores that can issue to the cache per cycle" },
    { "branch_units",       "Number of branch units" },
    { "special_units",      "Number of special instruction units" },
    { "issues_per_cycle",   "Number of instruction issues per cycle" },
    { "fetches_per_cycle",  "Number of instruction fetches per cycle" },
    { "retires_per_cycle",   "Number of instruction retires per cycle" },
    { "decodes_per_cycle",   "Number of instruction decodes per cycle" },
	{ "print_int_reg",      "Print integer registers true/false, auto set to true if verbose > 16" },
	{ "print_fp_reg",		"Print floating-point registers true/false, auto set to true if verbose > 16" }
    )

    SST_ELI_DOCUMENT_STATISTICS(
        { "cycles",  "Number of cycles where there were no events to process, x-bar was quiet", "cycles", 1 },
    )

    SST_ELI_DOCUMENT_PORTS(
    	{ "icache_link", "Connects the CPU to the instruction cache", {} },
    	{ "dcache_link", "Connects the CPU to the data cache", {} }
    )

    // Optional since there is nothing to document
    SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS(
	{ "lsq",                "Load-Store Queue for Memory Access", "SST::Vanadis::VanadisLoadStoreQueue" },
	{ "mem_interface_inst", "Interface to memory system for instructions", "SST::Interfaces::SimpleMem" },
	{ "decoder%(hardware_threads)d", "Instruction decoder for a hardware thread", "SST::Vanadis::VanadisDecoder" }
    )

    VanadisComponent( SST::ComponentId_t id, SST::Params& params );
    ~VanadisComponent();

    virtual void init(unsigned int phase);

    void setup();
    void finish();

    void printStatus( SST::Output& output );

    void handleIncomingDataCacheEvent( SimpleMem::Request* ev );
    void handleIncomingInstCacheEvent( SimpleMem::Request* ev );

    void handleMisspeculate( const uint32_t hw_thr, const uint64_t new_ip );
    void clearROBMisspeculate( const uint32_t hw_thr );
    void resetRegisterStacks( const uint32_t hw_thr );
    void clearFuncUnit( const uint32_t hw_thr, std::vector<VanadisFunctionalUnit*>& unit );

    void syscallReturnCallback( uint32_t thr );

private:
    VanadisComponent();  // for serialization only
    VanadisComponent(const VanadisComponent&); // do not implement
    void operator=(const VanadisComponent&); // do not implement

    virtual bool tick(SST::Cycle_t);

    int assignRegistersToInstruction(
	const uint16_t int_reg_count,
    const uint16_t fp_reg_count,
	VanadisInstruction* ins,
	VanadisRegisterStack* int_regs,
	VanadisRegisterStack* fp_regs,
	VanadisISATable* isa_table);

    int checkInstructionResources(
        VanadisInstruction* ins,
        VanadisRegisterStack* int_regs,
        VanadisRegisterStack* fp_regs,
        VanadisISATable* isa_table,
        std::set<uint16_t>& isa_int_regs_written_ahead,
        std::set<uint16_t>& isa_fp_regs_written_ahead );

    int recoverRetiredRegisters( 
		VanadisInstruction* ins,
        VanadisRegisterStack* int_regs,
        VanadisRegisterStack* fp_regs,
		VanadisISATable* issue_isa_table,
        VanadisISATable* retire_isa_table );

    int performFetch( const uint64_t cycle );
    int performDecode( const uint64_t cycle );   
    int performIssue( const uint64_t cycle );
	int performExecute( const uint64_t cycle );
	int performRetire( VanadisCircularQueue<VanadisInstruction*>* rob,
		const uint64_t cycle );
	int allocateFunctionalUnit( VanadisInstruction* ins );
	bool mapInstructiontoFunctionalUnit( VanadisInstruction* ins, 
		std::vector< VanadisFunctionalUnit* >& functional_units );

    SST::Output* output;

    uint16_t core_id;
    uint64_t current_cycle;
    uint64_t max_cycle;
    uint32_t hw_threads;
    
    uint32_t fetches_per_cycle;
    uint32_t decodes_per_cycle;
    uint32_t issues_per_cycle;
    uint32_t retires_per_cycle;
    
    std::vector< VanadisCircularQueue<VanadisInstruction*>* > rob;
    std::vector< VanadisDecoder* > thread_decoders;
    std::vector< const VanadisDecoderOptions* > isa_options;

    std::vector<VanadisFunctionalUnit*> fu_int_arith;
    std::vector<VanadisFunctionalUnit*> fu_int_div;
    std::vector<VanadisFunctionalUnit*> fu_branch;
    std::vector<VanadisFunctionalUnit*> fu_fp_arith;
    std::vector<VanadisFunctionalUnit*> fu_fp_div;

    std::vector<VanadisRegisterFile*> register_files;
    std::vector<VanadisRegisterStack*> int_register_stacks;
    std::vector<VanadisRegisterStack*> fp_register_stacks;

    std::vector<VanadisISATable*> issue_isa_tables;
    std::vector<VanadisISATable*> retire_isa_tables;

    std::set<uint16_t> tmp_raw_int;
    std::set<uint16_t> tmp_raw_fp;

    std::list<VanadisInsCacheLoadRecord*>* icache_load_records;

    VanadisLoadStoreQueue* lsq;
    SimpleMem* memInstInterface;

    bool* halted_masks;
    bool print_int_reg;
    bool print_fp_reg;

    char* instPrintBuffer;
    uint64_t nextInsID;
    uint64_t dCacheLineWidth;
    uint64_t iCacheLineWidth;

    TimeConverter* cpuClockTC;

    FILE* pipelineTrace;
    VanadisELFInfo* binary_elf_info;
    bool handlingSysCall;
};


} // namespace VanadisComponent
} // namespace SST

#endif /* _VANADIS_COMPONENT_H */
