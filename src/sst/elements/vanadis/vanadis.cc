// Copyright 2009-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2021, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <sst_config.h>
#include <sst/core/output.h>

#include <cstdio>
#include <vector>

#include "vanadis.h"

#include "velf/velfinfo.h"
#include "decoder/vmipsdecoder.h"
#include "inst/vinstall.h"

using namespace SST::Vanadis;

VANADIS_COMPONENT::VANADIS_COMPONENT(SST::ComponentId_t id, SST::Params& params) :
	Component(id),
	current_cycle(0) {

	instPrintBuffer = new char[1024];
	pipelineTrace = nullptr;

	max_cycle = params.find<uint64_t>("max_cycle",
		std::numeric_limits<uint64_t>::max() );

	const int32_t verbosity = params.find<int32_t>("verbose", 0);
	core_id   = params.find<uint32_t>("core_id", 0);

	char* outputPrefix = (char*) malloc( sizeof(char) * 256 );
	sprintf(outputPrefix, "[Core: %3" PRIu32 "]: ", core_id);

	output = new SST::Output(outputPrefix, verbosity, 0, Output::STDOUT);
	free(outputPrefix);

	std::string binary_img = params.find<std::string>("executable", "");

	if( "" == binary_img ) {
		output->verbose(CALL_INFO, 2, 0, "No executable specified, will not perform any binary load.\n");
		binary_elf_info = nullptr;
	} else {
		output->verbose(CALL_INFO, 2, 0, "Executable: %s\n", binary_img.c_str());
		binary_elf_info = readBinaryELFInfo( output, binary_img.c_str() );
		binary_elf_info->print( output );

		if( binary_elf_info->isDynamicExecutable() ) {
                        output->fatal( CALL_INFO, -1, "--> error - executable is dynamically linked. Only static executables are currently supported for simulation.\n");
                } else {
                        output->verbose(CALL_INFO, 2, 0, "--> executable is identified as static linked\n");
                }
	}

	std::string clock_rate = params.find<std::string>("clock", "1GHz");
	output->verbose(CALL_INFO, 2, 0, "Registering clock at %s.\n", clock_rate.c_str());
        cpuClockHandler = new Clock::Handler<VANADIS_COMPONENT>(this, &VANADIS_COMPONENT::tick);
	cpuClockTC = registerClock( clock_rate, cpuClockHandler );

	const uint32_t rob_count = params.find<uint32_t>("reorder_slots", 64);
	dCacheLineWidth = params.find<uint64_t>("dcache_line_width", 64);
        iCacheLineWidth = params.find<uint64_t>("icache_line_width", 64);

	output->verbose(CALL_INFO, 2, 0, "Core L1 Cache Configurations:\n");
	output->verbose(CALL_INFO, 2, 0, "-> D-Cache Line Width:       %" PRIu64 " bytes\n", dCacheLineWidth);
	output->verbose(CALL_INFO, 2, 0, "-> I-Cache Line Width:       %" PRIu64 " bytes\n", iCacheLineWidth);

	hw_threads = params.find<uint32_t>("hardware_threads", 1);
	output->verbose(CALL_INFO, 2, 0, "Creating %" PRIu32 " SMT threads.\n", hw_threads);

        print_int_reg = params.find<bool>("print_int_reg", verbosity > 16 ? 1 : 0);
        print_fp_reg  = params.find<bool>("print_fp_reg",  verbosity > 16 ? 1 : 0);

	const uint16_t int_reg_count = params.find<uint16_t>("physical_integer_registers", 128);
	const uint16_t fp_reg_count  = params.find<uint16_t>("physical_fp_registers", 128);

	output->verbose(CALL_INFO, 2, 0, "Creating physical register files (quantities are per hardware thread)...\n");
	output->verbose(CALL_INFO, 2, 0, "Physical Integer Registers (GPRs): %5" PRIu16 "\n", int_reg_count);
	output->verbose(CALL_INFO, 2, 0, "Physical Floating-Point Registers: %5" PRIu16 "\n", fp_reg_count);

	const uint16_t issue_queue_len = params.find<uint16_t>("issue_queue_length", 4);

	halted_masks = new bool[hw_threads];

	//////////////////////////////////////////////////////////////////////////////////////

	char* decoder_name = new char[64];

	for( uint32_t i = 0; i < hw_threads; ++i ) {

		sprintf(decoder_name, "decoder%" PRIu32 "", i);
		VanadisDecoder* thr_decoder = loadUserSubComponent<SST::Vanadis::VanadisDecoder>(decoder_name);
//		thr_decoder->setHardwareThread( i );

		output->verbose(CALL_INFO, 8, 0, "Loading decoder%" PRIu32 ": %s.\n", i,
			(nullptr == thr_decoder) ? "failed" : "successful");

		if( nullptr == thr_decoder ) {
			output->fatal(CALL_INFO, -1, "Error: was unable to load %s on thread %" PRIu32 "\n",
				decoder_name, i);
		} else {
			output->verbose(CALL_INFO, 8, 0, "-> Decoder configured for %s\n",
				thr_decoder->getISAName());
		}

		thr_decoder->setHardwareThread(i);
		thread_decoders.push_back( thr_decoder );

		output->verbose(CALL_INFO, 8, 0, "Registering SYSCALL return interface...\n");
		std::function<void(uint32_t)> sys_callback = std::bind( &VANADIS_COMPONENT::syscallReturnCallback, this, std::placeholders::_1 );
		thread_decoders[i]->getOSHandler()->registerReturnCallback( sys_callback );

		if( 0 == thread_decoders[i]->getInsCacheLineWidth() ) {
			output->verbose(CALL_INFO, 2, 0, "Auto-setting icache line width in decoder to %" PRIu64 "\n", iCacheLineWidth);
			thread_decoders[i]->setInsCacheLineWidth( iCacheLineWidth );
		} else {
			if( iCacheLineWidth < thread_decoders[i]->getInsCacheLineWidth() ) {
				output->fatal(CALL_INFO, -1, "Decoder for thr %" PRIu32 " has an override icache-line-width of %" PRIu64 ", this exceeds the core icache-line-with of %" PRIu64 " and is likely to result in cache load failures. Set this to less than equal to %" PRIu64 "\n",
					i, thread_decoders[i]->getInsCacheLineWidth(), iCacheLineWidth, iCacheLineWidth);
			} else {
				output->verbose(CALL_INFO, 2, 0, "Decoder for thr %" PRIu32 " is already set to %" PRIu64 ", will not auto-set. The core icache-line-width is currently: %" PRIu64 "\n",
					(uint32_t) i, thread_decoders[i]->getInsCacheLineWidth(),
					iCacheLineWidth);
			}
		}

		isa_options.push_back( thread_decoders[i]->getDecoderOptions() );

		output->verbose(CALL_INFO, 8, 0, "Thread: %6" PRIu32 " ISA set to: %s [Int-Reg: %" PRIu16 "/FP-Reg: %" PRIu16 "]\n",
			i,
			thread_decoders[i]->getISAName(),
			thread_decoders[i]->countISAIntReg(),
			thread_decoders[i]->countISAFPReg());

		register_files.push_back( new VanadisRegisterFile( i, thread_decoders[i]->getDecoderOptions(),
			int_reg_count, fp_reg_count, thr_decoder->getFPRegisterMode() ) );
		int_register_stacks.push_back( new VanadisRegisterStack( int_reg_count ) );
		fp_register_stacks.push_back( new VanadisRegisterStack( fp_reg_count ));

		output->verbose(CALL_INFO, 8, 0, "Reorder buffer set to %" PRIu32 " entries, these are shared by all threads.\n",
			rob_count);
		rob.push_back( new VanadisCircularQueue<VanadisInstruction*>( rob_count ) );
		// WE NEED ISA INTEGER AND FP COUNTS HERE NOT ZEROS
		issue_isa_tables.push_back( new VanadisISATable( thread_decoders[i]->getDecoderOptions(),
			thread_decoders[i]->countISAIntReg(), thread_decoders[i]->countISAFPReg() ) );

		thread_decoders[i]->setThreadROB( rob[i] );

		for( uint16_t j = 0; j < thread_decoders[i]->countISAIntReg(); ++j ) {
			issue_isa_tables[i]->setIntPhysReg( j, int_register_stacks[i]->pop() );
		}

		for( uint16_t j = 0; j < thread_decoders[i]->countISAFPReg(); ++j ) {
			issue_isa_tables[i]->setFPPhysReg( j, fp_register_stacks[i]->pop() );
		}

		retire_isa_tables.push_back( new VanadisISATable( thread_decoders[i]->getDecoderOptions(),
			thread_decoders[i]->countISAIntReg(), thread_decoders[i]->countISAFPReg() ) );
		retire_isa_tables[i]->reset(issue_isa_tables[i]);

		halted_masks[i] = true;
	}

	delete[] decoder_name;

	uint16_t max_int_regs = 0;
	uint16_t max_fp_regs  = 0;

	for( uint32_t i = 0; i < hw_threads; ++i ) {
		max_int_regs = std::max( max_int_regs, thread_decoders[i]->countISAIntReg() );
		max_fp_regs  = std::max( max_fp_regs, thread_decoders[i]->countISAFPReg() );
	}

//	printf("MAX INT: %" PRIu16 ", MAX FP: %" PRIu16 "\n", max_int_regs, max_fp_regs );

	tmp_not_issued_int_reg_read.reserve( max_int_regs );
	tmp_int_reg_write.reserve( max_int_regs );
	tmp_not_issued_fp_reg_read.reserve( max_fp_regs);
	tmp_fp_reg_write.reserve( max_fp_regs );

	resetRegisterUseTemps( max_int_regs, max_fp_regs );

//	memDataInterface = loadUserSubComponent<Interfaces::SimpleMem>("mem_interface_data", ComponentInfo::SHARE_NONE, cpuClockTC,
//		new SimpleMem::Handler<SST::Vanadis::VanadisComponent>(this, &VanadisComponent::handleIncomingDataCacheEvent ));
	memInstInterface = loadUserSubComponent<Interfaces::SimpleMem>("mem_interface_inst", ComponentInfo::SHARE_NONE, cpuClockTC,
		new SimpleMem::Handler<SST::Vanadis::VANADIS_COMPONENT>(this, &VANADIS_COMPONENT::handleIncomingInstCacheEvent));

    	// Load anonymously if not found in config
/*
    	if (!memInterface) {
        	std::string memIFace = params.find<std::string>("meminterface", "memHierarchy.memInterface");
        	output.verbose(CALL_INFO, 1, 0, "Loading memory interface: %s ...\n", memIFace.c_str());
        	Params interfaceParams = params.get_scoped_params("meminterface");
        	interfaceParams.insert("port", "dcache_link");

        	memInterface = loadAnonymousSubComponent<Interfaces::SimpleMem>(memIFace, "memory", 0, ComponentInfo::SHARE_PORTS | ComponentInfo::INSERT_STATS,
		interfaceParams, cpuClockTC, new SimpleMem::Handler<JunoCPU>(this, &JunoCPU::handleEvent));

        	if( NULL == mem )
            		output.fatal(CALL_INFO, -1, "Error: unable to load %s memory interface.\n", memIFace.c_str());
    	}
*/

//	if( nullptr == memDataInterface ) {
//		output->fatal(CALL_INFO, -1, "Error: unable to load memory interface subcomponent for data cache.\n");
//	}

	if( nullptr == memInstInterface ) {
		output->fatal(CALL_INFO, -1, "Error: unable ot load memory interface subcomponent for instruction cache.\n");
	}

    	output->verbose(CALL_INFO, 1, 0, "Successfully loaded memory interface.\n");

	for( uint32_t i = 0; i < thread_decoders.size(); ++i ) {
		output->verbose(CALL_INFO, 8, 0, "Configuring thread instruction cache interface (thread %" PRIu32 ")\n", i);
		thread_decoders[i]->getInstructionLoader()->setMemoryInterface( memInstInterface );
	}

	lsq = loadUserSubComponent<SST::Vanadis::VanadisLoadStoreQueue>("lsq");

	if( nullptr == lsq ) {
		output->fatal(CALL_INFO, -1, "Error - unable to load the load-store queue (lsq subcomponent)\n");
	}

	lsq->setRegisterFiles( &register_files );

	if( 0 == core_id ) {
		halted_masks[0] = false;
		uint64_t initial_config_ip = thread_decoders[0]->getInstructionPointer();

		// This wasn't provided, or its explicitly set to zero which means
		// we should auto-calculate it
		const uint64_t c0_entry = (binary_elf_info != nullptr) ? binary_elf_info->getEntryPoint() : 0;
		output->verbose(CALL_INFO, 8, 0, "Configuring core-0, thread-0 entry point = %p\n",
			(void*) c0_entry);
		thread_decoders[0]->setInstructionPointer( c0_entry );

		uint64_t stack_start = 0;

		Params app_params = params.get_scoped_params("app");

		output->verbose(CALL_INFO, 8, 0, "Configuring core-0, thread-0 application info...\n");
		thread_decoders[0]->configureApplicationLaunch( output, issue_isa_tables[0], register_files[0],
			lsq, binary_elf_info, app_params );

		// Force retire table to sync with issue table
		retire_isa_tables[0]->reset( issue_isa_tables[0] );

		if( initial_config_ip > 0 ) {
			output->verbose(CALL_INFO, 8, 0, "Overrding entry point for core-0, thread-0, set to 0x%llx\n",
				initial_config_ip);
			thread_decoders[0]->setInstructionPointer( initial_config_ip );
		} else {
			output->verbose(CALL_INFO, 8, 0, "Utilizing entry point from binary (auto-detected) 0x%llx\n",
				thread_decoders[0]->getInstructionPointer() );
		}
	}

//	VanadisInstruction* test_ins = new VanadisAddInstruction(nextInsID++, 0, 0, isa_options[0], 3, 4, 5);
//	thread_decoders[0]->getDecodedQueue()->push( test_ins );
//	rob[0]->push(test_ins);

//	test_ins = new VanadisAddImmInstruction(nextInsID++, 1, 0, isa_options[0], 1, 3, 128);
//	thread_decoders[0]->getDecodedQueue()->push( test_ins );
//	rob[0]->push(test_ins);

//	test_ins = new VanadisAddImmInstruction(nextInsID++, 2, 0, isa_options[0], 3, 10101010, 4);
//	thread_decoders[0]->getDecodedQueue()->push( test_ins );
//	rob[0]->push(test_ins);

//	test_ins = new VanadisAddInstruction(nextInsID++, 4, 0, isa_options[0], 9, 4, 3);
//	thread_decoders[0]->getDecodedQueue()->push( test_ins );
//	rob[0]->push(test_ins);

//	test_ins = new VanadisSubInstruction(nextInsID++, 3, 0, isa_options[0], 4, 1, 1);
//	thread_decoders[0]->getDecodedQueue()->push( test_ins );
//	rob[0]->push(test_ins);

//	test_ins = new VanadisSubInstruction(nextInsID++, 3, 0, isa_options[0], 5, 6, 1);
//	thread_decoders[0]->getDecodedQueue()->push( test_ins );
//	rob[0]->push(test_ins);

//	test_ins = new VanadisAddImmInstruction(nextInsID++, 3, 0, isa_options[0], 10, 0, 256 );
//	thread_decoders[0]->getDecodedQueue()->push( test_ins );
//        rob[0]->push(test_ins);

//	test_ins = new VanadisStoreInstruction( nextInsID++, 3, 0, isa_options[0], 10, 512, 5, 8 );
//	thread_decoders[0]->getDecodedQueue()->push( test_ins );
//        rob[0]->push(test_ins);

//	test_ins = new VanadisLoadInstruction( nextInsID++, 4, 0, isa_options[0], 0, 768, 12, 8 );
//	thread_decoders[0]->getDecodedQueue()->push( test_ins );
//        rob[0]->push(test_ins);

	//////////////////////////////////////////////////////////////////////////////////////

	uint16_t fu_id = 0;

	const uint16_t int_arith_units = params.find<uint16_t>("integer_arith_units", 2);
	const uint16_t int_arith_cycles = params.find<uint16_t>("integer_arith_cycles", 2);

	output->verbose(CALL_INFO, 2, 0, "Creating %" PRIu16 " integer arithmetic units, latency = %" PRIu16 "...\n",
		int_arith_units, int_arith_cycles);

	for( uint16_t i = 0; i < int_arith_units; ++i ) {
		fu_int_arith.push_back( new VanadisFunctionalUnit(fu_id++, INST_INT_ARITH, int_arith_cycles) );
	}

	const uint16_t int_div_units = params.find<uint16_t>("integer_div_units", 1);
	const uint16_t int_div_cycles = params.find<uint16_t>("integer_div_cycles", 4);

	output->verbose(CALL_INFO, 2, 0, "Creating %" PRIu16 " integer division units, latency = %" PRIu16 "...\n",
		int_div_units, int_div_cycles);

	for( uint16_t i = 0; i < int_div_units; ++i ) {
		fu_int_div.push_back( new VanadisFunctionalUnit(fu_id++, INST_INT_DIV, int_div_cycles) );
	}

	const uint16_t branch_units  = params.find<uint16_t>("branch_units", 1);
	const uint16_t branch_cycles = params.find<uint16_t>("branch_unit_cycles", int_arith_cycles);

	output->verbose(CALL_INFO, 2, 0, "Creating %" PRIu16 " branching units, latency = %" PRIu16 "...\n",
		branch_units, branch_cycles);

	for( uint16_t i = 0; i < branch_units; ++i ) {
		fu_branch.push_back( new VanadisFunctionalUnit(fu_id++, INST_BRANCH, branch_cycles) );
	}

	//////////////////////////////////////////////////////////////////////////////////////

	const uint16_t fp_arith_units = params.find<uint16_t>("fp_arith_units", 2);
	const uint16_t fp_arith_cycles = params.find<uint16_t>("fp_arith_cycles", 8);

	output->verbose(CALL_INFO, 2, 0, "Creating %" PRIu16 " floating point arithmetic units, latency = %" PRIu16 "...\n",
		fp_arith_units, fp_arith_cycles);

	for( uint16_t i = 0; i < fp_arith_units; ++i ) {
		fu_fp_arith.push_back( new VanadisFunctionalUnit(fu_id++, INST_FP_ARITH, fp_arith_cycles) );
	}

	const uint16_t fp_div_units = params.find<uint16_t>("fp_div_units", 1);
	const uint16_t fp_div_cycles = params.find<uint16_t>("fp_div_cycles", 80);

	output->verbose(CALL_INFO, 2, 0, "Creating %" PRIu16 " floating point division units, latency = %" PRIu16 "...\n",
		fp_div_units, fp_div_cycles);

	for( uint16_t i = 0; i < fp_div_units; ++i ) {
		fu_fp_div.push_back( new VanadisFunctionalUnit(fu_id++, INST_FP_DIV, fp_div_cycles) );
	}

	//////////////////////////////////////////////////////////////////////////////////////

	std::function<void(uint32_t, int64_t)> haltThreadCB = std::bind(
		&VANADIS_COMPONENT::setHalt, this, std::placeholders::_1, std::placeholders::_2 );

	for( uint32_t i = 0; i < hw_threads; ++i ) {
		thread_decoders[i]->getOSHandler()->setCoreID( core_id );
		thread_decoders[i]->getOSHandler()->setHWThread(i);
		thread_decoders[i]->getOSHandler()->setRegisterFile( register_files[i] );
		thread_decoders[i]->getOSHandler()->setISATable( retire_isa_tables[i]  );
		thread_decoders[i]->getOSHandler()->setHaltThreadCallback( haltThreadCB );
	}

	//////////////////////////////////////////////////////////////////////////////////////

//	const uint64_t max_addr_mask = binary_elf_info->isELF32() ? 0xFFFFFFFF : UINT64_MAX;
//	output->verbose(CALL_INFO, 8, 0, "Setting maximum address mask for the LSQ to: 0x%0llx (derived from ELF-32? %s)\n",
//		max_addr_mask, binary_elf_info->isELF32() ? "yes" : "no");
//
//	// if we are doing a 32-bit run, then limit the address ranges which can be reached
//	// otherwise leave the mask fully open.
//	lsq->setMaxAddressMask( max_addr_mask );

	handlingSysCall = false;
	fetches_per_cycle = params.find<uint32_t>("fetches_per_cycle", 2);
    decodes_per_cycle = params.find<uint32_t>("decodes_per_cycle", 2);
    issues_per_cycle  = params.find<uint32_t>("issues_per_cycle",  2);
    retires_per_cycle = params.find<uint32_t>("retires_per_cycle", 2);

	output->verbose(CALL_INFO, 8, 0, "Configuring hardware parameters:\n");
	output->verbose(CALL_INFO, 8, 0, "-> Fetches/cycle:                %" PRIu32 "\n", fetches_per_cycle);
	output->verbose(CALL_INFO, 8, 0, "-> Decodes/cycle:                %" PRIu32 "\n", decodes_per_cycle);
	output->verbose(CALL_INFO, 8, 0, "-> Retires/cycle:                %" PRIu32 "\n", retires_per_cycle);
//        output->verbose(CALL_INFO, 8, 0, "-> LSQ Store Entries:            %" PRIu32 "\n", (uint32_t) lsq_store_size );
//        output->verbose(CALL_INFO, 8, 0, "-> LSQ Stores In-flight:         %" PRIu32 "\n", (uint32_t) lsq_store_pending );
//        output->verbose(CALL_INFO, 8, 0, "-> LSQ Load Entries:             %" PRIu32 "\n", (uint32_t) lsq_load_size );
//        output->verbose(CALL_INFO, 8, 0, "-> LSQ Loads In-flight:          %" PRIu32 "\n", (uint32_t) lsq_load_pending );

    	std::string pipeline_trace_path = params.find<std::string>("pipeline_trace_file", "");

    	if( pipeline_trace_path == "" ) {
    		output->verbose(CALL_INFO, 8, 0, "Pipeline trace output not specified, disabling.\n");
    	} else {
    		output->verbose(CALL_INFO, 8, 0, "Opening a pipeline trace output at: %s\n",
    			pipeline_trace_path.c_str());
    		pipelineTrace = fopen( pipeline_trace_path.c_str(), "wt" );

    		if( pipelineTrace == nullptr ) {
    			output->fatal(CALL_INFO, -1, "Failed to open pipeline trace file.\n");
    		}
    	}

        pause_on_retire_address = params.find<uint64_t>("pause_when_retire_address", 0);

	// Register statistics ///////////////////////////////////////////////////////
	stat_ins_retired   = registerStatistic<uint64_t>( "instructions_retired", "1" );
	stat_ins_decoded   = registerStatistic<uint64_t>( "instructions_decoded", "1" );
	stat_ins_issued    = registerStatistic<uint64_t>( "instructions_issued",  "1" );
	stat_loads_issued  = registerStatistic<uint64_t>( "loads_issued",  "1" );
	stat_stores_issued = registerStatistic<uint64_t>( "stores_issued", "1" );
	stat_branch_mispredicts = registerStatistic<uint64_t>( "branch_mispredicts", "1" );
	stat_branches      = registerStatistic<uint64_t>( "branches", "1" );
	stat_cycles        = registerStatistic<uint64_t>( "cycles", "1" );
	stat_rob_entries   = registerStatistic<uint64_t>( "rob_slots_in_use", "1" );
	stat_rob_cleared_entries = registerStatistic<uint64_t>( "rob_cleared_entries", "1" );
	stat_syscall_cycles = registerStatistic<uint64_t>( "syscall-cycles", "1" );
	stat_int_phys_regs_in_use = registerStatistic<uint64_t>( "phys_int_reg_in_use", "1" );
	stat_fp_phys_regs_in_use = registerStatistic<uint64_t>( "phys_fp_reg_in_use", "1" );

	registerAsPrimaryComponent();
    	primaryComponentDoNotEndSim();
}

VANADIS_COMPONENT::~VANADIS_COMPONENT() {
	delete[] instPrintBuffer;
	delete lsq;

	if( pipelineTrace != nullptr ) {
		fclose( pipelineTrace );
	}
}

void VANADIS_COMPONENT::setHalt( uint32_t thr, int64_t halt_code ) {
	output->verbose(CALL_INFO, 2, 0, "-> Receive halt request on thread %" PRIu32 " / code: %" PRId64 "\n",
		thr, halt_code);

	if( thr >= hw_threads ) {
		// Incorrect thread, ignore? error?
	} else {
		switch( halt_code ) {
		default:
			{
				halted_masks[thr] = true;

				// Reset address to zero
				handleMisspeculate( thr, 0 );

				bool all_halted = true;

				for( uint32_t i = 0; i < hw_threads; ++i ) {
					all_halted = all_halted & halted_masks[i];
				}

				if( all_halted ) {
					output->verbose(CALL_INFO, 2, 0, "-> all threads on core are halted, tell core we can exit further simulation unless we recv a wake up.\n");
					primaryComponentOKToEndSim();
				}
			}
			break;
		}
	}
}

int VANADIS_COMPONENT::performFetch( const uint64_t cycle ) {
	// This is handled by the decoder step, so just keep it empty.
	return 0;
}

int VANADIS_COMPONENT::performDecode( const uint64_t cycle ) {

	for( uint32_t i = 0 ; i < hw_threads; ++i ) {
		const int64_t rob_before_decode = (int64_t) rob[i]->size();

		// If thread is not masked then decode from it
		if( ! halted_masks[i] ) {
			thread_decoders[i]->tick(output, (uint64_t) cycle);
		}

		const int64_t rob_after_decode = (int64_t) rob[i]->size();
		const int64_t decoded_cycle = (rob_after_decode - rob_before_decode);
		ins_decoded_this_cycle += (decoded_cycle > 0) ?
			static_cast<uint64_t>( decoded_cycle ) : 0;
	}

	return 0;
}

void VANADIS_COMPONENT::resetRegisterUseTemps( const uint16_t int_reg_count,
	const uint16_t fp_reg_count ) {

	for( uint16_t i = 0; i < int_reg_count; ++i ) {
		tmp_not_issued_int_reg_read[i] = false;
		tmp_int_reg_write[i]           = false;
	}

	for( uint16_t i = 0; i < fp_reg_count; ++i ) {
		tmp_not_issued_fp_reg_read[i]   = false;
		tmp_fp_reg_write[i]             = false;
	}
}

int VANADIS_COMPONENT::performIssue( const uint64_t cycle ) {
	const int output_verbosity = output->getVerboseLevel();
	bool issued_an_ins = false;;

	for( uint32_t i = 0 ; i < hw_threads; ++i ) {
		if( ! halted_masks[i] ) {
#ifdef VANADIS_BUILD_DEBUG
			if( output->getVerboseLevel() >= 4 ) {
				issue_isa_tables[i]->print(output, register_files[i], print_int_reg, print_fp_reg);
			}
#endif

			bool found_store = false;
			bool found_load  = false;
			issued_an_ins = false;

			// Set all register uses to false for this thread
			resetRegisterUseTemps( thread_decoders[i]->countISAIntReg(), thread_decoders[i]->countISAFPReg() );

			// Find the next instruction which has not been issued yet
			for( uint32_t j = 0; j < rob[i]->size(); ++j ) {
				VanadisInstruction* ins = rob[i]->peekAt(j);

				if( ! ins->completedIssue() ) {
#ifdef VANADIS_BUILD_DEBUG
					if( output_verbosity >= 8 ) {
						ins->printToBuffer(instPrintBuffer, 1024);
						output->verbose(CALL_INFO, 8, 0, "--> Attempting issue for: rob[%" PRIu32 "]: 0x%llx / %s\n",
							j, ins->getInstructionAddress(), instPrintBuffer );
					}
#endif
					const int resource_check = checkInstructionResources( ins, int_register_stacks[i],
						fp_register_stacks[i], issue_isa_tables[i]);

#ifdef VANADIS_BUILD_DEBUG
					if( output_verbosity >= 8 ) {
						output->verbose(CALL_INFO, 8, 0, "----> Check if registers are usable? result: %d (%s)\n",
							resource_check, (0 == resource_check) ? "success" : "cannot issue");
					}
#endif

					if( 0 == resource_check ) {
						if( ((INST_STORE == ins->getInstFuncType()) || (INST_LOAD == ins->getInstFuncType()))
							 && (found_load || found_store) ) {
								// We cannot issue
						} else {
//							if( (INST_LOAD == ins->getInstFuncType()) && (found_load || found_store) ) {
//									// We cannot issue
//							} else {
								const int allocate_fu = allocateFunctionalUnit( ins );


#ifdef VANADIS_BUILD_DEBUG
								if( output_verbosity >= 8 ) {
									output->verbose(CALL_INFO, 8, 0, "----> allocated functional unit: %s\n",
										(0 == allocate_fu) ? "yes" : "no");
								}
#endif

								if( 0 == allocate_fu ) {
									const int status = assignRegistersToInstruction(
										thread_decoders[i]->countISAIntReg(),
										thread_decoders[i]->countISAFPReg(),
										ins,
										int_register_stacks[i],
										fp_register_stacks[i],
										issue_isa_tables[i]);
#ifdef VANADIS_BUILD_DEBUG
									if( output_verbosity  >= 8 ) {
										ins->printToBuffer(instPrintBuffer, 1024);
										output->verbose(CALL_INFO, 8, 0, "----> Issued for: %s / 0x%llx / status: %d\n", instPrintBuffer,
											ins->getInstructionAddress(), status);
									}
#endif
									ins->markIssued();
									ins_issued_this_cycle++;
//									stat_ins_issued->addData(1);
									issued_an_ins = true;
								}
//							}
						}
					}

					// if the instruction is *not* issued yet, we need to keep track
					// of which instructions are being read
					for( uint16_t k = 0; k < ins->countISAIntRegIn(); ++k ) {
						tmp_not_issued_int_reg_read[ ins->getISAIntRegIn(k) ] = true;
					}

					for( uint16_t k = 0; k < ins->countISAFPRegIn(); ++k ) {
						tmp_not_issued_fp_reg_read[ ins->getISAFPRegIn(k) ] = true;
					}
				}

				// Collect up all integer registers we write to
				for( uint16_t k = 0; k < ins->countISAIntRegOut(); ++k ) {
					tmp_int_reg_write[ ins->getISAIntRegOut(k) ] = true;
				}

				// Collect up all fp registers we write to
				for( uint16_t k = 0; k < ins->countISAFPRegOut(); ++k ) {
					tmp_fp_reg_write[ ins->getISAFPRegOut(k) ] = true;
				}

				// Keep track of whether we have seen a load or a store ahead of us
				// that hasn't been issued, because that means the LSQ hasn't seen it
				// yet and so we could get an ordering violation in the memory system
				found_store |= (INST_STORE == ins->getInstFuncType()) && (! ins->completedIssue());
				found_load  |= (INST_LOAD  == ins->getInstFuncType()) && (! ins->completedIssue());

				// Keep track of whether we have seen any fences, we just ensure we
				// cannot issue load/stores until fences complete
				if( INST_FENCE == ins->getInstFuncType() ) {
					found_store = true;
					found_load  = true;
				}

				// We issued an instruction this cycle, so exit
				if( issued_an_ins ) {
					break;
				}
			}

			// Only print the table if we issued an instruction, reduce print out
			// clutter
			if( issued_an_ins ) {
				if( output_verbosity >= 8 ) {
					issue_isa_tables[i]->print(output, register_files[i], print_int_reg, print_fp_reg);
				}
			}
		} else {
			output->verbose(CALL_INFO, 8, 0, "thread %" PRIu32 " is halted, did not process for issue this cycle.\n", i);
		}
	}

	// if we issued an instruction tell the caller we want to be called again (return 0)
	if( issued_an_ins ) {
		return 0;
	} else {
		// we didn't issue this time, don't call us again this cycle because we will just repeat the work
		// and conclude we cannot issue again as well
		return 1;
	}
}

int VANADIS_COMPONENT::performExecute( const uint64_t cycle ) {
	for( VanadisFunctionalUnit* next_fu : fu_int_arith ) {
		next_fu->tick(cycle, output, register_files);
	}

	for( VanadisFunctionalUnit* next_fu : fu_int_div ) {
		next_fu->tick(cycle, output, register_files);
	}

	for( VanadisFunctionalUnit* next_fu : fu_fp_arith ) {
		next_fu->tick(cycle, output, register_files);
	}

	for( VanadisFunctionalUnit* next_fu : fu_fp_div ) {
		next_fu->tick(cycle, output, register_files);
	}

	for( VanadisFunctionalUnit* next_fu : fu_branch ) {
		next_fu->tick(cycle, output, register_files);
	}

	// Tick the load/store queue
	lsq->tick( (uint64_t) cycle );

	return 0;
}

int VANADIS_COMPONENT::performRetire( VanadisCircularQueue<VanadisInstruction*>* rob, const uint64_t cycle ) {

#ifdef VANADIS_BUILD_DEBUG
	if( output->getVerboseLevel() >= 8 ) {
		output->verbose(CALL_INFO, 8, 0, "-- ROB: %" PRIu32 " out of %" PRIu32 " entries:\n",
			(uint32_t) rob->size(), (uint32_t) rob->capacity() );

		for( int j = (int) rob->size() - 1; j >= 0; --j ) {
			output->verbose(CALL_INFO, 8, 0, "----> ROB[%2d]: ins: 0x%016llx / %10s / error: %3s / issued: %3s / spec: %3s / rob-front: %3s / exe: %3s\n",
				j, rob->peekAt(j)->getInstructionAddress(), rob->peekAt(j)->getInstCode(),
				rob->peekAt(j)->trapsError() ? "yes" : "no",
				rob->peekAt(j)->completedIssue() ? "yes" : "no",
				rob->peekAt(j)->isSpeculated() ? "yes" : "no",
				rob->peekAt(j)->checkFrontOfROB() ? "yes" : "no",
				rob->peekAt(j)->completedExecution() ? "yes" : "no" );
		}
	}
#endif

	// if empty, nothing to do here, return 1 to prevent being called again as nothing we can
	// do this cycle. This is likely the result of a branch mis-predict
	if( rob->empty() ) {
		return 1;
	}

	VanadisInstruction* rob_front = rob->peek();
	bool perform_pipeline_clear = false;

	// Instruction is flagging error, print out and halt
	if( rob_front->trapsError() ) {
		output->verbose(CALL_INFO, 0, 0, "Error has been detected in retired instruction. Retired register status:\n");

		retire_isa_tables[rob_front->getHWThread()]->print(output,
                                        register_files[rob_front->getHWThread()], print_int_reg, print_fp_reg, 0);

		output->fatal( CALL_INFO, -1, "Instruction 0x%llx flags an error (instruction-type=%s) at cycle %" PRIu64 "\n",
			rob_front->getInstructionAddress(), rob_front->getInstCode(), cycle );
	}

	if( rob_front->completedIssue() && rob_front->completedExecution() ) {
		bool perform_cleanup         = true;
		bool perform_delay_cleanup   = false;
		uint64_t pipeline_reset_addr = 0;

		if( rob_front->isSpeculated() ) {
#ifdef VANADIS_BUILD_DEBUG
			output->verbose(CALL_INFO, 8, 0, "--> instruction is speculated\n");
#endif
			VanadisSpeculatedInstruction* spec_ins = dynamic_cast<VanadisSpeculatedInstruction*>( rob_front );

			if( nullptr == spec_ins ) {
				output->fatal(CALL_INFO, -1, "Error - instruction is speculated, but not able to perform a cast to a speculated instruction.\n");
			}

			stat_branches->addData(1);

			switch( spec_ins->getDelaySlotType() ) {
			case VANADIS_SINGLE_DELAY_SLOT:
			case VANADIS_CONDITIONAL_SINGLE_DELAY_SLOT:
				{
					// is there an instruction behind us in the ROB queue?
					if( rob->size() >= 2 ) {
						VanadisInstruction* delay_ins = rob->peekAt(1);

						if( delay_ins->completedExecution() ) {
							if( delay_ins->trapsError() ) {
								output->fatal(CALL_INFO, -1, "Instruction (delay-slot) 0x%llx flags an error (instruction-type: %s)\n",
									delay_ins->getInstructionAddress(), delay_ins->getInstCode() );
							}

							perform_delay_cleanup = true;
						} else {
#ifdef VANADIS_BUILD_DEBUG
							output->verbose(CALL_INFO, 8, 0, "----> delay slot has not completed execution, stall to wait.\n");
#endif
							if( ! delay_ins->checkFrontOfROB() ) {
								delay_ins->markFrontOfROB();
							}

							perform_cleanup = false;
						}
					} else {
						// The instruction is not in the ROB yet, so we must wait
						perform_cleanup = false;
						perform_delay_cleanup = false;
					}
				}
				break;
			case VANADIS_NO_DELAY_SLOT:
				break;
			}

			// If we are performing a clean up (means we executed and the delays are
			// processed OK, then we are good to calculate branch-to locations.
			if( perform_cleanup ) {
				pipeline_reset_addr   = spec_ins->getTakenAddress();
				perform_pipeline_clear = (pipeline_reset_addr != spec_ins->getSpeculatedAddress());

#ifdef VANADIS_BUILD_DEBUG
				output->verbose(CALL_INFO, 8, 0, "----> speculated addr: 0x%llx / result addr: 0x%llx / pipeline-clear: %s\n",
					spec_ins->getSpeculatedAddress(), pipeline_reset_addr,
					perform_pipeline_clear ? "yes" : "no" );

				output->verbose(CALL_INFO, 8, 0, "----> Updating branch predictor with new information (new addr: 0x%llx)\n",
					pipeline_reset_addr);
#endif
				thread_decoders[rob_front->getHWThread()]->getBranchPredictor()->push(
					spec_ins->getInstructionAddress(), pipeline_reset_addr );


				if( (pause_on_retire_address > 0) &&
					(rob_front->getInstructionAddress() == pause_on_retire_address) ) {

					// print the register and pipeline status
					printStatus((*output));

					output->verbose(CALL_INFO, 0, 0, "ins: 0x%llx / speculated-address: 0x%llx / taken: 0x%llx / reset: 0x%llx / clear-check: %3s / pipe-clear: %3s / delay-cleanup: %3s\n",
						spec_ins->getInstructionAddress(),
						spec_ins->getSpeculatedAddress(),
						spec_ins->getTakenAddress(),
						pipeline_reset_addr,
						spec_ins->getSpeculatedAddress() != pipeline_reset_addr ? "yes" : "no",
						perform_pipeline_clear ? "yes" : "no",
						perform_delay_cleanup  ? "yes" : "no");

					// stop simulation
					output->fatal(CALL_INFO, -2, "Retired instruction address 0x%llx, requested terminate on retire this address.\n",
						pause_on_retire_address);
				}

			}
		}

		// is the instruction completed (including anything like delay slots) and can
		// be cleared from the ROB
		if( perform_cleanup ) {
			rob->pop();

#ifdef VANADIS_BUILD_DEBUG
			output->verbose(CALL_INFO, 8, 0, "----> Retire: 0x%0llx / %s\n",
				rob_front->getInstructionAddress(), rob_front->getInstCode() );
#endif
			if( pipelineTrace != nullptr ) {
				fprintf( pipelineTrace, "0x%08llx %s\n",
					rob_front->getInstructionAddress(), rob_front->getInstCode() );
			}
				recoverRetiredRegisters( rob_front, int_register_stacks[rob_front->getHWThread()],
					fp_register_stacks[rob_front->getHWThread()],
					issue_isa_tables[rob_front->getHWThread()],
					retire_isa_tables[rob_front->getHWThread()] );

				ins_retired_this_cycle++;

				if( perform_delay_cleanup ) {

					VanadisInstruction* delay_ins = rob->pop();
#ifdef VANADIS_BUILD_DEBUG
					output->verbose(CALL_INFO, 8, 0, "----> Retire delay: 0x%llx / %s\n",
						delay_ins->getInstructionAddress(), delay_ins->getInstCode() );
#endif
					if( pipelineTrace != nullptr ) {
						fprintf( pipelineTrace, "0x%08llx %s\n",
							delay_ins->getInstructionAddress(), delay_ins->getInstCode() );
					}

					recoverRetiredRegisters( delay_ins, int_register_stacks[delay_ins->getHWThread()],
						fp_register_stacks[delay_ins->getHWThread()],
						issue_isa_tables[delay_ins->getHWThread()],
						retire_isa_tables[delay_ins->getHWThread()] );
						
//					if( delay_ins->endsMicroOpGroup() ) {
//						stat_ins_retired->addData(1);
					ins_retired_this_cycle++;
//					}

					delete delay_ins;
				}

				retire_isa_tables[rob_front->getHWThread()]->print(output,
					register_files[rob_front->getHWThread()], print_int_reg, print_fp_reg);

			if( (pause_on_retire_address > 0) &&
				(rob_front->getInstructionAddress() == pause_on_retire_address) ) {

				// print the register and pipeline status
				printStatus((*output));

				output->verbose(CALL_INFO, 0, 0, "pipe-clear: %3s / delay-cleanup: %3s\n",
					perform_pipeline_clear ? "yes" : "no",
					perform_delay_cleanup  ? "yes" : "no");

				// stop simulation
				output->fatal(CALL_INFO, -2, "Retired instruction address 0x%llx, requested terminate on retire this address.\n",
					pause_on_retire_address);
			}

				if( perform_pipeline_clear ) {
#ifdef VANADIS_BUILD_DEBUG
					output->verbose(CALL_INFO, 8, 0, "----> perform a pipeline clear thread %" PRIu32 ", reset to address: 0x%llx\n",
						rob_front->getHWThread(), pipeline_reset_addr);
#endif
					handleMisspeculate( rob_front->getHWThread(), pipeline_reset_addr );

					stat_branch_mispredicts->addData(1);
				}


			delete rob_front;
		}
	} else {
		if( INST_SYSCALL == rob_front->getInstFuncType() ) {
			if( rob_front->completedIssue() ) {
				// have we been marked front on ROB yet? if yes, then we have issued our syscall
				if( ! rob_front->checkFrontOfROB() ) {
					VanadisSysCallInstruction* the_syscall_ins = dynamic_cast<VanadisSysCallInstruction*>( rob_front );

					if( nullptr == the_syscall_ins ) {
						output->fatal(CALL_INFO, -1, "Error: SYSCALL cannot be converted to an actual sys-call instruction.\n");
					}

#ifdef VANADIS_BUILD_DEBUG
					output->verbose(CALL_INFO, 8, 0, "[syscall] -> calling OS handler in decode engine (ins-addr: 0x%0llx)...\n",
						the_syscall_ins->getInstructionAddress());
#endif
					thread_decoders[ rob_front->getHWThread() ]->getOSHandler()->handleSysCall( the_syscall_ins );

					// mark as front of ROB now we can proceed
					rob_front->markFrontOfROB();
				}

				// We spent this cycle waiting on an issued SYSCALL, it has not resolved at the emulated OS component
				// yet so we have to wait, potentiallty for a lot longer
				stat_syscall_cycles->addData(1);

				return INT_MAX;
			}
		} else {
			if( ! rob_front->checkFrontOfROB() ) {
				rob_front->markFrontOfROB();
			} else {
				// Return 2 because instruction is front of ROB but has not executed and so we cannot make progress
				// any more this cycle
				return 2;
			}
		}
	}

	return 0;
}

bool VANADIS_COMPONENT::mapInstructiontoFunctionalUnit( VanadisInstruction* ins,
	std::vector< VanadisFunctionalUnit* >& functional_units ) {

	bool allocated = false;

	for( VanadisFunctionalUnit* next_fu : functional_units ) {
		if( next_fu->isInstructionSlotFree() ) {
			next_fu->setSlotInstruction( ins );
			allocated = true;
			break;
		}
	}

	return allocated;
}

int VANADIS_COMPONENT::allocateFunctionalUnit( VanadisInstruction* ins ) {
	bool allocated_fu = false;

	switch( ins->getInstFuncType() ) {
		case INST_INT_ARITH:
			allocated_fu = mapInstructiontoFunctionalUnit( ins, fu_int_arith );
			break;

		case INST_FP_ARITH:
			allocated_fu = mapInstructiontoFunctionalUnit( ins, fu_fp_arith );
			break;
			
		case INST_LOAD:
			if( ! lsq->loadFull() ) {
//				if( ins->endsMicroOpGroup() ) {
					stat_loads_issued->addData(1);
//				}
			
				lsq->push( (VanadisLoadInstruction*) ins );
				allocated_fu = true;
			}
			break;
			
		case INST_STORE:
			if( ! lsq->storeFull() ) {
//				if( ins->endsMicroOpGroup() ) {
					stat_stores_issued->addData(1);
//				}
			
				lsq->push( (VanadisStoreInstruction*) ins );
				allocated_fu = true;
			}
			break;
			
		case INST_INT_DIV:
			allocated_fu = mapInstructiontoFunctionalUnit( ins, fu_int_div );
			break;
			
		case INST_FP_DIV:
			allocated_fu = mapInstructiontoFunctionalUnit( ins, fu_fp_div );
			break;
			
		case INST_BRANCH:
			allocated_fu = mapInstructiontoFunctionalUnit( ins, fu_branch );
			break;

		case INST_FENCE:
			{
				VanadisFenceInstruction* fence_ins = dynamic_cast<VanadisFenceInstruction*>( ins );

				if( nullptr == fence_ins ) {
					output->fatal(CALL_INFO, -1, "Error: instruction (0x%0llu) is a fence but not convertable to a fence instruction.\n",
						ins->getInstructionAddress());
				}


				allocated_fu = true;

				output->verbose(CALL_INFO, 16, 0, "[fence]: processing ins: 0x%0llx functional unit allocation for fencing (lsq-load size: %" PRIu32 " / lsq-store size: %" PRIu32 ")\n",
							ins->getInstructionAddress(), (uint32_t) lsq->loadSize(), (uint32_t) lsq->storeSize());

				if( fence_ins->createsStoreFence() ) {
					if( lsq->storeSize() == 0 ) {
						allocated_fu = true;
					} else {
						allocated_fu = false;
					}
				}
					
				if( fence_ins->createsLoadFence() ) {
					if( lsq->loadSize() == 0 ) {
						allocated_fu = allocated_fu & true;
					} else {
						allocated_fu = false;
					}
				}

				output->verbose(CALL_INFO, 16, 0, "[fence]: can proceed? %s\n", allocated_fu ? "yes" : "no");

				if( allocated_fu ) {
					ins->markExecuted();
				}
			}
			break;

		case INST_NOOP:
		case INST_FAULT:
			ins->markExecuted();
		case INST_SYSCALL:
			allocated_fu = true;
			break;
		default:
			output->fatal(CALL_INFO, -1, "Error - no processing for instruction class (%s)\n", ins->getInstCode() );
		break;
	}
	
	if( allocated_fu ) {
		return 0;
	} else {
		return 1;
	}
}

bool VANADIS_COMPONENT::tick(SST::Cycle_t cycle) {
	if( current_cycle >= max_cycle ) {
		output->verbose(CALL_INFO, 1, 0, "Reached maximum cycle %" PRIu64 ". Core stops processing.\n", current_cycle );
		primaryComponentOKToEndSim();
		return true;
	}

	stat_cycles->addData(1);
	ins_issued_this_cycle = 0;
	ins_retired_this_cycle = 0;
	ins_decoded_this_cycle = 0;

	bool should_process = false;
	for( uint32_t i = 0; i < hw_threads; ++i ) {
		should_process = should_process | halted_masks[i];
	}

#ifdef VANADIS_BUILD_DEBUG
	output->verbose(CALL_INFO, 2, 0, "============================ Cycle %12" PRIu64 " ============================\n", current_cycle );
	output->verbose(CALL_INFO, 8, 0, "-- Core Status:\n");

	for( uint32_t i = 0; i < hw_threads; ++i) {
		output->verbose(CALL_INFO, 8, 0, "---> Thr: %5" PRIu32 " (%s) / ROB-Pend: %" PRIu16 " / IntReg-Free: %" PRIu16 " / FPReg-Free: %" PRIu16 "\n",
			i, halted_masks[i] ? "halted" : "unhalted",
			(uint16_t) rob[i]->size(),
			(uint16_t) int_register_stacks[i]->unused(), (uint16_t) fp_register_stacks[i]->unused() );
	}

	output->verbose(CALL_INFO, 8, 0, "-- Resetting Zero Registers\n" );
#endif

	for( uint32_t i = 0; i < hw_threads; ++i ) {
		const uint16_t zero_reg = isa_options[i]->getRegisterIgnoreWrites();

		if( zero_reg < isa_options[i]->countISAIntRegisters() ) {
			VanadisISATable* thr_issue_table = issue_isa_tables[i];
			const uint16_t zero_phys_reg = thr_issue_table->getIntPhysReg( zero_reg );
			uint64_t* reg_ptr = (uint64_t*) register_files[i]->getIntReg( zero_phys_reg );
			*(reg_ptr) = 0;
		}
	}

	// Fetch  //////////////////////////////////////////////////////////////////////////
#ifdef VANADIS_BUILD_DEBUG
	output->verbose(CALL_INFO, 8, 0, "=> Fetch Stage <==========================================================\n");
#endif
	for( uint32_t i = 0; i < fetches_per_cycle; ++i ) {
		if( performFetch( cycle ) != 0 ) {
			break;
		}
	}

	// Decode //////////////////////////////////////////////////////////////////////////
#ifdef VANADIS_BUILD_DEBUG
	output->verbose(CALL_INFO, 8, 0, "=> Decode Stage <==========================================================\n");
#endif
	for( uint32_t i = 0; i < decodes_per_cycle; ++i ) {
		if( performDecode( cycle ) != 0 ) {
			break;
		}
	}

	stat_ins_decoded->addData( ins_decoded_this_cycle );

	// Issue  //////////////////////////////////////////////////////////////////////////
#ifdef VANADIS_BUILD_DEBUG
	output->verbose(CALL_INFO, 8, 0, "=> Issue Stage  <==========================================================\n");
#endif
	for( uint32_t i = 0; i < issues_per_cycle; ++i ) {
		if( performIssue( cycle ) != 0 ) {
			break;
		}
	}

	// Record how many instructions we issued this cycle
	stat_ins_issued->addData( ins_issued_this_cycle );

	// Execute //////////////////////////////////////////////////////////////////////////
#ifdef VANADIS_BUILD_DEBUG
	output->verbose(CALL_INFO, 8, 0, "=> Execute Stage <==========================================================\n");
#endif
	performExecute( cycle );

	bool tick_return = false;

	// Retire //////////////////////////////////////////////////////////////////////////
	for( uint32_t i = 0; i < retires_per_cycle; ++i ) {
		for( uint32_t j = 0; j < rob.size(); ++j ) {
			const int retire_rc = performRetire( rob[j], cycle );

			if( retire_rc == INT_MAX ) {
				// we will return true and tell the handler not to clock us until re-register
				tick_return = true;
#ifdef VANADIS_BUILD_DEBUG
				output->verbose(CALL_INFO, 8, 0, "--> declocking core, result from retire is SYSCALL pending front of ROB\n");
#endif
			} else {
				// Signal from retire calls that we can't make progress is non-zero
				if( retire_rc != 0 ) {
					break;
				}
			}
		}
	}

	// Record how many instructions we retired this cycle
	stat_ins_retired->addData( ins_retired_this_cycle );

	uint64_t rob_total_count = 0;
	for( uint32_t i = 0; i < hw_threads; ++i ) {
		rob_total_count += rob[i]->size();
	}
	stat_rob_entries->addData( rob_total_count );

#ifdef VANADIS_BUILD_DEBUG
	output->verbose(CALL_INFO, 2, 0, "================================ End of Cycle ==============================\n" );
#endif

	current_cycle++;

	uint64_t used_phys_int = 0;
	uint64_t used_phys_fp  = 0;

	for( uint16_t i = 0; i < hw_threads; ++i ) {
		VanadisRegisterStack* thr_reg_stack = int_register_stacks[i];
		used_phys_int += ( thr_reg_stack->capacity() - thr_reg_stack->size() );

		thr_reg_stack = fp_register_stacks[i];
		used_phys_fp += ( thr_reg_stack->capacity() - thr_reg_stack->size() );
	}

	stat_int_phys_regs_in_use->addData( used_phys_int );
	stat_fp_phys_regs_in_use->addData( used_phys_fp );

	if( current_cycle >= max_cycle ) {
		output->verbose(CALL_INFO, 1, 0, "Reached maximum cycle %" PRIu64 ". Core stops processing.\n", current_cycle );
		primaryComponentOKToEndSim();
		return true;
	} else{
		return tick_return;
	}
}

int VANADIS_COMPONENT::checkInstructionResources(
	VanadisInstruction* ins,
    	VanadisRegisterStack* int_regs,
    	VanadisRegisterStack* fp_regs,
    	VanadisISATable* isa_table ) {

	bool resources_good = true;
	const int output_verbosity = output->getVerboseLevel();

	// We need places to store our output registers
	resources_good &= (int_regs->unused() >= ins->countISAIntRegOut());
	resources_good &= (fp_regs->unused() >= ins->countISAFPRegOut());

	if( ! resources_good ) {
#ifdef VANADIS_BUILD_DEBUG
		output->verbose(CALL_INFO, 16, 0, "----> insufficient output / req: int: %" PRIu16 " fp: %" PRIu16 " / free: int: %" PRIu16 " fp: %" PRIu16 "\n",
			(uint16_t) ins->countISAIntRegOut(), (uint16_t) ins->countISAFPRegOut(), (uint16_t) int_regs->unused(), (uint16_t) fp_regs->unused() );
#endif
		return 1;
	}

	// If there are any pending writes against our reads, we can't issue until
	// they are done
	const uint16_t int_reg_in_count = ins->countISAIntRegIn();
	for( uint16_t i = 0; i < int_reg_in_count; ++i ) {
		const uint16_t ins_isa_reg = ins->getISAIntRegIn(i);
		resources_good &= (!isa_table->pendingIntWrites(ins_isa_reg));

		// Check there are no RAW in the pending instruction queue
		resources_good &= (!tmp_int_reg_write[ins_isa_reg]);
	}

#ifdef VANADIS_BUILD_DEBUG
	if( output_verbosity >= 16 ) {
		output->verbose(CALL_INFO, 16, 0, "--> Check input integer registers, issue-status: %s\n",
			(resources_good ? "yes" : "no") );
	}
#endif

	if( ! resources_good ) {
		return 2;
	}

	const uint16_t fp_reg_in_count = ins->countISAFPRegIn();
	for( uint16_t i = 0; i < fp_reg_in_count; ++i ) {
		const uint16_t ins_isa_reg = ins->getISAFPRegIn(i);
		resources_good &= (! isa_table->pendingFPWrites(ins_isa_reg));

		// Check there are no RAW in the pending instruction queue
		resources_good &= (! tmp_fp_reg_write[ins_isa_reg]);
	}

#ifdef VANADIS_BUILD_DEBUG
	if( output_verbosity >= 16 ) {
		output->verbose(CALL_INFO, 16, 0, "--> Check input floating-point registers, issue-status: %s\n",
			(resources_good ? "yes" : "no") );
	}
#endif

	if( ! resources_good ) {
		return 3;
	}

	const uint16_t int_reg_out_count = ins->countISAIntRegOut();
	for( uint16_t i = 0; i < int_reg_out_count; ++i ) {
		const uint16_t ins_isa_reg = ins->getISAIntRegOut(i);

		// Check there are no RAW in the pending instruction queue
		resources_good &= (!tmp_not_issued_int_reg_read[ins_isa_reg]);
	}

#ifdef VANADIS_BUILD_DEBUG
	if( output_verbosity >= 16 ) {
		output->verbose(CALL_INFO, 16, 0, "--> Check output integer registers, issue-status: %s\n",
			(resources_good ? "yes" : "no") );
	}
#endif

	if( ! resources_good ) {
		return 4;
	}

	const uint16_t fp_reg_out_count = ins->countISAFPRegOut();
	for( uint16_t i = 0; i < ins->countISAFPRegOut(); ++i ) {
		const uint16_t ins_isa_reg = ins->getISAFPRegOut(i);

		// Check there are no RAW in the pending instruction queue
		resources_good &= (!tmp_not_issued_fp_reg_read[ins_isa_reg]);
	}

#ifdef VANADIS_BUILD_DEBUG
	if( output_verbosity >= 16 ) {
		output->verbose(CALL_INFO, 16, 0, "--> Check output floating-point registers, issue-status: %s\n",
			(resources_good ? "yes" : "no") );
	}
#endif

	if( ! resources_good ) {
		return 5;
	}

	return 0;
}

int VANADIS_COMPONENT::assignRegistersToInstruction(
	const uint16_t int_reg_count,
	const uint16_t fp_reg_count,
        VanadisInstruction* ins,
        VanadisRegisterStack* int_regs,
        VanadisRegisterStack* fp_regs,
        VanadisISATable* isa_table) {

	// PROCESS INPUT REGISTERS  ///////////////////////////////////////////////////////

	// Set the current ISA registers required for input
	for( uint16_t i = 0; i < ins->countISAIntRegIn(); ++i ) {
		if( ins->getISAIntRegIn(i) >= int_reg_count ) {
			output->fatal(CALL_INFO, -1, "Error: ins request in-int-reg: %" PRIu16 " but ISA has only %" PRIu16 " available\n",
				ins->getISAIntRegIn(i), int_reg_count );
		}

		ins->setPhysIntRegIn(i, isa_table->getIntPhysReg( ins->getISAIntRegIn(i) ));
		isa_table->incIntRead( ins->getISAIntRegIn(i) );
	}
		
	for( uint16_t i = 0; i < ins->countISAFPRegIn(); ++i ) {
		if( ins->getISAFPRegIn(i) >= fp_reg_count ) {
			output->fatal(CALL_INFO, -1, "Error: ins request in-fp-reg: %" PRIu16 " but ISA has only %" PRIu16 " available\n",
				ins->getISAFPRegIn(i), fp_reg_count);
		}

		ins->setPhysFPRegIn(i, isa_table->getFPPhysReg( ins->getISAFPRegIn(i) ));
		isa_table->incFPRead( ins->getISAFPRegIn(i) );
	}
	
	// PROCESS OUTPUT REGISTERS ///////////////////////////////////////////////////////

	// SYSCALLs have special handling because they request *every* register to lock up
	// the pipeline. We just give them full access to the register file without requiring
	// anything from the register file (otherwise we exhaust registers). This REQUIRES
	// that SYSCALL doesn't muck with the registers itself at execute and relies on the
	// OS handlers, otherwise this is not out-of-order compliant (since mis-predicts would
	// corrupt the register file contents
	if( INST_SYSCALL == ins->getInstFuncType() ) {
		for( uint16_t i = 0; i < ins->countISAIntRegOut(); ++i ) {
			if( ins->getISAIntRegOut(i) >= int_reg_count ) {
				output->fatal(CALL_INFO, -1, "Error: ins request out-int-reg: %" PRIu16 " but ISA has only %" PRIu16 " available\n",
					ins->getISAIntRegOut(i), int_reg_count);
			}

			const uint16_t ins_isa_reg = ins->getISAIntRegOut(i);
			const uint16_t out_reg = isa_table->getIntPhysReg( ins_isa_reg );
	
			ins->setPhysIntRegOut(i, out_reg);
			isa_table->incIntWrite( ins_isa_reg );
		}

		// Set current ISA registers required for output
		for( uint16_t i = 0; i < ins->countISAFPRegOut(); ++i ) {
			if( ins->getISAFPRegOut(i) >= fp_reg_count ) {
				output->fatal(CALL_INFO, -1, "Error: ins request out-fp-reg: %" PRIu16 " but ISA has only %" PRIu16 " available\n",
					ins->getISAFPRegOut(i), fp_reg_count);
			}

			const uint16_t ins_isa_reg = ins->getISAFPRegOut(i);
			const uint16_t out_reg = isa_table->getFPPhysReg( ins_isa_reg );

			ins->setPhysFPRegOut(i, out_reg);
			isa_table->incFPWrite( ins_isa_reg );
		}
	} else {

		// Set the current ISA registers required for input
/*		for( uint16_t i = 0; i < ins->countISAIntRegIn(); ++i ) {
			if( ins->getISAIntRegIn(i) >= int_reg_count ) {
				output->fatal(CALL_INFO, -1, "Error: ins request in-int-reg: %" PRIu16 " but ISA has only %" PRIu16 " available\n",
					ins->getISAIntRegIn(i), int_reg_count );
			}

			ins->setPhysIntRegIn(i, isa_table->getIntPhysReg( ins->getISAIntRegIn(i) ));
			isa_table->incIntRead( ins->getISAIntRegIn(i) );
		}

		for( uint16_t i = 0; i < ins->countISAFPRegIn(); ++i ) {
			if( ins->getISAFPRegIn(i) >= fp_reg_count ) {
				output->fatal(CALL_INFO, -1, "Error: ins request in-fp-reg: %" PRIu16 " but ISA has only %" PRIu16 " available\n",
					ins->getISAFPRegIn(i), fp_reg_count);
			}

			ins->setPhysFPRegIn(i, isa_table->getFPPhysReg( ins->getISAFPRegIn(i) ));
			isa_table->incFPRead( ins->getISAFPRegIn(i) );
		}
*/
		// Set current ISA registers required for output
		for( uint16_t i = 0; i < ins->countISAIntRegOut(); ++i ) {
			if( ins->getISAIntRegOut(i) >= int_reg_count ) {
				output->fatal(CALL_INFO, -1, "Error: ins request out-int-reg: %" PRIu16 " but ISA has only %" PRIu16 " available\n",
					ins->getISAIntRegOut(i), int_reg_count);
			}

			const uint16_t ins_isa_reg = ins->getISAIntRegOut(i);
			bool reg_is_also_in = false;

			for( uint16_t j = 0; j < ins->countISAIntRegIn(); ++j ) {
				if( ins_isa_reg == ins->getISAIntRegIn(j) ) {
					reg_is_also_in = true;
					break;
				}
			}

			//if( reg_is_also_in ) {
			//	out_reg = isa_table->getIntPhysReg( ins_isa_reg );
			
				//if( ins_isa_reg > 0 ) 
				//	output->fatal(CALL_INFO, -1, "STOP INSTRUCTION 0x%llx\n", ins->getInstructionAddress());
			//} else {
				const uint16_t out_reg = int_regs->pop();
				isa_table->setIntPhysReg( ins_isa_reg, out_reg );
			//}

			ins->setPhysIntRegOut(i, out_reg);
			isa_table->incIntWrite( ins_isa_reg );
		}

		// Set current ISA registers required for output
		for( uint16_t i = 0; i < ins->countISAFPRegOut(); ++i ) {
			if( ins->getISAFPRegOut(i) >= fp_reg_count ) {
				output->fatal(CALL_INFO, -1, "Error: ins request out-fp-reg: %" PRIu16 " but ISA has only %" PRIu16 " available\n",
					ins->getISAFPRegOut(i), fp_reg_count);
			}

			const uint16_t ins_isa_reg = ins->getISAFPRegOut(i);
/*			bool reg_is_also_in = false;

			for( uint16_t j = 0; j < ins->countISAFPRegIn(); ++j ) {
				if( ins_isa_reg == ins->getISAFPRegIn(j) ) {
					reg_is_also_in = true;
					break;
				}
			}

			uint16_t out_reg;

			if( reg_is_also_in ) {
				out_reg = isa_table->getFPPhysReg( ins_isa_reg );
			} else {*/
				const uint16_t out_reg = fp_regs->pop();
				isa_table->setFPPhysReg( ins_isa_reg, out_reg );
//			}

			ins->setPhysFPRegOut(i, out_reg);
			isa_table->incFPWrite( ins_isa_reg );
		}
	}

	return 0;
}

int VANADIS_COMPONENT::recoverRetiredRegisters(
        VanadisInstruction* ins,
        VanadisRegisterStack* int_regs,
        VanadisRegisterStack* fp_regs,
		VanadisISATable* issue_isa_table,
        VanadisISATable* retire_isa_table) {

	std::vector<uint16_t> recovered_phys_reg_int;
	std::vector<uint16_t> recovered_phys_reg_fp;
	
	for( uint16_t i = 0; i < ins->countISAIntRegIn(); ++i ) {
		const uint16_t isa_reg = ins->getISAIntRegIn(i);
		issue_isa_table->decIntRead( isa_reg );
	}

	for( uint16_t i = 0; i < ins->countISAFPRegIn(); ++i ) {
		const uint16_t isa_reg = ins->getISAFPRegIn(i);
		issue_isa_table->decFPRead( isa_reg );
	}
	
	if( ins->performIntRegisterRecovery() ) {
		for( uint16_t i = 0; i < ins->countISAIntRegOut(); ++i ) {
			const uint16_t isa_reg = ins->getISAIntRegOut(i);
   			const uint16_t cur_phys_reg = retire_isa_table->getIntPhysReg(isa_reg);

			issue_isa_table->decIntWrite( isa_reg );

			// Check this register isn't also in our input set because then we
			// wouldn't have allocated a new register for it
			//bool reg_also_input = false;

			//for( uint16_t j = 0; j < ins->countISAIntRegIn(); ++j ) {
			//	if( isa_reg == ins->getISAIntRegIn(j) ) {
			//		reg_also_input = true;
			//	}
			//}

			//if( ! reg_also_input ) {
				recovered_phys_reg_int.push_back( cur_phys_reg );

				// Set the ISA register in the retirement table to point
				// to the physical register used by this instruction
				retire_isa_table->setIntPhysReg( isa_reg, ins->getPhysIntRegOut(i) );
			//}
		}
	} else {
#ifdef VANADIS_BUILD_DEBUG
		output->verbose(CALL_INFO, 16, 0, "-> instruction bypasses integer register recovery\n");
#endif		
		for( uint16_t i = 0; i < ins->countISAIntRegOut(); ++i ) {
			const uint16_t isa_reg = ins->getISAIntRegOut(i);
   			const uint16_t cur_phys_reg = retire_isa_table->getIntPhysReg(isa_reg);
		
			issue_isa_table->decIntWrite( isa_reg );
		}
	}
	
	if( ins->performFPRegisterRecovery() ) {	
		for( uint16_t i = 0; i < ins->countISAFPRegOut(); ++i ) {
			const uint16_t isa_reg = ins->getISAFPRegOut(i);
			const uint16_t cur_phys_reg = retire_isa_table->getFPPhysReg(isa_reg);

			issue_isa_table->decFPWrite( isa_reg );

			// Check this register isn't also in our input set because then we
			// wouldn't have allocated a new register for it
			//bool reg_also_input = false;
//
			//for( uint16_t j = 0; j < ins->countISAIntRegIn(); ++j ) {
			//	if( isa_reg == ins->getISAIntRegIn(j) ) {
			//		reg_also_input = true;
			//	}
			//}

			//if( ! reg_also_input ) {
				recovered_phys_reg_fp.push_back( cur_phys_reg );

				// Set the ISA register in the retirement table to point
				// to the physical register used by this instruction
				retire_isa_table->setFPPhysReg( isa_reg, ins->getPhysFPRegOut(i) );
			//}
		}
	} else {
#ifdef VANADIS_BUILD_DEBUG
		output->verbose(CALL_INFO, 16, 0, "-> instruction bypasses fp register recovery\n");
#endif
		for( uint16_t i = 0; i < ins->countISAFPRegOut(); ++i ) {
			const uint16_t isa_reg = ins->getISAFPRegOut(i);
			const uint16_t cur_phys_reg = retire_isa_table->getFPPhysReg(isa_reg);

			issue_isa_table->decFPWrite( isa_reg );
		}
	}

#ifdef VANADIS_BUILD_DEBUG
	output->verbose(CALL_INFO, 16, 0, "Recovering: %d int-reg and %d fp-reg\n",
		(int) recovered_phys_reg_int.size(), (int) recovered_phys_reg_fp.size());
#endif

	for( uint16_t next_reg : recovered_phys_reg_int ) {
		int_regs->push(next_reg);
	}

	for( uint16_t next_reg : recovered_phys_reg_fp ) {
		fp_regs->push(next_reg);
	}

	return 0;
}

void VANADIS_COMPONENT::setup() {

}

void VANADIS_COMPONENT::finish() {

}

void VANADIS_COMPONENT::printStatus( SST::Output& output ) {
	output.verbose(CALL_INFO, 0, 0, "----------------------------------------------------------------------------------------------------------------------------\n");
	output.verbose(CALL_INFO, 0, 0, "Vanadis (Core: %" PRIu16 " / Threads: %" PRIu32 " / cycle: %" PRIu64 " / max-cycle: %" PRIu64 ")\n",
		core_id, hw_threads, current_cycle, max_cycle);
	output.verbose(CALL_INFO, 0, 0, "\n");

	uint32_t thread_num = 0;

	for( VanadisCircularQueue< VanadisInstruction* >* next_rob : rob ) {
		output.verbose(CALL_INFO, 0, 0, "-> ROB Information for Thread %" PRIu32 "\n", thread_num++ );
		for( size_t i = next_rob->size(); i > 0; i-- ) {
			VanadisInstruction* next_ins = next_rob->peekAt(i-1);
			output.verbose(CALL_INFO, 0, 0, "---> rob[%5" PRIu16 "]: addr: 0x%08llx / %10s / spec: %3s / err: %3s / issued: %3s / front: %3s / exe: %3s\n",
				(uint16_t) (i-1),
				next_ins->getInstructionAddress(), next_ins->getInstCode(),
				next_ins->isSpeculated() ? "yes" : "no",
				next_ins->trapsError() ? "yes" : "no",
				next_ins->completedIssue() ? "yes" : "no",
				next_ins->checkFrontOfROB() ? "yes" : "no",
				next_ins->completedExecution() ? "yes" : "no");
		}
	}

	output.verbose(CALL_INFO, 0, 0, "\n");
	output.verbose(CALL_INFO, 0, 0, "-> LSQ-Size: %" PRIu32 "\n", (uint32_t) (lsq->storeSize() + lsq->loadSize()) );
	lsq->printStatus( output );
	output.verbose(CALL_INFO, 0, 0, "----------------------------------------------------------------------------------------------------------------------------\n");
}

void VANADIS_COMPONENT::init(unsigned int phase) {
	output->verbose(CALL_INFO, 2, 0, "Start: init-phase: %" PRIu32 "...\n", (uint32_t) phase );
	output->verbose(CALL_INFO, 2, 0, "-> Initializing memory interfaces with this phase...\n");

	lsq->init( phase );
//	memDataInterface->init( phase );
	memInstInterface->init( phase );

	output->verbose(CALL_INFO, 2, 0, "-> Performing init operations for Vanadis...\n");

	// Pull in all the binary contents and push this into the memory system
	// everything should be correctly up and running by now (components, links etc)
	if( 0 == phase ) {
		if( nullptr != binary_elf_info ) {
			if( 0 == core_id ) {
				output->verbose(CALL_INFO, 2, 0, "-> Loading %s, to locate program sections ...\n",
					binary_elf_info->getBinaryPath());
				FILE* exec_file = fopen( binary_elf_info->getBinaryPath(), "rb" );

				if( nullptr == exec_file ) {
					output->fatal(CALL_INFO, -1, "Error: unable to open %s\n", binary_elf_info->getBinaryPath());
				}

				std::vector<uint8_t> initial_mem_contents;
				uint64_t max_content_address = 0;

				// Find the max value we think we are going to need to place entries up to

				for( size_t i = 0; i < binary_elf_info->countProgramHeaders(); ++i ) {
					const VanadisELFProgramHeaderEntry* next_prog_hdr = binary_elf_info->getProgramHeader(i);
					max_content_address = std::max( max_content_address, (uint64_t) next_prog_hdr->getVirtualMemoryStart() +
						next_prog_hdr->getHeaderImageLength() );
				}

				for( size_t i = 0 ; i < binary_elf_info->countProgramSections(); ++i ) {
					const VanadisELFProgramSectionEntry* next_sec = binary_elf_info->getProgramSection( i );
					max_content_address = std::max( max_content_address, (uint64_t) next_sec->getVirtualMemoryStart() +
						next_sec->getImageLength() );
				}

				output->verbose(CALL_INFO, 2, 0, "-> expecting max address for initial binary load is 0x%llx, zeroing the memory\n",
					max_content_address );
				initial_mem_contents.resize( max_content_address, (uint8_t) 0 );

				// Populate the memory with contents from the binary
				output->verbose(CALL_INFO, 2, 0, "-> populating memory contents with info from the executable...\n");
/*
  				for( size_t i = 0; i < binary_elf_info->countProgramHeaders(); ++i ) {
					const VanadisELFProgramHeaderEntry* next_prog_hdr = binary_elf_info->getProgramHeader(i);

					if( (PROG_HEADER_LOAD == next_prog_hdr->getHeaderType()) ) {
						output->verbose(CALL_INFO, 2, 0, ">> Loading Program Header from executable at %p, len=%" PRIu64 "...\n",
							(void*) next_prog_hdr->getImageOffset(), next_prog_hdr->getHeaderImageLength());
						output->verbose(CALL_INFO, 2, 0, ">>>> Placing at virtual address: %p\n",
							(void*) next_prog_hdr->getVirtualMemoryStart());

						// Note - padding automatically zeros because we perform a resize with parameter 0 given
						const uint64_t padding = 4096 - ((next_prog_hdr->getVirtualMemoryStart() + next_prog_hdr->getHeaderImageLength()) % 4096);

						// Do we have enough space in the memory image, if not, extend and zero
						if( initial_mem_contents.size() < ( next_prog_hdr->getVirtualMemoryStart() + next_prog_hdr->getHeaderImageLength() )) {
							initial_mem_contents.resize( ( next_prog_hdr->getVirtualMemoryStart() + next_prog_hdr->getHeaderImageLength()
								+ padding), 0);
						}

						fseek( exec_file, next_prog_hdr->getImageOffset(), SEEK_SET );
						fread( &initial_mem_contents[ next_prog_hdr->getVirtualMemoryStart() ],
							next_prog_hdr->getHeaderImageLength(), 1, exec_file);
					}
				}
*/
				for( size_t i = 0; i < binary_elf_info->countProgramSections(); ++i ) {
					const VanadisELFProgramSectionEntry* next_sec = binary_elf_info->getProgramSection( i );

					if( SECTION_HEADER_PROG_DATA == next_sec->getSectionType() ) {
						output->verbose(CALL_INFO, 2, 0, ">> Loading Section (%" PRIu64 ") from executable at: 0x%0llx, len=%" PRIu64 "...\n",
							next_sec->getID(), next_sec->getVirtualMemoryStart(), next_sec->getImageLength());

						if( next_sec->getVirtualMemoryStart() > 0 ) {
							const uint64_t padding = 4096 - ((next_sec->getVirtualMemoryStart() + next_sec->getImageLength()) % 4096);

							// Executable data, let's load it in
							if( initial_mem_contents.size() < (next_sec->getVirtualMemoryStart() + next_sec->getImageLength()) ) {
								size_t size_now = initial_mem_contents.size();
								initial_mem_contents.resize( next_sec->getVirtualMemoryStart() + next_sec->getImageLength() +
									padding, 0 );
							}

							// Find the section and read it all in
							fseek( exec_file, next_sec->getImageOffset(), SEEK_SET );
							fread( &initial_mem_contents[next_sec->getVirtualMemoryStart()],
								next_sec->getImageLength(), 1, exec_file);
						} else {
							output->verbose(CALL_INFO, 2, 0, "--> Not loading because virtual address is zero.\n");
						}
					} else if( SECTION_HEADER_BSS == next_sec->getSectionType() ) {
						output->verbose(CALL_INFO, 2, 0, ">> Loading BSS Section (%" PRIu64 ") with zeroing at 0x%0llx, len=%" PRIu64 "\n",
							next_sec->getID(), next_sec->getVirtualMemoryStart(), next_sec->getImageLength());

						if( next_sec->getVirtualMemoryStart() > 0 ) {
							const uint64_t padding = 4096 - ((next_sec->getVirtualMemoryStart() + next_sec->getImageLength()) % 4096);

       		                                         // Resize if needed with zeroing
               		                                 if( initial_mem_contents.size() < (next_sec->getVirtualMemoryStart() + next_sec->getImageLength()) ) {
                        	                                size_t size_now = initial_mem_contents.size();
                                	                        initial_mem_contents.resize( next_sec->getVirtualMemoryStart() + next_sec->getImageLength() +
                                       		                         padding, 0 );
                                                	}

							// Zero out the section according to the Section header info
							std::memset( &initial_mem_contents[next_sec->getVirtualMemoryStart()], 0, next_sec->getImageLength());
						} else {
							output->verbose(CALL_INFO, 2, 0, "--> Not loading because virtual address is zero.\n");
						}
					} else {
						if( next_sec->isAllocated() ) {
							output->verbose(CALL_INFO, 2, 0, ">> Loading Allocatable Section (%" PRIu64 ") at 0x%0llx, len: %" PRIu64 "\n",
								next_sec->getID(), next_sec->getVirtualMemoryStart(), next_sec->getImageLength());

							if( next_sec->getVirtualMemoryStart() > 0 ) {
								const uint64_t padding = 4096 - ((next_sec->getVirtualMemoryStart() + next_sec->getImageLength()) % 4096);

                        	                                 // Resize if needed with zeroing
                	                                         if( initial_mem_contents.size() < (next_sec->getVirtualMemoryStart() + next_sec->getImageLength()) ) {
        	                                                        size_t size_now = initial_mem_contents.size();
	                                                                initial_mem_contents.resize( next_sec->getVirtualMemoryStart() + next_sec->getImageLength() +
                                                                	         padding, 0 );
                                                        	}

								// Find the section and read it all in
	                                                        fseek( exec_file, next_sec->getImageOffset(), SEEK_SET );
        	                                                fread( &initial_mem_contents[next_sec->getVirtualMemoryStart()],
                	                                                next_sec->getImageLength(), 1, exec_file);
							}
						}
					}
				}

				fclose(exec_file);

				output->verbose(CALL_INFO, 2, 0, ">> Writing memory contents (%" PRIu64 " bytes at index 0)\n",
					(uint64_t) initial_mem_contents.size());

//				SimpleMem::Request* writeExe = new SimpleMem::Request(SimpleMem::Request::Write,
//					0, initial_mem_contents.size(), initial_mem_contents);
				lsq->setInitialMemory( 0, initial_mem_contents );
//				memInstInterface->sendInitData( writeExe );

				const uint64_t page_size = 4096;

				uint64_t initial_brk = (uint64_t) initial_mem_contents.size();
				initial_brk = initial_brk + (page_size - (initial_brk % page_size));

				output->verbose(CALL_INFO, 2, 0, ">> Setting initial break point to image size in memory ( brk: 0x%llx )\n", initial_brk );
				thread_decoders[0]->getOSHandler()->registerInitParameter( SYSCALL_INIT_PARAM_INIT_BRK, &initial_brk );
			} else {
				output->verbose(CALL_INFO, 2, 0, "Not core-0, so will not perform any loading of binary info.\n");
			}
		} else {
			output->verbose(CALL_INFO, 2, 0, "No ELF binary information loaded, will not perform any loading.\n");
		}
	}

	output->verbose(CALL_INFO, 2, 0, "End: init-phase: %" PRIu32 "...\n", (uint32_t) phase );
}

//void VANADIS_COMPONENT::handleIncomingDataCacheEvent( SimpleMem::Request* ev ) {
//	output->verbose(CALL_INFO, 16, 0, "-> Incoming d-cache event (addr=%p)...\n",
//		(void*) ev->addr);
//	lsq->processIncomingDataCacheEvent( output, ev );
//}

void VANADIS_COMPONENT::handleIncomingInstCacheEvent( SimpleMem::Request* ev ) {
#ifdef VANADIS_BUILD_DEBUG
	output->verbose(CALL_INFO, 16, 0, "-> Incoming i-cache event (addr=%p)...\n",
		(void*) ev->addr);
#endif
	// Needs to get attached to the decoder
	bool hit = false;

	for( VanadisDecoder* next_decoder : thread_decoders ) {
		if( next_decoder->acceptCacheResponse( output, ev ) ) {
			hit = true;
			break;
		}
	}

	if( hit ) {
		output->verbose(CALL_INFO, 16, 0, "---> Successful hit in hardware-thread decoders.\n");
	}

	delete ev;
}

void VANADIS_COMPONENT::handleMisspeculate( const uint32_t hw_thr, const uint64_t new_ip ) {
#ifdef VANADIS_BUILD_DEBUG
	output->verbose(CALL_INFO, 16, 0, "-> Handle mis-speculation on %" PRIu32 " (new-ip: 0x%llx)...\n", hw_thr, new_ip);
#endif
	clearFuncUnit( hw_thr, fu_int_arith );
	clearFuncUnit( hw_thr, fu_int_div );
	clearFuncUnit( hw_thr, fu_fp_arith );
	clearFuncUnit( hw_thr, fu_fp_div );
	clearFuncUnit( hw_thr, fu_branch );

	lsq->clearLSQByThreadID( hw_thr );
	resetRegisterStacks( hw_thr );
	clearROBMisspeculate(hw_thr);

	// Reset the ISA table to get correct ISA to physical mappings
	issue_isa_tables[hw_thr]->reset( retire_isa_tables[hw_thr] );

	// Notify the decoder we need a clear and reset to new instruction pointer
	thread_decoders[hw_thr]->setInstructionPointerAfterMisspeculate( output, new_ip );

	output->verbose(CALL_INFO, 16, 0, "-> Mis-speculate repair finished.\n");
}

void VANADIS_COMPONENT::clearFuncUnit( const uint32_t hw_thr, std::vector<VanadisFunctionalUnit*>& unit ) {
	for( VanadisFunctionalUnit* next_fu : unit ) {
		next_fu->clearByHWThreadID( output, hw_thr );
	}
}

void VANADIS_COMPONENT::resetRegisterStacks( const uint32_t hw_thr ) {
	const uint16_t int_reg_count = int_register_stacks[hw_thr]->capacity();
#ifdef VANADIS_BUILD_DEBUG
	output->verbose(CALL_INFO, 16, 0, "-> Resetting register stacks on thread %" PRIu32 "...\n", hw_thr);
	output->verbose(CALL_INFO, 16, 0, "---> Reclaiming integer registers...\n");
	output->verbose(CALL_INFO, 16, 0, "---> Creating a new int register stack with %" PRIu16 " registers...\n", int_reg_count);
#endif
	VanadisRegisterStack* thr_int_stack = int_register_stacks[hw_thr];
	thr_int_stack->clear();

	for( uint16_t i = 0; i < int_reg_count; ++i ) {
		if( ! retire_isa_tables[ hw_thr ]->physIntRegInUse( i ) ) {
			thr_int_stack->push(i);
		}
	}

//	delete int_register_stacks[hw_thr];
//	int_register_stacks[hw_thr] = new_int_stack;
	const uint16_t fp_reg_count = fp_register_stacks[hw_thr]->capacity();
#ifdef VANADIS_BUILD_DEBUG
	output->verbose(CALL_INFO, 16, 0, "---> Integer register stack contains %" PRIu32 " registers.\n",
		(uint32_t) thr_int_stack->size());
	output->verbose(CALL_INFO, 16, 0, "---> Reclaiming floating point registers...\n");

	output->verbose(CALL_INFO, 16, 0, "---> Creating a new fp register stack with %" PRIu16 " registers...\n", fp_reg_count);
#endif
//	VanadisRegisterStack* new_fp_stack = new VanadisRegisterStack( fp_reg_count );
	VanadisRegisterStack* thr_fp_stack = fp_register_stacks[hw_thr];
	thr_fp_stack->clear();

	for( uint16_t i = 0; i < fp_reg_count; ++i ) {
		if( ! retire_isa_tables[ hw_thr ]->physFPRegInUse( i ) ) {
			thr_fp_stack->push(i);
		}
	}

//	delete fp_register_stacks[hw_thr];
//	fp_register_stacks[hw_thr] = new_fp_stack;

#ifdef VANADIS_BUILD_DEBUG
	output->verbose(CALL_INFO, 16, 0, "---> Floating point stack contains %" PRIu32 " registers.\n",
		(uint32_t) thr_fp_stack->size());
#endif
}

void VANADIS_COMPONENT::clearROBMisspeculate( const uint32_t hw_thr ) {
	VanadisCircularQueue<VanadisInstruction*>* thr_rob = rob[hw_thr];
	stat_rob_cleared_entries->addData(thr_rob->size());

	// Delete all the instructions which we aren't going to process
	for( size_t i = 0; i < thr_rob->size(); ++i ) {
		VanadisInstruction* next_ins = thr_rob->peekAt(i);
		delete next_ins;
	}

	// clear the ROB entries and reset
	thr_rob->clear();
}

void VANADIS_COMPONENT::syscallReturnCallback( uint32_t thr ) {
	if( rob[thr]->empty() ) {
		output->fatal(CALL_INFO, -1, "Error - syscall return called on thread: %" PRIu32 " but ROB is empty.\n", thr);
	}

	VanadisInstruction* rob_front = rob[thr]->peek();
	VanadisSysCallInstruction* syscall_ins = dynamic_cast<VanadisSysCallInstruction*>( rob_front );

	if( nullptr == syscall_ins ) {
		output->fatal(CALL_INFO, -1, "Error - unable to obtain a syscall from the ROB front of thread %" PRIu32 " (code: %s)\n", thr,
			rob_front->getInstCode());
	}

#ifdef VANADIS_BUILD_DEBUG
	output->verbose(CALL_INFO, 16, 0, "[syscall-return]: syscall on thread %" PRIu32 " (0x%0llx) is completed, return to processing.\n",
		thr, syscall_ins->getInstructionAddress());
#endif
	syscall_ins->markExecuted();

	// Set back to false ready for the next SYSCALL
	handlingSysCall = false;

	// re-register the CPU clock, it will fire on the next cycle
	reregisterClock( cpuClockTC, cpuClockHandler );
}
