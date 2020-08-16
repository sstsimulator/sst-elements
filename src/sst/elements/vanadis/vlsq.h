
#ifndef _H_VANADIS_LOAD_STORE_Q
#define _H_VANADIS_LOAD_STORE_Q

#include "sst/core/interfaces/simpleMem.h"

#include "inst/vinst.h"
#include "inst/vstore.h"
#include "inst/vload.h"

#include "datastruct/cqueue.h"
#include "util/vsignx.h"

#include <map>
#include <set>

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

	bool predates( VanadisInstruction* check ) { return gen_ins->getID() < check->getID(); }
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

class VanadisLoadStoreQueue {

public:
	VanadisLoadStoreQueue(
		SimpleMem* memI, 
		const size_t lsq_store_entries,
		const size_t lsq_load_entries,
		const size_t max_issue_store_count,
		const size_t max_issue_load_count,
		const size_t max_stores_issue_each_cycle,
		const size_t max_loads_issue_each_cycle,
		std::vector<VanadisRegisterFile*>* reg_files
		)	 : 	
			memInterface(memI),
			max_mem_issued_stores(max_issue_store_count),
			max_mem_issued_loads(max_issue_load_count),
			max_queued_stores(lsq_store_entries),
			max_queued_loads(lsq_load_entries),
			pending_queued_stores(0),
			pending_queued_loads(0),
			pending_mem_issued_stores(0),
			pending_mem_issued_loads(0),
			max_stores_issue_per_cycle(max_stores_issue_each_cycle),
			max_load_issue_per_cycle(max_loads_issue_each_cycle),
			registerFiles(reg_files) {

		store_q = new VanadisCircularQueue<VanadisStoreRecord*>(lsq_store_entries);
	}

	~VanadisLoadStoreQueue() {
		delete store_q;
	}

	bool storeFull() const 	 { return (pending_queued_stores >= max_queued_stores); }
	bool loadFull() const    { return (pending_queued_loads  >= max_queued_loads);  }

	size_t storeSize() const { return pending_queued_stores; }
	size_t loadSize()  const { return pending_queued_loads;  }

	void push( VanadisStoreInstruction* store_me ) {
		store_q->push( new VanadisStoreRecord( store_me ) );
		pending_queued_stores++;
	}

	void push( VanadisLoadInstruction* load_me ) {
		load_q.push_back( new VanadisLoadRecord( load_me ) );
		pending_queued_loads++;
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

	void tick( uint64_t cycle, SST::Output* output ) {
		output->verbose(CALL_INFO, 16, 0, "-> Ticking Load/Store Queue Processors...\n");
		output->verbose(CALL_INFO, 16, 0, "---> LSQ contains: %" PRIu32 " queued loads / %" PRIu32 " queued stores.\n", pending_queued_loads, pending_queued_stores );
		output->verbose(CALL_INFO, 16, 0, "---> LSQ contains: %" PRIu32 " loads in flight / %" PRIu32 " stores in flight\n", pending_mem_issued_loads, pending_mem_issued_stores );
		output->verbose(CALL_INFO, 16, 0, "-> Ticking Load Queue Handling...\n");
		tick_loads(cycle, output);
		output->verbose(CALL_INFO, 16, 0, "---> Ticking Store Queue Handling (max stores per cycle = %" PRIu32 ")\n", max_stores_issue_per_cycle);
		int stores_this_cycle = 0;
		for( uint32_t i = 0; i < max_stores_issue_per_cycle; ++i ) {
			int rc = tick_stores(cycle, output);

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

	void tick_loads( const uint64_t cycle, SST::Output* output ) {
		for( auto next_load = load_q.begin(); next_load != load_q.end(); ) {
			VanadisLoadInstruction* load_ins = (*next_load)->getAssociatedInstruction();

			output->verbose(CALL_INFO, 16, 0, "-> LSQ inspect load record: executed? %s\n",
				(load_ins->completedExecution() ? "yes" : "no"));

			// If instruction has already been issued, we skip it, and will remove it
			// from the queue later
			if( load_ins->completedExecution() ) {
				break;
			}

			const uint64_t load_address = load_ins->computeLoadAddress(
				registerFiles->at( load_ins->getHWThread() ) );

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

					const uint64_t store_address = check_store_ins->computeStoreAddress(
						registerFiles->at( load_ins->getHWThread() ) );

					output->verbose(CALL_INFO, 16, 0, "-> LSQ compare load (%p, width=%" PRIu16 ") to store at (%p, width=%" PRIu16 ")\n",
						(void*) load_address, load_ins->getLoadWidth(),
						(void*) store_address, check_store_ins->getStoreWidth());

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
							const uint64_t store_value = (*((uint64_t*) reg_file->getIntReg(check_store_ins->getPhysIntRegIn()[1]) ));

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
					const uint64_t load_addr = load_ins->computeLoadAddress(
						registerFiles->at( load_ins->getHWThread() ) );
					const uint16_t load_width = load_ins->getLoadWidth();
					output->verbose(CALL_INFO, 16, 0, "-> LSQ issuing load to %p width=%" PRIu16 "\n", (void*) load_addr, load_width);

					SimpleMem::Request* new_load_req = new SimpleMem::Request( SimpleMem::Request::Read,
						load_addr, load_width);

					pending_loads.insert( std::pair<SimpleMem::Request::id_t, VanadisLoadRecord*>(new_load_req->id, new VanadisLoadRecord( load_ins )) );
					memInterface->sendRequest( new_load_req );

					pending_mem_issued_loads++;

					// Remove this load from the queue and fix up the iterator
					next_load = load_q.erase( next_load );
					pending_queued_loads--;
				} else {
					output->verbose(CALL_INFO, 16, 0, "-> LSQ -> attempt to issue load but pending memory operation slots are full.\n");
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

	int tick_stores( const uint64_t cycle, SST::Output* output ) {
		output->verbose(CALL_INFO, 16, 0, "---> LSQ -> Ticking Store Processing (store_q = %" PRIu32 " items)\n", (uint32_t) store_q->size() );
		int tick_rc = 1;

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
					const uint64_t store_address = front_store->computeStoreAddress( hw_thr_reg );
					const uint16_t store_width   = front_store->getStoreWidth();

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

					for( uint16_t j = 0; j < store_width; ++j ) {
						payload.push_back( value_reg_addr[j] );
					}

					SimpleMem::Request* new_store_req = new SimpleMem::Request(
						SimpleMem::Request::Write,
						store_address, store_width , payload );

					output->verbose(CALL_INFO, 16, 0, "---> LSQ -> issuing store to cache, addr=%p / %" PRIu64 ", width=%" PRIu16 " bytes\n", (void*) store_address, store_address, store_width);

					memInterface->sendRequest( new_store_req );
					pending_stores.insert( new_store_req->id );
	
					pending_mem_issued_stores++;

					// Mark the instruction as executed and clear it from our queue
					front_store->markExecuted();
					store_q->pop();

					pending_queued_stores--;

					// delete the record, but not the instruction
					// the main core ROB engine will do that for us
					delete front_record;

					// Return code set to zero indicates we processed something
					// and want additional tick_store calls if we are allowed them
					// during this cycle.
					tick_rc = 0;
				}
			}
		} else {
			tick_rc = -1;
		}

		return tick_rc;
	}

	void processIncomingDataCacheEvent( SST::Output* output, SimpleMem::Request* ev ) {
		auto check_ev_exists = pending_stores.find( ev->id );

		if( check_ev_exists == pending_stores.end() ) {
			auto check_ev_load_exists = pending_loads.find( ev->id );

			if( check_ev_load_exists == pending_loads.end() ) {

			} else {
				output->verbose(CALL_INFO, 16, 0, "-> LSQ match load entry, unpacking payload.\n");
				VanadisLoadRecord* load_record = check_ev_load_exists->second;
				VanadisLoadInstruction* load_ins = load_record->getAssociatedInstruction();

				const uint16_t target_reg = load_ins->getPhysIntRegOut(0);
				const uint16_t load_width = load_ins->getLoadWidth();
				const uint32_t hw_thr     = load_ins->getHWThread();

				output->verbose(CALL_INFO, 16, 0, "-> LSQ matched to load hw_thr = %" PRIu32 ", target_reg = %" PRIu16 ", width=%" PRIu16 "\n",
					hw_thr, target_reg, load_width);
/*
				registerFiles->at( hw_thr )->setIntReg( target_reg, (uint64_t) 0);

				uint8_t* reg_ptr = (uint8_t*) registerFiles->at( hw_thr )->getIntReg( target_reg );

				// Copy out the payload for the register
				for( uint16_t i = 0; i < load_width; ++i ) {
					reg_ptr[i] = ev->data[i];
				}

				const uint8_t check_sign_mask = 0b10000000;
				const uint8_t all_bits_set    = 0b11111111;

				// Do we need to perform sign extension

				if( load_ins->performSignExtension() ) {
					if( reg_ptr[(load_width-1)] & check_sign_mask != 0 ) {
						// The sign is bit is set
						for( uint16_t i = load_width; i < 8; ++i ) {
							reg_ptr[i] = all_bits_set;
						}
					}
				}
*/
				uint64_t new_value = 0;

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

				// Set the register value
				registerFiles->at( hw_thr )->setIntReg( target_reg, new_value );

				load_ins->markExecuted();
				pending_loads.erase( check_ev_load_exists );
				pending_mem_issued_loads--;

				delete load_record;
			}
		} else {
			output->verbose(CALL_INFO, 16, 0, "-> LSQ match store entry, cleaning entry list.\n");

			// Found in the Pending Store List
			pending_stores.erase( check_ev_exists );
			pending_mem_issued_stores--;
		}
	}

	void clearLSQByThreadID( SST::Output* output, const uint32_t thr ) {
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
		pending_queued_stores = store_q->size();
	}

protected:
	VanadisCircularQueue<VanadisStoreRecord*>* store_q;
	std::list<VanadisLoadRecord*> load_q;

	std::vector<VanadisRegisterFile*>* registerFiles;
	SimpleMem* memInterface;

	const uint32_t max_queued_stores;
	const uint32_t max_queued_loads;

	uint32_t pending_queued_stores;
	uint32_t pending_queued_loads;

	const uint32_t max_mem_issued_stores;
	const uint32_t max_mem_issued_loads;

	uint32_t pending_mem_issued_stores;
	uint32_t pending_mem_issued_loads;

	const uint32_t max_stores_issue_per_cycle;
	const uint32_t max_load_issue_per_cycle;

	std::set<SimpleMem::Request::id_t> pending_stores;
	std::map<SimpleMem::Request::id_t, VanadisLoadRecord*> pending_loads;
};

}
}

#endif
