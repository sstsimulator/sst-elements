

#include <sst_config.h>
#include <sst/core/output.h>

#include <cstdio>
#include <vector>

#include "vanadis.h"

#include "velf/velfinfo.h"
#include "decoder/vmipsdecoder.h"
#include "inst/vinstall.h"

using namespace SST::Vanadis;

VanadisComponent::VanadisComponent(SST::ComponentId_t id, SST::Params& params) :
	Component(id),
	current_cycle(0) {

	instPrintBuffer = new char[1024];

	max_cycle = params.find<uint64_t>("max_cycle",
		std::numeric_limits<uint64_t>::max() );

	const int32_t verbosity = params.find<int32_t>("verbose", 0);
	core_id   = params.find<uint32_t>("core_id", 0);

	char* outputPrefix = (char*) malloc( sizeof(char) * 256 );
	sprintf(outputPrefix, "[Core: %6" PRIu32 "]: ", core_id);

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
	}

	std::string clock_rate = params.find<std::string>("clock", "1GHz");
	output->verbose(CALL_INFO, 2, 0, "Registering clock at %s.\n", clock_rate.c_str());
	cpuClockTC = registerClock( clock_rate, new Clock::Handler<VanadisComponent>(this, &VanadisComponent::tick) );

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
		thr_decoder->setHardwareThread( i );

		output->verbose(CALL_INFO, 8, 0, "Loading decoder%" PRIu32 ": %s.\n", i,
			(nullptr == thr_decoder) ? "failed" : "successful");

		if( nullptr == thr_decoder ) {
			output->fatal(CALL_INFO, -1, "Error: was unable to load %s on thread %" PRIu32 "\n",
				decoder_name, i);
		} else {
			output->verbose(CALL_INFO, 8, 0, "-> Decoder configured for %s\n",
				thr_decoder->getISAName());
		}

		thread_decoders.push_back( thr_decoder );

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
			int_reg_count, fp_reg_count ) );
		int_register_stacks.push_back( new VanadisRegisterStack( int_reg_count ) );
		fp_register_stacks.push_back( new VanadisRegisterStack( fp_reg_count ));

		output->verbose(CALL_INFO, 8, 0, "Reorder buffer set to %" PRIu32 " entries, these are shared by all threads.\n",
			rob_count);
		rob.push_back( new VanadisCircularQueue<VanadisInstruction*>( rob_count ) );
		// WE NEED ISA INTEGER AND FP COUNTS HERE NOT ZEROS
		issue_isa_tables.push_back( new VanadisISATable( thread_decoders[i]->getDecoderOptions(),
			thread_decoders[i]->countISAIntReg(), thread_decoders[i]->countISAFPReg() ) );

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

	memDataInterface = loadUserSubComponent<Interfaces::SimpleMem>("mem_interface_data", ComponentInfo::SHARE_NONE, cpuClockTC, new SimpleMem::Handler<SST::Vanadis::VanadisComponent>(this, &VanadisComponent::handleIncomingDataCacheEvent ));
	memInstInterface = loadUserSubComponent<Interfaces::SimpleMem>("mem_interface_inst", ComponentInfo::SHARE_NONE, cpuClockTC, new SimpleMem::Handler<SST::Vanadis::VanadisComponent>(this, &VanadisComponent::handleIncomingInstCacheEvent));

    	// Load anonymously if not found in config
/*
    	if (!memInterface) {
        	std::string memIFace = params.find<std::string>("meminterface", "memHierarchy.memInterface");
        	output.verbose(CALL_INFO, 1, 0, "Loading memory interface: %s ...\n", memIFace.c_str());
        	Params interfaceParams = params.find_prefix_params("meminterface.");
        	interfaceParams.insert("port", "dcache_link");

        	memInterface = loadAnonymousSubComponent<Interfaces::SimpleMem>(memIFace, "memory", 0, ComponentInfo::SHARE_PORTS | ComponentInfo::INSERT_STATS,
		interfaceParams, cpuClockTC, new SimpleMem::Handler<JunoCPU>(this, &JunoCPU::handleEvent));

        	if( NULL == mem )
            		output.fatal(CALL_INFO, -1, "Error: unable to load %s memory interface.\n", memIFace.c_str());
    	}
*/

	if( nullptr == memDataInterface ) {
		output->fatal(CALL_INFO, -1, "Error: unable to load memory interface subcomponent for data cache.\n");
	}

	if( nullptr == memInstInterface ) {
		output->fatal(CALL_INFO, -1, "Error: unable ot load memory interface subcomponent for instruction cache.\n");
	}

    	output->verbose(CALL_INFO, 1, 0, "Successfully loaded memory interface.\n");

	for( uint32_t i = 0; i < thread_decoders.size(); ++i ) {
		output->verbose(CALL_INFO, 8, 0, "Configuring thread instruction cache interface (thread %" PRIu32 ")\n", i);
		thread_decoders[i]->getInstructionLoader()->setMemoryInterface( memInstInterface );
	}

	if( 0 == core_id ) {
		halted_masks[0] = false;

		if( 0 == thread_decoders[0]->getInstructionPointer() ) {
			// This wasn't provided, or its explicitly set to zero which means
			// we should auto-calculate it
			const uint64_t c0_entry = (binary_elf_info != nullptr) ? binary_elf_info->getEntryPoint() : 0;
			output->verbose(CALL_INFO, 8, 0, "Configuring core-0, thread-0 entry point = %p\n",
				(void*) c0_entry);
			thread_decoders[0]->setInstructionPointer( c0_entry );

			output->verbose(CALL_INFO, 8, 0, "Configuring core-0, thread-0 application info...\n");
			thread_decoders[0]->configureApplicationLaunch( output, issue_isa_tables[0], register_files[0], memDataInterface);

			// Force retire table to sync with issue table
			retire_isa_tables[0]->reset( issue_isa_tables[0] );
		} else {
			output->verbose(CALL_INFO, 8, 0, "Entry point for core-0, thread-0 is set by configuration or decoder to: %p\n",
				(void*) thread_decoders[0]->getInstructionPointer());
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

	size_t lsq_store_size    = params.find<size_t>("lsq_store_entries", 8);
	size_t lsq_store_pending = params.find<size_t>("lsq_issued_stores_inflight", 8);
	size_t lsq_load_size     = params.find<size_t>("lsq_load_entries", 8);
	size_t lsq_load_pending  = params.find<size_t>("lsq_issused_loads_inflight", 8);
	size_t lsq_max_loads_per_cycle = params.find<size_t>("max_loads_per_cycle", 2);
	size_t lsq_max_stores_per_cycle = params.find<size_t>("max_stores_per_cycle", 2);

	lsq = new VanadisLoadStoreQueue( memDataInterface, lsq_store_size, lsq_store_pending,
		lsq_load_size, lsq_load_pending, lsq_max_loads_per_cycle,
		lsq_max_stores_per_cycle, &register_files);

	const uint64_t max_addr_mask = binary_elf_info->isELF32() ? 0xFFFFFFFF : UINT64_MAX;
	output->verbose(CALL_INFO, 8, 0, "Setting maximum address mask for the LSQ to: 0x%0llx (derived from ELF-32? %s)\n",
		max_addr_mask, binary_elf_info->isELF32() ? "yes" : "no");

	// if we are doing a 32-bit run, then limit the address ranges which can be reached
	// otherwise leave the mask fully open.
	lsq->setMaxAddressMask( max_addr_mask );

	registerAsPrimaryComponent();
    	primaryComponentDoNotEndSim();
}

VanadisComponent::~VanadisComponent() {
	delete[] instPrintBuffer;
	delete lsq;
}

bool VanadisComponent::tick(SST::Cycle_t cycle) {
	if( current_cycle >= max_cycle ) {
		output->verbose(CALL_INFO, 1, 0, "Reached maximum cycle %" PRIu64 ". Core stops processing.\n", current_cycle );
		primaryComponentOKToEndSim();
		return true;
	}

	bool should_process = false;
	for( uint32_t i = 0; i < hw_threads; ++i ) {
		should_process = should_process | halted_masks[i];
	}

	output->verbose(CALL_INFO, 2, 0, "============================ Cycle %12" PRIu64 " ============================\n", current_cycle );

	output->verbose(CALL_INFO, 8, 0, "-- Core Status:\n");

	for( uint32_t i = 0; i < hw_threads; ++i) {
		output->verbose(CALL_INFO, 8, 0, "---> Thr: %5" PRIu32 " (%s) / ROB-Pend: %" PRIu16 " / IntReg-Free: %" PRIu16 " / FPReg-Free: %" PRIu16 "\n",
			i, halted_masks[i] ? "halted" : "unhalted",
			(uint16_t) rob[i]->size(),
			(uint16_t) int_register_stacks[i]->unused(), (uint16_t) fp_register_stacks[i]->unused() );
	}

	// Fetch
	output->verbose(CALL_INFO, 8, 0, "-- Fetch Stage --------------------------------------------------------------\n");

	// Decode
	output->verbose(CALL_INFO, 8, 0, "-- Decode Stage -------------------------------------------------------------\n");
	for( uint32_t i = 0 ; i < hw_threads; ++i ) {
		// If thread is not masked then decode from it
		if( ! halted_masks[i] ) {
			thread_decoders[i]->tick(output, (uint64_t) cycle);
		}

		output->verbose(CALL_INFO, 16, 0, "---> Decode [hw: %5" PRIu32 "] queue-size: %" PRIu32 "\n", i,
			(uint32_t) thread_decoders[i]->getDecodedQueue()->size());
	}

	// Issue
	output->verbose(CALL_INFO, 8, 0, "-- Issue Stage --------------------------------------------------------------\n");
	for( uint32_t i = 0 ; i < hw_threads; ++i ) {
		tmp_raw_int.clear();
		tmp_raw_fp.clear();

		// If thread is not masked then pull a pending instruction and issue
		if( ! halted_masks[i] ) {
			output->verbose(CALL_INFO, 8, 0, "--> Performing issue for thread %" PRIu32 " (decoded pending queue depth: %" PRIu32 ")...\n",
				i, (uint32_t) thread_decoders[i]->getDecodedQueue()->size());

			for( size_t j = 0; j < thread_decoders[i]->getDecodedQueue()->size(); ++j ) {
				if( rob[i]->full() ) {
					break;
				} else {
					VanadisInstruction* ins = thread_decoders[i]->getDecodedQueue()->peekAt(j);

					if( nullptr == ins ) {
						output->fatal(CALL_INFO, -1, "Error: peekAt(%" PRIu32 ") produced a null instruction.\n", (uint32_t) j);
					}

					output->verbose(CALL_INFO, 16, 0, "---> ROB check for ins: %" PRIu64 " / 0x%0llx / %s\n",
						ins->getID(),
						ins->getInstructionAddress(),
						ins->getInstCode());

					if( ! ins->hasROBSlotIssued() ) {
						output->verbose(CALL_INFO, 16, 0, "---> Inst: %" PRIu64 " (queue-slot: %" PRIu32 "), allocating to ROB (ROB-size: %" PRIu32 " / %" PRIu32 ")\n",
							ins->getID(), (uint32_t) j,
							(uint32_t) rob[i]->size(), (uint32_t) rob[i]->capacity() );
						ins->markROBSlotIssued();
						rob[i]->push( ins );
						output->verbose(CALL_INFO, 16, 0, "---> Allocation to ROB completed for instruction\n");
					} else {
						output->verbose(CALL_INFO, 16, 0, "---> Inst: %" PRIu64 " (queue-slot: %" PRIu32 "), already allocated in the ROB.\n",
							ins->getID(), (uint32_t) j);
					}
				}
			}

			output->verbose(CALL_INFO, 8, 0, "--> Allocation of entries into ROB is completed for this cycle.\n");
			output->verbose(CALL_INFO, 8, 0, "--> Being functional unit allocation...\n");

			if( ! thread_decoders[i]->getDecodedQueue()->empty() ) {
				VanadisInstruction* ins = thread_decoders[i]->getDecodedQueue()->peek();

				ins->printToBuffer(instPrintBuffer, 1024);
				output->verbose(CALL_INFO, 8, 0, "--> Attempting issue for: %s / %p\n", instPrintBuffer,
						(void*) ins->getInstructionAddress());

				/*
				if( rob[i]->full() ) {
					output->verbose(CALL_INFO, 8, 0, "--> ROB for the executing thread is full, cannot issue.\n");
				} else {
					output->verbose(CALL_INFO, 8, 0, "--> ROB for executing thread has %" PRIu32 " out of %" PRIu32 " entries used.\n",
						(uint32_t) rob[i]->size(), (uint32_t) rob[i]->capacity());
				}
				*/

				if( ins->hasROBSlotIssued() ) {
					output->verbose(CALL_INFO, 16, 0, "---> Current instrn has an ROB slot allocated, processing for issue can continue...\n");
				} else {
					output->verbose(CALL_INFO, 16, 0, "---> Current instrn does not have an ROB slot allocated, cannot issue this, stall this cycle.\n");
					continue;
				}

				const int resource_check = checkInstructionResources( ins,
					int_register_stacks[i], fp_register_stacks[i],
					issue_isa_tables[i], tmp_raw_int, tmp_raw_fp);

				output->verbose(CALL_INFO, 8, 0, "Instruction resource can be issuable: %s (issue-query result: %d)\n",
					(resource_check == 0) ? "yes" : "no", resource_check);

				bool can_be_issued = (resource_check == 0) && (! rob[i]->full());
				bool allocated_fu = false;

				// Register dependencies are met and ROB has an entry
				if( can_be_issued ) {

					const VanadisFunctionalUnitType ins_type = ins->getInstFuncType();

					switch( ins_type ) {
					case INST_INT_ARITH:
						for( VanadisFunctionalUnit* next_fu : fu_int_arith ) {
							if(next_fu->isInstructionSlotFree()) {
								next_fu->setSlotInstruction( ins );
								allocated_fu = true;
								break;
							}
						}

						break;
					case INST_FP_ARITH:
						for( VanadisFunctionalUnit* next_fu : fu_fp_div ) {
							if(next_fu->isInstructionSlotFree()) {
								next_fu->setSlotInstruction( ins );
								allocated_fu = true;
								break;
							}
						}

						break;
					case INST_LOAD:
						if( ! lsq->loadFull() ) {
							lsq->push( (VanadisLoadInstruction*) ins );
							allocated_fu = true;
						}
						break;
					case INST_STORE:
						if( ! lsq->storeFull() ) {
							lsq->push( (VanadisStoreInstruction*) ins );
							allocated_fu = true;
						}
						break;
					case INST_INT_DIV:
						for( VanadisFunctionalUnit* next_fu : fu_int_div ) {
							if(next_fu->isInstructionSlotFree()) {
								next_fu->setSlotInstruction( ins );
								allocated_fu = true;
								break;
							}
						}

						break;
					case INST_FP_DIV:
						for( VanadisFunctionalUnit* next_fu : fu_fp_div ) {
							if(next_fu->isInstructionSlotFree()) {
								next_fu->setSlotInstruction( ins );
								allocated_fu = true;
								break;
							}
						}

						break;
					case INST_BRANCH:
						for( VanadisFunctionalUnit* next_fu : fu_branch ) {
                                                        if(next_fu->isInstructionSlotFree() ) {
                                                                next_fu->setSlotInstruction( ins );
                                                                allocated_fu = true;
                                                                break;
                                                        }
                                                }
						break;
					case INST_NOOP:
					case INST_FAULT:
						allocated_fu = true;
						ins->markExecuted();
						break;
					default:
						// ERROR UNALLOCATED
						output->fatal(CALL_INFO, -1, "Error - no processing for instruction class.\n");
						break;
					}

					if( allocated_fu ) {
						const int status = assignRegistersToInstruction(
							thread_decoders[i]->countISAIntReg(),
							thread_decoders[i]->countISAFPReg(),
							ins,
							int_register_stacks[i],
							fp_register_stacks[i],
							issue_isa_tables[i]);

						ins->printToBuffer(instPrintBuffer, 1024);
						output->verbose(CALL_INFO, 8, 0, "--> Issued for: %s / %p\n", instPrintBuffer,
							(void*) ins->getInstructionAddress());

						thread_decoders[i]->getDecodedQueue()->pop();
						ins->markIssued();
						output->verbose(CALL_INFO, 8, 0, "Issued to functional unit, status=%d\n", status);
					}
				}
			}
		}

		issue_isa_tables[i]->print(output, register_files[i], print_int_reg, print_fp_reg);
	}

	// Functional Units / Execute
	output->verbose(CALL_INFO, 8, 0, "-- Execute Stage ------------------------------------------------------------\n");

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

	// LSQ Processing
	lsq->tick( (uint64_t) cycle, output );

	// Retirement
	output->verbose(CALL_INFO, 8, 0, "-- Retire Stage -------------------------------------------------------------\n");

	for( uint32_t i = 0; i < hw_threads; ++i ) {
		output->verbose(CALL_INFO, 8, 0, "Executing retire for thread %" PRIu32 ", rob-entries: %" PRIu64 "\n", i,
			(uint64_t) rob[i]->size());

		if( ! rob[i]->empty() ) {
			VanadisInstruction* rob_front = rob[i]->peek();
			bool perform_pipeline_clear = false;
			uint64_t pipeline_clear_set_ip = 0;

			for( int j = std::min(3, (int) rob[i]->size() - 1); j >= 0; --j ) {
				output->verbose(CALL_INFO, 8, 0, "----> ROB[%2d]: ins: 0x%0llx / %s / error: %s / issued: %s / spec: %s / exe: %s\n",
					j,
					rob[i]->peekAt(j)->getInstructionAddress(),
					rob[i]->peekAt(j)->getInstCode(),
					rob[i]->peekAt(j)->trapsError() ? "yes" : "no",
					rob[i]->peekAt(j)->completedIssue() ? "yes" : "no",
					rob[i]->peekAt(j)->isSpeculated() ? "yes" : "no",
					rob[i]->peekAt(j)->completedExecution() ? "yes" : "no");
			}

			// Instruction is flagging error, print out and halt
			if( rob_front->trapsError() ) {
				output->fatal( CALL_INFO, -1, "Instruction %" PRIu64 " at 0x%llx flags an error (instruction-type=%s)\n",
					rob_front->getID(),
					rob_front->getInstructionAddress(),
					rob_front->getInstCode() );
			}

			// Check we have actually issued the instruction, otherwise this is stuck at the issue stage waiting for
			// resources and has been given an ROB slot to maintain ordering.
			if( ! rob_front->completedIssue() ) {
				output->verbose( CALL_INFO, 8, 0, "ROB -> front instruction (id=%" PRIu64 ") has been allocated, but not issued yet, stall this cycle.\n",
					rob_front->getID());
				continue;
			}

			if( rob_front->isSpeculated() && rob_front->completedExecution() ) {
				// Check we predicted in the right direction.
				output->verbose(CALL_INFO, 8, 0, "ROB -> front on thread %" PRIu32 " is a speculated instruction.\n", i);

				VanadisSpeculatedInstruction* spec_ins = dynamic_cast<VanadisSpeculatedInstruction*>( rob_front );

				output->verbose(CALL_INFO, 8, 0, "ROB -> check prediction: speculated: %s / result: %s\n",
					directionToChar( spec_ins->getSpeculatedDirection() ),
					directionToChar( spec_ins->getResultDirection( register_files[i] ) ) );

				bool delaySlotsAreOK = true;

				switch( spec_ins->getDelaySlotType() ) {
				case VANADIS_SINGLE_DELAY_SLOT:
					output->verbose(CALL_INFO, 8, 0, "ROB ---> requires a single delay slot for marking complete.\n");

					// We need to check the instruction just behind this one (a single delay slot, it must have executed)
					if( rob[i]->peekAt(1)->completedExecution() ) {
						output->verbose(CALL_INFO, 8, 0, "ROB ------> delay slot required ans has completed execution.\n");
					} else {
						output->verbose(CALL_INFO, 8, 0, "ROB ------> delay slot required but has not completed execution, must wait.\n");
						delaySlotsAreOK = false;
					}
					break;
				default:
					break;
				}

				// If the delay slot handlers are satisfied we can proceed
				if( delaySlotsAreOK ) {
					if( spec_ins->getSpeculatedDirection() != spec_ins->getResultDirection( register_files[i] ) ) {
						const uint64_t recalculate_ip = spec_ins->calculateAddress( output, register_files[i], spec_ins->getInstructionAddress() );

						// We have a mis-speculated instruction, uh-oh.
						output->verbose(CALL_INFO, 8, 0, "ROB -> [PIPELINE-CLEAR] mis-speculated execution, begin pipeline reset (set ip: 0x%llx)\n",
							recalculate_ip);

						output->verbose(CALL_INFO, 8, 0, "ROB -> Updating branch predictor with new information\n");
						thread_decoders[i]->getBranchPredictor()->push( spec_ins->getInstructionAddress(), recalculate_ip );

                                        	// Actually pop the instruction now we know its safe to do so.
      	                                 	rob[i]->pop();

                                        	output->verbose(CALL_INFO, 16, 0, "----> Retire inst: %" PRIu64 " (addr: 0x%0llx / %s)\n",
                                                	rob_front->getID(),
                                                	rob_front->getInstructionAddress(),
                                                	rob_front->getInstCode() );

						if( VANADIS_SINGLE_DELAY_SLOT == spec_ins->getDelaySlotType() ) {
							// Need to retire the delay slot too
       	        	                         	recoverRetiredRegisters( rob[i]->peek(),
        	                                        int_register_stacks[rob_front->getHWThread()],
                        	                        fp_register_stacks[rob_front->getHWThread()],
                                	                issue_isa_tables[i], retire_isa_tables[i] );

							VanadisInstruction* delay_ins = rob[i]->pop();
							delete delay_ins;
						}

                                        	recoverRetiredRegisters( rob_front,
                                                int_register_stacks[rob_front->getHWThread()],
                                                fp_register_stacks[rob_front->getHWThread()],
                                                issue_isa_tables[i], retire_isa_tables[i] );

                                        	retire_isa_tables[i]->print(output, print_int_reg, print_fp_reg);

                                        	delete rob_front;

						handleMisspeculate( i, recalculate_ip );

						perform_pipeline_clear = true;
						pipeline_clear_set_ip = recalculate_ip;
					} else {
						const uint64_t recalculate_ip = spec_ins->calculateAddress( output, register_files[i], spec_ins->getInstructionAddress() );

						if( recalculate_ip == spec_ins->getSpeculatedAddress() ) {
							output->verbose(CALL_INFO, 8, 0,  "ROB -> speculation direction and target correct for branch, continue execution.\n");
							output->verbose(CALL_INFO, 16, 0, "ROB -> spec-addr: 0x%0llx / target: 0x%0llx\n",
								recalculate_ip, spec_ins->getSpeculatedAddress());

	                                        	// Actually pop the instruction now we know its safe to do so.
       	                                 		rob[i]->pop();

                                        		output->verbose(CALL_INFO, 16, 0, "----> Retire inst: %" PRIu64 " (addr: 0x%0llx / %s)\n",
                                                		rob_front->getID(),
                                                		rob_front->getInstructionAddress(),
                                                		rob_front->getInstCode() );

							if( VANADIS_SINGLE_DELAY_SLOT == spec_ins->getDelaySlotType() ) {
								// Need to retire the delay slot too
       	        	        	                 	recoverRetiredRegisters( rob[i]->peek(),
        	       		                                int_register_stacks[rob_front->getHWThread()],
                	        	                        fp_register_stacks[rob_front->getHWThread()],
       		                         	                issue_isa_tables[i], retire_isa_tables[i] );

								VanadisInstruction* delay_ins = rob[i]->pop();
								delete delay_ins;
							}

                                        		recoverRetiredRegisters( rob_front,
                                                	int_register_stacks[rob_front->getHWThread()],
                                                	fp_register_stacks[rob_front->getHWThread()],
                                                	issue_isa_tables[i], retire_isa_tables[i] );

                                        		retire_isa_tables[i]->print(output, print_int_reg, print_fp_reg);

                                        		delete rob_front;
						} else {
							output->verbose(CALL_INFO, 8, 0, "ROB -> [PIPELINE-CLEAR] correctly speculated direction, but target was incorrect.\n");
							output->verbose(CALL_INFO, 8, 0, "ROB -> [PIPELINE-CLEAR] pred: 0x%0llx executed: 0x%0llx\n",
								recalculate_ip, spec_ins->getSpeculatedAddress());

							perform_pipeline_clear = true;
							pipeline_clear_set_ip = recalculate_ip;

							output->verbose(CALL_INFO, 8, 0, "ROB -> Updating branch predictor with new information\n");
							thread_decoders[i]->getBranchPredictor()->push( spec_ins->getInstructionAddress(), recalculate_ip );

	                                        	// Actually pop the instruction now we know its safe to do so.
       	                                 		rob[i]->pop();

                                        		output->verbose(CALL_INFO, 16, 0, "----> Retire inst: %" PRIu64 " (addr: 0x%0llx / %s)\n",
                                                		rob_front->getID(),
                                                		rob_front->getInstructionAddress(),
                                                		rob_front->getInstCode() );

							if( VANADIS_SINGLE_DELAY_SLOT == spec_ins->getDelaySlotType() ) {
								// Need to retire the delay slot too
       	        	        	                 	recoverRetiredRegisters( rob[i]->peek(),
        	       		                                int_register_stacks[rob_front->getHWThread()],
                	        	                        fp_register_stacks[rob_front->getHWThread()],
       		                         	                issue_isa_tables[i], retire_isa_tables[i] );

								VanadisInstruction* delay_ins = rob[i]->pop();
								delete delay_ins;
							}

                                        		recoverRetiredRegisters( rob_front,
                                                	int_register_stacks[rob_front->getHWThread()],
                                                	fp_register_stacks[rob_front->getHWThread()],
                                                	issue_isa_tables[i], retire_isa_tables[i] );

                                        		retire_isa_tables[i]->print(output, print_int_reg, print_fp_reg);

                                        		delete rob_front;

							handleMisspeculate( i, recalculate_ip );
						}
					}
				}
			} else if( rob_front->completedExecution() ) {
				output->verbose(CALL_INFO, 8, 0, "ROB for Thread %5" PRIu32 " contains entries and those have finished executing, in retire status...\n", i);
				bool perform_execute_clear_up = true;

				if( INST_FENCE == rob_front->getInstFuncType() ) {
					VanadisFenceInstruction* fence_ins = dynamic_cast<VanadisFenceInstruction*>( rob_front );
					output->verbose(CALL_INFO, 8, 0, "ROB -> front entry performs fence load-fence: %s / store-fence: %s\n",
						fence_ins->createsLoadFence() ? "yes" : "no",
						fence_ins->createsStoreFence() ? "yes" : "no" );

					perform_execute_clear_up = fence_ins->createsLoadFence()  ? (lsq->loadSize() == 0)  : true;
					perform_execute_clear_up = fence_ins->createsStoreFence() ? (lsq->storeSize() == 0) : perform_execute_clear_up;

					output->verbose(CALL_INFO, 8, 0, "ROB ---> evaluation of LSQ determines that fence retire can %s\n",
						perform_execute_clear_up ? "proceed" : "*not* proceed" );

					if( perform_execute_clear_up ) {
						// Notify the decoder we can accept load/stores again
					}
				}

				if( perform_execute_clear_up ) {
					// Actually pop the instruction now we know its safe to do so.
					rob[i]->pop();

					output->verbose(CALL_INFO, 16, 0, "----> Retire inst: %" PRIu64 " (addr: 0x%0llx / %s)\n",
						rob_front->getID(),
						rob_front->getInstructionAddress(),
						rob_front->getInstCode() );

					recoverRetiredRegisters( rob_front,
						int_register_stacks[rob_front->getHWThread()],
						fp_register_stacks[rob_front->getHWThread()],
						issue_isa_tables[i], retire_isa_tables[i] );

					retire_isa_tables[i]->print(output, print_int_reg, print_fp_reg);

					delete rob_front;
				}
			} else {
				output->verbose(CALL_INFO, 16, 0, "----> [ROB] - marking front instruction as ROB-Front\n");
				// make sure instruction is marked at front of ROB since this can
				// enable instructions which need to be retire-ready to process
				rob_front->markFrontOfROB();
			}
		}
	}

	output->verbose(CALL_INFO, 2, 0, "================================ End of Cycle ==============================\n" );

	current_cycle++;

	if( current_cycle >= max_cycle ) {
		output->verbose(CALL_INFO, 1, 0, "Reached maximum cycle %" PRIu64 ". Core stops processing.\n", current_cycle );
		primaryComponentOKToEndSim();
		return true;
	} else{
		return false;
	}
}

int VanadisComponent::checkInstructionResources(
	VanadisInstruction* ins,
    VanadisRegisterStack* int_regs,
    VanadisRegisterStack* fp_regs,
    VanadisISATable* isa_table,
	std::set<uint16_t>& isa_int_regs_written_ahead,
	std::set<uint16_t>& isa_fp_regs_written_ahead) {

	bool resources_good = true;

	// We need places to store our output registers
	resources_good &= (int_regs->unused() >= ins->countISAIntRegOut());
	resources_good &= (fp_regs->unused() >= ins->countISAFPRegOut());

	// If there are any pending writes against our reads, we can't issue until
	// they are done
	for( uint16_t i = 0; i < ins->countISAIntRegIn(); ++i ) {
		const uint16_t ins_isa_reg = ins->getISAIntRegIn(i);
		resources_good &= (!isa_table->pendingIntWrites(ins_isa_reg));

		// Check there are no RAW in the pending instruction queue
		resources_good &= (isa_int_regs_written_ahead.find(ins_isa_reg) == isa_int_regs_written_ahead.end());
	}
	
	output->verbose(CALL_INFO, 16, 0, "--> Check input integer registers, issue-status: %s\n",
		(resources_good ? "yes" : "no") );

	if( resources_good ) {
		for( uint16_t i = 0; i < ins->countISAFPRegIn(); ++i ) {
			const uint16_t ins_isa_reg = ins->getISAFPRegIn(i);
			resources_good &= (!isa_table->pendingFPWrites(ins_isa_reg));

			// Check there are no RAW in the pending instruction queue
			resources_good &= (isa_fp_regs_written_ahead.find(ins_isa_reg) == isa_fp_regs_written_ahead.end());
		}
	
		output->verbose(CALL_INFO, 16, 0, "--> Check input floating-point registers, issue-status: %s\n",
			(resources_good ? "yes" : "no") );
	}

	// Update RAW table
	for( uint16_t i = 0; i < ins->countISAIntRegOut(); ++i ) {
		const uint16_t ins_isa_reg = ins->getISAIntRegOut(i);
		isa_int_regs_written_ahead.insert(ins_isa_reg);
	}

	for( uint16_t i = 0; i < ins->countISAFPRegOut(); ++i ) {
		const uint16_t ins_isa_reg = ins->getISAIntRegOut(i);
		isa_fp_regs_written_ahead.insert(ins_isa_reg);
	}

	return (resources_good) ? 0 : 1;
}

int VanadisComponent::assignRegistersToInstruction(
	const uint16_t int_reg_count,
	const uint16_t fp_reg_count,
        VanadisInstruction* ins,
        VanadisRegisterStack* int_regs,
        VanadisRegisterStack* fp_regs,
        VanadisISATable* isa_table) {

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

		uint16_t out_reg;

		if( reg_is_also_in ) {
			out_reg = isa_table->getIntPhysReg( ins_isa_reg );
		} else {
			out_reg = int_regs->pop();
			isa_table->setIntPhysReg( ins_isa_reg, out_reg );
		}

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
		bool reg_is_also_in = false;

		for( uint16_t j = 0; j < ins->countISAFPRegIn(); ++j ) {
			if( ins_isa_reg == ins->getISAFPRegIn(j) ) {
				reg_is_also_in = true;
				break;
			}
		}

		uint16_t out_reg;

		if( reg_is_also_in ) {
			out_reg = isa_table->getFPPhysReg( ins_isa_reg );
		} else {
			out_reg = fp_regs->pop();
			isa_table->setFPPhysReg( ins_isa_reg, out_reg );
		}

		ins->setPhysFPRegOut(i, out_reg);
		isa_table->incFPWrite( ins_isa_reg );
	}

	return 0;
}

int VanadisComponent::recoverRetiredRegisters(
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

	for( uint16_t i = 0; i < ins->countISAIntRegOut(); ++i ) {
		const uint16_t isa_reg = ins->getISAIntRegOut(i);
   		const uint16_t cur_phys_reg = retire_isa_table->getIntPhysReg(isa_reg);

		issue_isa_table->decIntWrite( isa_reg );

		// Check this register isn't also in our input set because then we
		// wouldn't have allocated a new register for it
		bool reg_also_input = false;

		for( uint16_t j = 0; j < ins->countISAIntRegIn(); ++j ) {
			if( isa_reg == ins->getISAIntRegIn(j) ) {
				reg_also_input = true;
			}
		}

		if( ! reg_also_input ) {
			recovered_phys_reg_int.push_back( cur_phys_reg );

			// Set the ISA register in the retirement table to point
			// to the physical register used by this instruction
			retire_isa_table->setIntPhysReg( isa_reg, ins->getPhysIntRegOut(i) );
		}
	}

	for( uint16_t i = 0; i < ins->countISAFPRegOut(); ++i ) {
		const uint16_t isa_reg = ins->getISAFPRegOut(i);
		const uint16_t cur_phys_reg = retire_isa_table->getFPPhysReg(isa_reg);

		issue_isa_table->decFPWrite( isa_reg );

		// Check this register isn't also in our input set because then we
		// wouldn't have allocated a new register for it
		bool reg_also_input = false;

		for( uint16_t j = 0; j < ins->countISAIntRegIn(); ++j ) {
			if( isa_reg == ins->getISAIntRegIn(j) ) {
				reg_also_input = true;
			}
		}

		if( ! reg_also_input ) {
			recovered_phys_reg_fp.push_back( cur_phys_reg );

			// Set the ISA register in the retirement table to point
			// to the physical register used by this instruction
			retire_isa_table->setFPPhysReg( isa_reg, ins->getPhysFPRegOut(i) );
		}
	}

	output->verbose(CALL_INFO, 16, 0, "Recovering: %d int-reg and %d fp-reg\n",
		(int) recovered_phys_reg_int.size(), (int) recovered_phys_reg_fp.size());

	for( uint16_t next_reg : recovered_phys_reg_int ) {
		int_regs->push(next_reg);
	}

	for( uint16_t next_reg : recovered_phys_reg_fp ) {
		fp_regs->push(next_reg);
	}

	return 0;
}

void VanadisComponent::setup() {

}

void VanadisComponent::finish() {

}

void VanadisComponent::printStatus( SST::Output& output ) {

}

void VanadisComponent::init(unsigned int phase) {
	output->verbose(CALL_INFO, 2, 0, "Start: init-phase: %" PRIu32 "...\n", (uint32_t) phase );
	output->verbose(CALL_INFO, 2, 0, "-> Initializing memory interfaces with this phase...\n");

	memDataInterface->init( phase );
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

  				for( size_t i = 0; i < binary_elf_info->countProgramHeaders(); ++i ) {
					const VanadisELFProgramHeaderEntry* next_prog_hdr = binary_elf_info->getProgramHeader(i);

					if( (PROG_HEADER_LOAD == next_prog_hdr->getHeaderType()) ) {
						output->verbose(CALL_INFO, 2, 0, ">> Loading Program Header from executable at %p, len=%" PRIu64 "...\n",
							(void*) next_prog_hdr->getImageOffset(), next_prog_hdr->getHeaderImageLength());
						output->verbose(CALL_INFO, 2, 0, ">>>> Placing at virtual address: %p\n",
							(void*) next_prog_hdr->getVirtualMemoryStart());

						// Do we have enough space in the memory image, if not, extend and zero
						if( initial_mem_contents.size() < ( next_prog_hdr->getVirtualMemoryStart() + next_prog_hdr->getHeaderImageLength() )) {
							initial_mem_contents.resize( ( next_prog_hdr->getVirtualMemoryStart() + next_prog_hdr->getHeaderImageLength() ), 0);
						}

						fseek( exec_file, next_prog_hdr->getImageOffset(), SEEK_SET );
						fread( &initial_mem_contents[ next_prog_hdr->getVirtualMemoryStart() ],
							next_prog_hdr->getHeaderImageLength(), 1, exec_file);
					}
				}

				for( size_t i = 0; i < binary_elf_info->countProgramSections(); ++i ) {
					const VanadisELFProgramSectionEntry* next_sec = binary_elf_info->getProgramSection( i );

					if( (SECTION_HEADER_PROG_DATA == next_sec->getSectionType()) ||
						(SECTION_HEADER_BSS == next_sec->getSectionType()) ) {

						output->verbose(CALL_INFO, 2, 0, ">> Loading Section from executable at: %p, len=%" PRIu64 "...\n",
							(void*) next_sec->getVirtualMemoryStart(), next_sec->getImageLength());

						// Executable data, let's load it in
						if( initial_mem_contents.size() < (next_sec->getVirtualMemoryStart() + next_sec->getImageLength()) ) {
							size_t size_now = initial_mem_contents.size();
							initial_mem_contents.resize( next_sec->getVirtualMemoryStart() + next_sec->getImageLength(), 0 );
						}

						// Find the section and read it all in
						fseek( exec_file, next_sec->getImageOffset(), SEEK_SET );
						fread( &initial_mem_contents[next_sec->getVirtualMemoryStart()],
							next_sec->getImageLength(), 1, exec_file);
					}
				}

				fclose(exec_file);

				output->verbose(CALL_INFO, 2, 0, ">> Writing memory contents (%" PRIu64 " bytes at index 0)\n",
					(uint64_t) initial_mem_contents.size());

/*
				for( size_t i = 0x400580; i < (0x400580 + 0x8); i++ ) {
					printf("i=%p = %x\n", (void*) i, initial_mem_contents[i]);
				}
*/
				SimpleMem::Request* writeExe = new SimpleMem::Request(SimpleMem::Request::Write,
					0, initial_mem_contents.size(), initial_mem_contents);
				memDataInterface->sendInitData( writeExe );
			} else {
				output->verbose(CALL_INFO, 2, 0, "Not core-0, so will not perform any loading of binary info.\n");
			}
		} else {
			output->verbose(CALL_INFO, 2, 0, "No ELF binary information loaded, will not perform any loading.\n");
		}
	}

	output->verbose(CALL_INFO, 2, 0, "End: init-phase: %" PRIu32 "...\n", (uint32_t) phase );
}

void VanadisComponent::handleIncomingDataCacheEvent( SimpleMem::Request* ev ) {
	output->verbose(CALL_INFO, 16, 0, "-> Incoming d-cache event (addr=%p)...\n",
		(void*) ev->addr);
	lsq->processIncomingDataCacheEvent( output, ev );
}

void VanadisComponent::handleIncomingInstCacheEvent( SimpleMem::Request* ev ) {
	output->verbose(CALL_INFO, 16, 0, "-> Incoming i-cache event (addr=%p)...\n",
		(void*) ev->addr);

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

void VanadisComponent::handleMisspeculate( const uint32_t hw_thr, const uint64_t new_ip ) {
	output->verbose(CALL_INFO, 16, 0, "-> Handle mis-speculation on %" PRIu32 " (new-ip: 0x%llx)...\n", hw_thr, new_ip);

	clearFuncUnit( hw_thr, fu_int_arith );
	clearFuncUnit( hw_thr, fu_int_div );
	clearFuncUnit( hw_thr, fu_fp_arith );
	clearFuncUnit( hw_thr, fu_fp_div );
	clearFuncUnit( hw_thr, fu_branch );

	lsq->clearLSQByThreadID( output, hw_thr );
	resetRegisterStacks( hw_thr );
	clearROBMisspeculate(hw_thr);

	// Reset the ISA table to get correct ISA to physical mappings
	issue_isa_tables[hw_thr]->reset( retire_isa_tables[hw_thr] );

	// Notify the decoder we need a clear and reset to new instruction pointer
	thread_decoders[hw_thr]->setInstructionPointerAfterMisspeculate( output, new_ip );

	output->verbose(CALL_INFO, 16, 0, "-> Mis-speculate repair finished.\n");
}

void VanadisComponent::clearFuncUnit( const uint32_t hw_thr, std::vector<VanadisFunctionalUnit*>& unit ) {
	for( VanadisFunctionalUnit* next_fu : unit ) {
		next_fu->clearByHWThreadID( output, hw_thr );
	}
}

void VanadisComponent::resetRegisterStacks( const uint32_t hw_thr ) {
	output->verbose(CALL_INFO, 16, 0, "-> Resetting register stacks on thread %" PRIu32 "...\n", hw_thr);
	output->verbose(CALL_INFO, 16, 0, "---> Reclaiming integer registers...\n");

	const uint16_t int_reg_count = int_register_stacks[hw_thr]->capacity();
	output->verbose(CALL_INFO, 16, 0, "---> Creating a new int register stack with %" PRIu16 " registers...\n", int_reg_count);
	VanadisRegisterStack* thr_int_stack = int_register_stacks[hw_thr];
	thr_int_stack->clear();

	for( uint16_t i = 0; i < int_reg_count; ++i ) {
		if( ! retire_isa_tables[ hw_thr ]->physIntRegInUse( i ) ) {
			thr_int_stack->push(i);
		}
	}

//	delete int_register_stacks[hw_thr];
//	int_register_stacks[hw_thr] = new_int_stack;

	output->verbose(CALL_INFO, 16, 0, "---> Integer register stack contains %" PRIu32 " registers.\n",
		(uint32_t) thr_int_stack->size());
	output->verbose(CALL_INFO, 16, 0, "---> Reclaiming floating point registers...\n");

	const uint16_t fp_reg_count = fp_register_stacks[hw_thr]->capacity();
	output->verbose(CALL_INFO, 16, 0, "---> Creating a new fp register stack with %" PRIu16 " registers...\n", fp_reg_count);
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

	output->verbose(CALL_INFO, 16, 0, "---> Floating point stack contains %" PRIu32 " registers.\n",
		(uint32_t) thr_fp_stack->size());
}

void VanadisComponent::clearROBMisspeculate( const uint32_t hw_thr ) {
	VanadisCircularQueue<VanadisInstruction*>* rob_tmp = new
		VanadisCircularQueue<VanadisInstruction*>( rob[hw_thr]->capacity() );

	// Delete the old ROB since this is not clear and reset to an empty one
	delete rob[hw_thr];
	rob[hw_thr] = rob_tmp;
}