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

#ifndef _H_VANADIS_STD_LOAD_STORE_Q
#define _H_VANADIS_STD_LOAD_STORE_Q

#include "sst/core/interfaces/simpleMem.h"

#include "inst/vinst.h"
#include "inst/vstore.h"
#include "inst/vload.h"

#include "lsq/vlsq.h"

#include "datastruct/cqueue.h"
#include "util/vsignx.h"

#include <map>
#include <set>
#include <cassert>

using namespace SST::Interfaces;

namespace SST {
namespace Vanadis {

enum VanadisAddressOverlapType {
	OVERLAP_FREE,
	PARTIAL_COVERAGE,
	STORE_COVERS_LOAD
};

enum VanadisLoadIssueEvaluation {
	FORWARD_STORE,
	STALL_PROCESSING,
	REQUIRE_LOAD
};

class VanadisStoreRecord {

public:
	VanadisStoreRecord(VanadisStoreInstruction* genIns) :
		gen_ins(genIns) {}

	bool predates( VanadisInstruction* check ) { assert(0); return true; }
	bool checkIssueToMemory() { return gen_ins->checkFrontOfROB(); }

	VanadisStoreInstruction* getAssociatedInstruction() { return gen_ins; }

protected:
	VanadisStoreInstruction* gen_ins;

};

class VanadisLoadRecord {

public:
	VanadisLoadRecord( VanadisLoadInstruction* genIns ):
		gen_ins(genIns) {}

	VanadisLoadInstruction* getAssociatedInstruction() { return gen_ins; }

protected:
	VanadisLoadInstruction* gen_ins;

};

class VanadisStandardLoadStoreQueue : public VanadisLoadStoreQueue {

public:
	SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(
		VanadisStandardLoadStoreQueue,
		"vanadis",
		"VanadisStandardLoadStoreQueue",
		SST_ELI_ELEMENT_VERSION(1,0,0),
		"Implements a basic laod-store queue for use with the SST memInterface",
		SST::Vanadis::VanadisLoadStoreQueue
	)

	SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS(
		{ "memory_interface",		"Set the interface to memory",		"SST::Interfaces::SimpleMem" 		}
	)

	SST_ELI_DOCUMENT_PORTS(
		{ "dcache_link",		"Connects the LSQ to the data cache",	{}					}
	)

	SST_ELI_DOCUMENT_PARAMS(
		{ "lsq_store_entries",		"Set the number of store entries in the queuing system",		"8"	},
		{ "lsq_load_entries",		"Set the number of load entries in the queuing system",         	"8"	},
		{ "lsq_store_pending",          "Set the maximum number of in-flight stores",			   	"8"	},
		{ "lsq_load_pending",           "Set the maximum number of in-flight loads",				"8"	},
		{ "max_store_issue_per_cycle",	"Set the maximum number of stores that can be issued per cycle",	"2"	},
		{ "max_load_issue_per_cycle",   "Set the maximum number of loads that can be issued per cycle",		"2"	}
	)

	VanadisStandardLoadStoreQueue( ComponentId_t id, Params& params ) :
		VanadisLoadStoreQueue( id, params ), processingLLSC(false) {

		max_mem_issued_stores = params.find<uint32_t>("lsq_store_pending", 8);
		max_mem_issued_loads  = params.find<uint32_t>("lsq_load_pending", 8);

		max_queued_stores     = params.find<uint32_t>("lsq_store_entries", 8);
		max_queued_loads      = params.find<uint32_t>("lsq_load_entries", 8);

		max_stores_issue_per_cycle = params.find<uint32_t>("max_store_issue_per_cycle", 2);
		max_load_issue_per_cycle   = params.find<uint32_t>("max_load_issue_per_cycle", 2);

		memInterface = loadUserSubComponent<Interfaces::SimpleMem>("memory_interface", ComponentInfo::SHARE_PORTS |
			ComponentInfo::INSERT_STATS, getTimeConverter("1ps"),
			new SimpleMem::Handler<SST::Vanadis::VanadisStandardLoadStoreQueue>(this,
			&VanadisStandardLoadStoreQueue::processIncomingDataCacheEvent));

		store_q = new VanadisCircularQueue<VanadisStoreRecord*>(max_mem_issued_stores);

		output->verbose(CALL_INFO, 2, 0, "LSQ Store Queue Length:               %" PRIu32 "\n", max_mem_issued_stores );
		output->verbose(CALL_INFO, 2, 0, "LSQ Load Queue Length:                %" PRIu32 "\n", max_mem_issued_loads );
		output->verbose(CALL_INFO, 2, 0, "LSQ Max Stores in Flight:             %" PRIu32 "\n", max_queued_stores );
		output->verbose(CALL_INFO, 2, 0, "LSQ Max Loads in Flight:              %" PRIu32 "\n", max_queued_loads );
		output->verbose(CALL_INFO, 2, 0, "LSQ Max Store issue/cycle:            %" PRIu32 "\n", max_stores_issue_per_cycle );
		output->verbose(CALL_INFO, 2, 0, "LSQ Max Loads issue/cycle:            %" PRIu32 "\n", max_load_issue_per_cycle );
	}

	~VanadisStandardLoadStoreQueue() {
		delete store_q;
		delete memInterface;
	}

	virtual bool storeFull()  	 { return store_q->full(); }
	virtual bool loadFull()          { return (pending_queued_loads  >= max_queued_loads);  }

	virtual size_t storeSize()       { return store_q->size(); }
	virtual size_t loadSize()        { return pending_queued_loads;  }

	virtual void push( VanadisStoreInstruction* store_me ) {
		assert( ! (store_q->full()) );

		store_q->push( new VanadisStoreRecord( store_me ) );
	}

	virtual void push( VanadisLoadInstruction* load_me ) {
		load_q.push_back( new VanadisLoadRecord( load_me ) );
		pending_queued_loads++;
	}

	virtual void init( unsigned int phase ) {
		memInterface->init( phase );
	}

	virtual void setInitialMemory( const uint64_t address, std::vector<uint8_t>& payload ) {
		output->verbose(CALL_INFO, 2, 0, "setting initial memory contents for address 0x%llx / size: %" PRIu64 "\n",
			address, (uint64_t) payload.size() );
		memInterface->sendInitData( new SimpleMem::Request( SimpleMem::Request::Write, address,
			payload.size(), payload) );
	}

	VanadisAddressOverlapType evaluateAddressOverlap( const uint64_t loadAddress, const uint16_t loadLen,
		const uint64_t storeAddress, const uint16_t storeLen ) const {

		VanadisAddressOverlapType overlap = OVERLAP_FREE;

		const uint64_t loadEnd  = loadAddress + loadLen;
		const uint64_t storeEnd = storeAddress + storeLen;

		if( loadEnd <= storeEnd ) {
			if( loadAddress >= storeAddress ) {
				overlap = STORE_COVERS_LOAD;
			} else if( loadEnd >= storeAddress ) {
				overlap = PARTIAL_COVERAGE;
			}
		} else {
			if( loadAddress <= storeEnd ) {
				overlap = PARTIAL_COVERAGE;
			}
		}

		return overlap;
	}

	virtual void tick( uint64_t cycle ) {
		output->verbose(CALL_INFO, 16, 0, "-> Ticking Load/Store Queue Processors...\n");
		output->verbose(CALL_INFO, 16, 0, "---> LSQ contains: %" PRIu32 " queued loads / %" PRIu32 " queued stores.\n", pending_queued_loads,
			(uint32_t) store_q->size() );
		char* inst_print_buffer = new char[256];
		for( size_t i = 0;i < store_q->size(); ++i) {
			store_q->peekAt(i)->getAssociatedInstruction()->printToBuffer( inst_print_buffer, 256 );
			output->verbose(CALL_INFO, 16, 0, "-----> store [%5" PRIu32 "]: addr: 0x%0llx (%s)\n", (uint32_t) i,
				store_q->peekAt(i)->getAssociatedInstruction()->getInstructionAddress(), inst_print_buffer);
		}
		delete[] inst_print_buffer;
		output->verbose(CALL_INFO, 16, 0, "---> LSQ contains: %" PRIu32 " loads in flight / %" PRIu32 " stores in flight\n", pending_mem_issued_loads, pending_mem_issued_stores );
		output->verbose(CALL_INFO, 16, 0, "-> Ticking Load Queue Handling...\n");
		tick_loads(cycle);
		output->verbose(CALL_INFO, 16, 0, "---> Ticking Store Queue Handling (max stores per cycle = %" PRIu32 ")\n", max_stores_issue_per_cycle);
		int stores_this_cycle = 0;
		for( uint32_t i = 0; i < max_stores_issue_per_cycle; ++i ) {
			int rc = tick_stores(cycle);

			if( 1 == rc ) {
				output->verbose(CALL_INFO, 16, 0, "---> Store handling returns that store at the front of the queue cannot be issued this cycle.\n");
				break;
			} else if( -1 == rc ) {
				output->verbose(CALL_INFO, 16, 0, "---> Store Queue is empty, no further processing needed this cycle.\n");
				break;
			}

			stores_this_cycle++;
		}
		output->verbose(CALL_INFO, 16, 0, "---> Processed %d stores this cycle.\n", stores_this_cycle);
		output->verbose(CALL_INFO, 16, 0, "-> Load/Store Queue Processing Complete for this cycle\n");
	}

	void tick_loads( const uint64_t cycle ) {
		for( auto next_load = load_q.begin(); next_load != load_q.end(); ) {
			VanadisLoadInstruction* load_ins = (*next_load)->getAssociatedInstruction();

			output->verbose(CALL_INFO, 16, 0, "-> LSQ inspect load record: (ins: 0x%0llx, thr: %" PRIu32 ") executed? %s\n",
				load_ins->getInstructionAddress(), load_ins->getHWThread(),
				(load_ins->completedExecution() ? "yes" : "no"));

			// If instruction has already been issued, we skip it, and will remove it
			// from the queue later
			if( load_ins->completedExecution() ) {
				break;
			}

			uint64_t load_address = 0;
			uint16_t load_width   = 0;

			assert( registerFiles != nullptr );

			load_ins->computeLoadAddress( output, registerFiles->at( load_ins->getHWThread() ),
				&load_address, &load_width );

			output->verbose(CALL_INFO, 16, 0, "-> LSQ attempt process for load at: %p / %" PRIu64 "\n",
				(void*) load_address, load_address );

			// iterate over store queue, do we find any with the same address?
			const size_t store_q_count = store_q->size();

			VanadisLoadIssueEvaluation load_eval = REQUIRE_LOAD;

			// Must check in reverse order, we need the most up to date version of
			// the address if multiple stores hit the same place
			for( int i = store_q_count - 1; i >= 0; i-- ) {
				VanadisStoreRecord* check_store = store_q->peekAt(i);
				VanadisStoreInstruction* check_store_ins = check_store->getAssociatedInstruction();

				// Are load/store in the same HW thread and does the load predate the store
				// we cannot use stores which would have occured *after* we are supposed to
				// be issued
				if( (load_ins->getHWThread() == check_store_ins->getHWThread()) &&
					 check_store->predates( load_ins ) ) {

					uint64_t store_address = 0;
					uint16_t store_width   = 0;

					check_store_ins->computeStoreAddress( output, registerFiles->at( load_ins->getHWThread() ),
						&store_address, &store_width );

					output->verbose(CALL_INFO, 16, 0, "-> LSQ compare load (0x%0llx, width=%" PRIu16 ") to store at (0x%0llx, width=%" PRIu16 ")\n",
						load_address, load_width, store_address, store_width);

					switch( evaluateAddressOverlap(
							load_address,
							load_ins->getLoadWidth(),
							store_address,
							check_store_ins->getStoreWidth()
						) ) {

					case OVERLAP_FREE:
						// Nothing to do here, load must be issued still.
						output->verbose(CALL_INFO, 16, 0, "-> LSQ compare -> require load, overlap-free\n");
						load_eval = REQUIRE_LOAD;
						break;

					case PARTIAL_COVERAGE:
						// This is a stop condition for searching, we need to wait
						// for the store to retire as we can only get *some* of the
						// data we need from the register
						output->verbose(CALL_INFO, 16, 0, "-> LSQ compare -> partial store coverage, requires stall and re-eval when store completed.\n");
						load_eval = STALL_PROCESSING;
						break;

					case STORE_COVERS_LOAD:
						{
							output->verbose(CALL_INFO, 16, 0, "-> LSQ compare -> load can be forwarded from associated store, process load from existing register contents...\n");
							// We can forward result from register back to load
							VanadisRegisterFile* reg_file = registerFiles->at( load_ins->getHWThread() );

							const uint64_t store_value = reg_file->getIntReg<uint64_t>( check_store_ins->getPhysIntRegIn()[1] );

							reg_file->setIntReg(
								load_ins->getPhysIntRegOut()[0],
								store_value
								);

							output->verbose(CALL_INFO, 16, 0, "---> load marked executed, load contents forwarded.\n");
							load_ins->markExecuted();
							load_eval = FORWARD_STORE;
						}
						break;
					default:
						break;
					}

					break;
				}
			}

			// Processing says we really have to go ahead and do this load
			if( load_eval == REQUIRE_LOAD ) {
				if( pending_mem_issued_loads < max_mem_issued_loads ) {
					uint64_t load_addr  = 0;
					uint16_t load_width = 0;

					load_ins->computeLoadAddress( output, registerFiles->at( load_ins->getHWThread() ) , &load_addr, &load_width );
					load_addr = load_addr & max_mem_address_mask;

					output->verbose(CALL_INFO, 16, 0, "-> LSQ issuing load to 0x%0llx width=%" PRIu16 " (partial? %s)\n", load_addr, load_width,
						load_ins->isPartialLoad() ? "yes" : "no");

					if( load_width > 0 ) {
						output->verbose(CALL_INFO, 16, 0, "-> issuing load into memory interface...\n");

						if( ! load_ins->trapsError() ) {
							if( (load_addr % load_width) == 0 ) {
								SimpleMem::Request* new_load_req = new SimpleMem::Request( SimpleMem::Request::Read,
									load_addr, load_width);
								new_load_req->setInstructionPointer( load_ins->getInstructionAddress() );

								switch( load_ins->getTransactionType() ) {
								case MEM_TRANSACTION_NONE:
									output->verbose(CALL_INFO, 16, 0, "---> [memory-transaction]: standard load\n");
									break;
								case MEM_TRANSACTION_LLSC_LOAD:
									output->verbose(CALL_INFO, 16, 0, "---> [memory-transaction]: marked for LLSC with load transaction type\n");
									new_load_req->flags |= SST::Interfaces::SimpleMem::Request::F_LLSC;
									break;
								case MEM_TRANSACTION_LLSC_STORE:
									output->fatal(CALL_INFO, -1, "Error - logical error, LOAD instruction is marked with an LLSC STORE transaction class.\n");
								case MEM_TRANSACTION_LOCK:
									output->verbose(CALL_INFO, 16, 0, "---> [memory-transaction]: marked for LOCK load transaction\n");
									new_load_req->flags |= SST::Interfaces::SimpleMem::Request::F_LOCKED;
									break;
								}

								pending_loads.insert( std::pair<SimpleMem::Request::id_t, VanadisLoadRecord*>(new_load_req->id, new VanadisLoadRecord( load_ins )) );
								memInterface->sendRequest( new_load_req );

								pending_mem_issued_loads++;
							} else {
								output->verbose(CALL_INFO, 16, 0, "-> fails alignment check, marking instruction 0x%llx as causing error.\n",
									load_ins->getInstructionAddress());
								load_ins->flagError();
							}
						}
					} else {
						output->verbose(CALL_INFO, 16, 0, "-> load width is zero, will not issue a load and just complete instruction.\n");
						output->verbose(CALL_INFO, 16, 0, "-> marking instruction as executed to allow ROB clear.\n");
						load_ins->markExecuted();
					}

					// Remove this load from the queue and fix up the iterator
					next_load = load_q.erase( next_load );
					pending_queued_loads--;
				} else {
					output->verbose(CALL_INFO, 16, 0, "-> LSQ -> attempt to issue load but pending memory operation slots are full.\n");
					break;
				}
			} else if( load_eval == FORWARD_STORE ) {
				// Store forwarding has already been done, load record is cleared to be
				// removed as we have satisfied the data request
				output->verbose(CALL_INFO, 16, 0, "-> LSQ load is resolved by store forward, clear from queue\n");
				next_load = load_q.erase( next_load );
				pending_queued_loads--;
			} else {
				next_load++;
			}
		}
	}

	int tick_stores( const uint64_t cycle ) {
		output->verbose(CALL_INFO, 16, 0, "---> LSQ -> Ticking Store Processing (store_q = %" PRIu32 " items)\n", (uint32_t) store_q->size() );
		int tick_rc = 1;

		if( processingLLSC ) {
			output->verbose(CALL_INFO, 16, 0, "---> LSQ has a pending LLSC store operation, will not process any other stores.\n");
			return tick_rc;
		}

		if( ! store_q->empty() ) {
			VanadisStoreRecord* front_record = store_q->peek();
			VanadisStoreInstruction* front_store = front_record->getAssociatedInstruction();

			output->verbose(CALL_INFO, 16, 0, "---> LSQ -> Check front of ROB? %s\n",
				front_store->checkFrontOfROB() ? "yes" : "no" );

			// Are we at the front of the ROB, if yes, we have avoided
			// any bad speculation and so can go ahead and issue to memory
			if( front_store->checkFrontOfROB() ) {
				// Are we using all the slots to issue stores to L1 we have available?
				if( pending_mem_issued_stores < max_mem_issued_stores ) {
					VanadisRegisterFile* hw_thr_reg = registerFiles->at( front_store->getHWThread() );
					uint64_t store_address = 0;
					uint16_t store_width   = 0;

					front_store->computeStoreAddress( output, hw_thr_reg, &store_address, &store_width );

					std::vector<uint8_t> payload;
					uint8_t* value_reg_addr = nullptr;

					switch( front_store->getValueRegisterType() ) {
					case STORE_INT_REGISTER:
						value_reg_addr = (uint8_t*) hw_thr_reg->getIntReg( front_store->getPhysIntRegIn(1) );
						break;
					case STORE_FP_REGISTER:
						value_reg_addr = (uint8_t*) hw_thr_reg->getFPReg( front_store->getPhysFPRegIn(0) );
						break;
					default:
						output->fatal(CALL_INFO, -1, "Unknown register type.\n");
					}

					output->verbose(CALL_INFO, 16, 0, "----> LSQ is-partial-store? %s\n",
						front_store->isPartialStore() ? "yes" : "no");

					if( front_store->isPartialStore() ) {
						uint16_t reg_offset = front_store->getRegisterOffset();

						output->verbose(CALL_INFO, 16, 0, "----> reg-offset for partial store: %" PRIu16 "\n",
							reg_offset);

						for( uint16_t j = 0; j < store_width; ++j ) {
							payload.push_back( value_reg_addr[ reg_offset + j ]);
						}
					} else {
						if( 0 == (store_address % store_width) ) {
							for( uint16_t j = 0; j < store_width; ++j ) {
								payload.push_back( value_reg_addr[j] );
							}
						} else {
							output->fatal( CALL_INFO, -1, "Error - store alignment check fails address: 0x%llx / width: %" PRIu32 "\n",
								store_address, (uint32_t) store_width);
						}
					}

					output->verbose(CALL_INFO, 16, 0, "----> LSQ creating memory store request: payload-len: %" PRIu32 "\n",
						(uint32_t) payload.size());

					SimpleMem::Request* new_store_req = new SimpleMem::Request(
						SimpleMem::Request::Write,
						store_address, payload.size() , payload );
					new_store_req->setInstructionPointer( front_store->getInstructionAddress() );

					switch( front_store->getTransactionType() ) {
                                                case MEM_TRANSACTION_NONE:
                                                        output->verbose(CALL_INFO, 16, 0, "---> [memory-transaction]: standard store\n");
                                                        break;
                                                case MEM_TRANSACTION_LLSC_LOAD:
							output->fatal(CALL_INFO, -1, "---> [memory-transaction]: marked for LLSC with LOAD transaction, logical error.\n");
                                                        break;
                                                case MEM_TRANSACTION_LLSC_STORE:
							output->verbose(CALL_INFO, 16, 0, "---> [memory-transaction]: marked for LLSC-STORE transaction\n");
							new_store_req->flags |= SST::Interfaces::SimpleMem::Request::F_LLSC;
							processingLLSC = true;
							break;
                                                case MEM_TRANSACTION_LOCK:
                                                        output->verbose(CALL_INFO, 16, 0, "---> [memory-transaction]: marked for LOCK store transaction\n");
                                                        new_store_req->flags |= SST::Interfaces::SimpleMem::Request::F_LOCKED;
                                             	break;
                                        }

					output->verbose(CALL_INFO, 16, 0, "---> LSQ -> issuing store to cache, addr=%p / %" PRIu64 ", width=%" PRIu16 " bytes\n", (void*) store_address, store_address, store_width);

					memInterface->sendRequest( new_store_req );
					pending_stores.insert( new_store_req->id );

					pending_mem_issued_stores++;

					if( ! processingLLSC ) {
						// Mark the instruction as executed and clear it from our queue
						front_store->markExecuted();
						store_q->pop();

						// delete the record, but not the instruction
						// the main core ROB engine will do that for us
						delete front_record;

						// Return code set to zero indicates we processed something
						// and want additional tick_store calls if we are allowed them
						// during this cycle.
						tick_rc = 0;
					}
				}
			}
		} else {
			tick_rc = -1;
		}

		return tick_rc;
	}

	void processIncomingDataCacheEvent( SimpleMem::Request* ev ) {
		auto check_ev_exists = pending_stores.find( ev->id );

		if( check_ev_exists == pending_stores.end() ) {
			output->verbose(CALL_INFO, 16, 0, "--> Does not match a store entry.\n");
			auto check_ev_load_exists = pending_loads.find( ev->id );

			if( check_ev_load_exists == pending_loads.end() ) {
				output->verbose(CALL_INFO, 16, 0, "--> Does not match a load entry.\n");
			} else {
				output->verbose(CALL_INFO, 16, 0, "--> LSQ match load entry, unpacking payload (load-addr: 0x%0llx).\n",
					ev->addr);

				VanadisLoadRecord* load_record = check_ev_load_exists->second;
				VanadisLoadInstruction* load_ins = load_record->getAssociatedInstruction();

				const uint16_t target_reg = load_ins->getPhysIntRegOut(0);
				const uint16_t load_width = load_ins->getLoadWidth();
				const uint32_t hw_thr     = load_ins->getHWThread();

				if( load_ins->isPartialLoad() ) {
					uint64_t recompute_address = 0;
					uint16_t recompute_width   = 0;

					load_ins->computeLoadAddress( registerFiles->at( hw_thr ), &recompute_address, &recompute_width );
					const uint16_t reg_offset = load_ins->getRegisterOffset();

					output->verbose(CALL_INFO, 16, 0, "-> LSQ matched an unaligned/partial load, target: %" PRIu16 " / reg-offset: %" PRIu16 " / full-width: %" PRIu16 " / partial-width: %" PRIu16 "\n",
						target_reg, reg_offset, load_width, recompute_width);

					// check we aren't writing to a zero register, if we are just drop the update
					if( target_reg != load_ins->getISAOptions()->getRegisterIgnoreWrites() ) {
						uint8_t* reg_ptr = (uint8_t*) registerFiles->at( hw_thr )->getIntReg( target_reg );

						for( uint16_t i = 0; i < recompute_width; ++i ) {
							reg_ptr[ reg_offset + i ] = ev->data[i];
						}

						// if we are not at offset zero then or we are loading the entire width, we need to perform an update to the full register
						if( (reg_offset > 0) || (recompute_width == load_ins->getLoadWidth()) ) {
							// if we are loading less than 8 bytes, because otherwise the full data width of the register is set
							if( load_ins->getLoadWidth() < 8 ) {
								uint64_t extended_value = 0;

								switch( load_ins->getLoadWidth() ) {
								case 2:
									{
										uint16_t* reg_ptr_16 = (uint16_t*) reg_ptr;
										extended_value = vanadis_sign_extend( (*reg_ptr_16) );
									}
									break;

								case 4:
									{
										uint32_t* reg_ptr_32 = (uint32_t*) reg_ptr;
										extended_value = vanadis_sign_extend( (*reg_ptr_32) );
									}
									break;
								}

								// Put the sign extended value into the register
								registerFiles->at( hw_thr )->setIntReg( target_reg, extended_value );
							}
						}
					}
				} else {
					output->verbose(CALL_INFO, 16, 0, "-> LSQ matched to load hw_thr = %" PRIu32 ", target_reg = %" PRIu16 ", width=%" PRIu16 "\n",
						hw_thr, target_reg, load_width);
					int64_t new_value = 0;

					switch( load_width ) {

					case 1:
						new_value = vanadis_sign_extend( ev->data[0] );
						break;
					case 2:
					{
						uint16_t* val_16 = (uint16_t*) &ev->data[0];
						new_value = vanadis_sign_extend( *val_16 );
					}
						break;
					case 4:
					{
						uint32_t* val_32 = (uint32_t*) &ev->data[0];
						new_value = vanadis_sign_extend( *val_32 );
					}
						break;
					case 8:
					{
						uint64_t* val_64 = (uint64_t*) &ev->data[0];
						new_value = *val_64;
					}
						break;

					default:
						output->fatal(CALL_INFO, -1, "Error: load-instruction forces a load which is not power-of-2: width=%" PRIu16 "\n", load_width);
						break;
					}

					output->verbose(CALL_INFO, 16, 0, "---> LSQ (ins: 0x%0llx) set sign-extended register value for r: %" PRIu16 ", v: %" PRId64 " / 0x%0llx\n",
						load_ins->getInstructionAddress(), target_reg, new_value, new_value );

					// Set the register value
					registerFiles->at( hw_thr )->setIntReg( target_reg, new_value );
				}

				output->verbose(CALL_INFO, 16, 0, "---> LSQ Execute: %s (0x%llx) load data instruction marked executed.\n",
					load_ins->getInstCode(), load_ins->getInstructionAddress() );

				if( ev->addr < 64 ) {
					output->fatal(CALL_INFO, -1, "Error - load operation at instruction 0x%llx address: 0x%llx is less than virtual address 64, this would be a segmentation fault.\n",
						load_ins->getInstructionAddress(), ev->addr );
				}

				load_ins->markExecuted();
				pending_loads.erase( check_ev_load_exists );
				pending_mem_issued_loads--;

				delete load_record;
			}
		} else {
			output->verbose(CALL_INFO, 16, 0, "-> LSQ match store entry, cleaning entry list.\n");
			output->verbose(CALL_INFO, 16, 0, "---> ev-null? %s\n", nullptr == ev ? "yes" : "no");
			output->verbose(CALL_INFO, 16, 0, "---> response flags: 0x%0llx\n", (uint64_t) ev->flags);

			// This is an LLSC requested store (i.e. SC)
			if( (ev->flags & SST::Interfaces::SimpleMem::Request::F_LLSC) != 0 ) {
				if( processingLLSC ) {
					// Check to see if this is a LLSC response event
					if( store_q->empty() ) {
						output->fatal(CALL_INFO, -1, "Error - received an LLSC response event, but store queue is empty\n");
					} else {
						VanadisStoreRecord* front_record = store_q->pop();
						VanadisStoreInstruction* front_store = front_record->getAssociatedInstruction();

						if( front_store->getTransactionType() != MEM_TRANSACTION_LLSC_STORE ) {
							output->fatal(CALL_INFO, -1, "Error - received an LLSC response event, but store queue front is not an LLSC transaction.\n");
						} else {
							output->verbose(CALL_INFO, 16, 0, "---> LSQ LLSC-STORE handled for ins: 0x%0llx, marked executed and LLSC/LSQ cleared for resuming operation\n",
								front_store->getInstructionAddress());

							const uint16_t value_reg = front_store->getPhysIntRegOut(0);

							if( (ev->flags & SST::Interfaces::SimpleMem::Request::F_LLSC_RESP)  != 0 ) {
								output->verbose(CALL_INFO, 16, 0, "---> LSQ LLSC-STORE rt: %" PRIu16 " <- 1 (success)\n", value_reg );
								registerFiles->at( front_store->getHWThread() )->setIntReg( value_reg, (uint64_t) 1 );
							} else {
								output->verbose(CALL_INFO, 16, 0, "---> LSQ LLSC-STORE rt: %" PRIu16 " <- 0 (failed)\n", value_reg );
								registerFiles->at( front_store->getHWThread() )->setIntReg( value_reg, (uint64_t) 0 );
							}

							output->verbose(CALL_INFO, 16, 0, "---> LSQ Execute %s (0x%llx) store operation executed.\n",
								front_store->getInstCode(), front_store->getInstructionAddress() );

							if( ev->addr < 64 ) {
								output->fatal(CALL_INFO, -1, "Error - store operation at 0x%llx address is less than virtual address 64, this would be a segmentation fault.\n",
									front_store->getInstructionAddress() );
							}

							front_store->markExecuted();
						}
					}
				} else {
					output->fatal(CALL_INFO, -1, "Response from cache was a store response with LLSC/SC marked, but core is not processing an LLSC event. Logical error?\n");
				}
			}

			processingLLSC = false;

			// Found in the Pending Store List
			pending_stores.erase( check_ev_exists );
			pending_mem_issued_stores--;
		}
	}

	void clearLSQByThreadID( const uint32_t thr ) {
		// Clear pending loads, returns will get dropped when we cannot match them
		for( auto load_itr = pending_loads.begin(); load_itr != pending_loads.end(); ) {
			if( load_itr->second->getAssociatedInstruction()->getHWThread() == thr ) {
				delete load_itr->second;
				load_itr = pending_loads.erase( load_itr );
				pending_mem_issued_loads--;
			} else {
				load_itr++;
			}
		}

		for( auto load_q_itr = load_q.begin(); load_q_itr != load_q.end(); ) {
			if( (*load_q_itr)->getAssociatedInstruction()->getHWThread() == thr ) {
				delete (*load_q_itr);
				load_q_itr = load_q.erase( load_q_itr );
				pending_queued_loads--;
			} else {
				load_q_itr++;
			}
		}

		VanadisCircularQueue<VanadisStoreRecord*>* sq_tmp = new
			VanadisCircularQueue<VanadisStoreRecord*>(max_queued_stores);

		for( size_t sq_itr = 0; sq_itr < store_q->size(); sq_itr++ ) {
			VanadisStoreRecord* tmp_srec = store_q->pop();

			if( tmp_srec->getAssociatedInstruction()->getHWThread() == thr ) {
				delete tmp_srec;
			} else {
				sq_tmp->push( tmp_srec );
			}
		}

		// Swap out queued stores to new queue and reset the counter
		delete store_q;
		store_q = sq_tmp;
	}

protected:
	VanadisCircularQueue<VanadisStoreRecord*>* store_q;
	std::list<VanadisLoadRecord*> load_q;

	SimpleMem* memInterface;

	uint32_t max_queued_stores;
	uint32_t max_queued_loads;

	uint32_t pending_queued_loads;

	uint32_t max_mem_issued_stores;
	uint32_t max_mem_issued_loads;

	uint32_t pending_mem_issued_stores;
	uint32_t pending_mem_issued_loads;

	uint32_t max_stores_issue_per_cycle;
	uint32_t max_load_issue_per_cycle;

	uint64_t max_mem_address_mask;

	std::set<SimpleMem::Request::id_t> pending_stores;
	std::map<SimpleMem::Request::id_t, VanadisLoadRecord*> pending_loads;

	bool processingLLSC;
};

}
}

#endif
