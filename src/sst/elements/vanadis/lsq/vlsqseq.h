
#ifndef _H_VANADIS_LSQ_SEQUENTIAL
#define _H_VANADIS_LSQ_SEQUENTIAL

#include <deque>

#include "lsq/vlsq.h"
#include "inst/vinst.h"

namespace SST {
namespace Vanadis {

class VanadisSequentialLoadStoreRecord {
public:
	VanadisSequentialLoadStoreRecord( VanadisInstruction* instr ) {
		ins = instr;
		issued = false;
		req_id = 0;
	}
	
	bool isOperationIssued() {
		return issued;
	}
	
	void markOperationIssued() {
		issued = true;
	}

	VanadisInstruction* getInstruction() {
		return ins;
	}
	
	SimpleMem::Request::id_t getRequestID() {
		return req_id;
	}
	
	void setRequestID( SimpleMem::Request::id_t new_id ) {
		req_id = new_id;
	}
	
	bool isStore() {
		return ins->getInstFuncType() == INST_STORE;
	}
	
	bool isLoad() {
		return ins->getInstFuncType() == INST_LOAD;
	}

protected:
	SimpleMem::Request::id_t req_id;
	VanadisInstruction* ins;
	bool issued;

};

class VanadisSequentialLoadStoreQueue : public SST::Vanadis::VanadisLoadStoreQueue {
public:
	SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(
		VanadisSequentialLoadStoreQueue,
		"vanadis",
		"VanadisSequentialLoadStoreQueue",
		SST_ELI_ELEMENT_VERSION(1,0,0),
		"Implements a sequential load-store for use with the SST memInterface",
		SST::Vanadis::VanadisLoadStoreQueue
	)

	SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS(
		{ "memory_interface",		"Set the interface to memory",		"SST::Interfaces::SimpleMem" 		}
	)

	SST_ELI_DOCUMENT_PORTS(
		{ "dcache_link",		"Connects the LSQ to the data cache",	{}					}
	)

	SST_ELI_DOCUMENT_PARAMS(
		{ "load_store_entries",		"Set the number of load/stores that can be pending",	"8"		}
	)
	
	VanadisSequentialLoadStoreQueue( ComponentId_t id, Params& params ) :
		VanadisLoadStoreQueue( id, params ) {
		
		memInterface = loadUserSubComponent<Interfaces::SimpleMem>("memory_interface", ComponentInfo::SHARE_PORTS |
			ComponentInfo::INSERT_STATS, getTimeConverter("1ps"),
			new SimpleMem::Handler<SST::Vanadis::VanadisSequentialLoadStoreQueue>(this,
			&VanadisSequentialLoadStoreQueue::processIncomingDataCacheEvent));
			
		if( nullptr == memInterface ) {
			output->fatal(CALL_INFO, -1, "Error - unable to load \"memory_interface\"\n");
		}
		
		max_q_size = params.find<size_t>("load_store_entries", 8);
		
		output->verbose(CALL_INFO, 2, 0, "LSQ Load/Store Queue entry count:     %" PRIu32 "\n",
			(size_t) max_q_size);
	}
	
	virtual ~VanadisSequentialLoadStoreQueue() {
		delete memInterface;
	}
	
	virtual bool storeFull() {
		return op_q.size() >= max_q_size;
	}
	
	virtual bool loadFull() {
		return op_q.size() >= max_q_size;
	}
	
	virtual size_t storeSize() {
		return max_q_size;
	}
	
	virtual size_t loadSize() {
		return max_q_size;
	}
	
	virtual void push( VanadisStoreInstruction* store_me ) {
		if( op_q.size() < max_q_size ) {
			output->verbose(CALL_INFO, 8, 0, "enqueue store ins-addr: 0x%llx\n",
				store_me->getInstructionAddress() );
			op_q.push_back( new VanadisSequentialLoadStoreRecord( store_me ) );
		} else {
			output->fatal(CALL_INFO, -1, "Error - attempted to enqueue but no room (max: %" PRIu32 ", size: %" PRIu32 ")\n",
				(uint32_t) max_q_size, (uint32_t) op_q.size());
		}
	}
	
	virtual void push( VanadisLoadInstruction* load_me ) {
		if( op_q.size() < max_q_size ) {
			output->verbose(CALL_INFO, 8, 0, "enqueue load ins-addr: 0x%llx\n",
				load_me->getInstructionAddress() );
			op_q.push_back( new VanadisSequentialLoadStoreRecord( load_me ) );
		} else {
			output->fatal(CALL_INFO, -1, "Error - attempted to enqueue but no room (max: %" PRIu32 ", size: %" PRIu32 ")\n",
				(uint32_t) max_q_size, (uint32_t) op_q.size());
		}	
	}
	
	virtual void tick( uint64_t cycle ) {
		output->verbose(CALL_INFO, 16, 0, "ticking load/store queue at cycle %" PRIu64 " lsq size: %" PRIu64 "\n",
			(uint64_t) cycle, op_q.size() );
	
		VanadisSequentialLoadStoreRecord* next_item = nullptr;
		auto op_q_itr = op_q.begin();
		
		for( ; op_q_itr != op_q.end(); op_q_itr++ ) {
			if( ! (*op_q_itr)->isOperationIssued() ) {
				next_item = (*op_q_itr);
				break;
			}
		}
		
		if( nullptr != next_item ) {
		
			VanadisRegisterFile* reg_file = registerFiles->at( next_item->getInstruction()->getHWThread() );
		
			if( next_item->isLoad() ) {
				VanadisLoadInstruction* load_ins = dynamic_cast<VanadisLoadInstruction*>( next_item->getInstruction() );
				
				uint64_t load_addr  = 0;
				uint16_t load_width = 0;
				
				load_ins->computeLoadAddress( reg_file, &load_addr, &load_width );

				output->verbose( CALL_INFO, 8, 0, "--> issue load for 0x%llx width: %" PRIu16 " bytes.\n",
					load_addr, load_width );
				
				SimpleMem::Request* load_req = new SimpleMem::Request( SimpleMem::Request::Read,
					load_addr, load_width );
					
				if( load_addr < 4096 ) {
					output->verbose(CALL_INFO, 16, 0, "[fault] address for load 0x%llx is less than 4096, indicates segmentation-fault, mark load error (load-ins: 0x%llx)\n",
						load_addr, load_ins->getInstructionAddress() );
					load_ins->flagError();
				} else {			
					memInterface->sendRequest( load_req );
					next_item->setRequestID( load_req->id );
				}
				
				next_item->markOperationIssued();
			} else if( next_item->isStore() ) {
				VanadisStoreInstruction* store_ins = dynamic_cast<VanadisStoreInstruction*>( next_item->getInstruction() );
				
				// Stores must be at the front of the ROB to be executed or else we will
				// potentially violate correct execution in OoO pipelines
				if( store_ins->checkFrontOfROB() ) {
					uint64_t store_addr  = 0;
					uint16_t store_width = 0;
					uint16_t value_reg   = store_ins->getPhysIntRegIn(1); // reg-0 = addr, reg-1 = value
					uint16_t reg_offset  = store_ins->getRegisterOffset();
					
					std::vector<uint8_t> payload;
					char* reg_ptr = reg_file->getIntReg( value_reg );
					
					store_ins->computeStoreAddress( output, reg_file, &store_addr, &store_width );
					
					for( uint16_t i = 0; i < store_width; ++i ) {
						payload.push_back( reg_ptr[reg_offset + i] );
					}
					
					output->verbose( CALL_INFO, 8, 0, "--> issue store at 0x%llx width: %" PRIu16 " bytes, value-reg: %" PRIu16 " / partial? %s / offset: %" PRIu16 "\n",
						store_addr, store_width, value_reg,
						store_ins->isPartialStore() ? "yes" : "no",
						reg_offset );
						
					if( store_addr < 4096 ) {
						output->fatal(CALL_INFO, -1, "[fault] - address 0x%llx is less than 4096, indicates a segmentation fault (store-ins: 0x%llx)\n",
							store_addr, store_ins->getInstructionAddress() );
					} else {
						memInterface->sendRequest( new SimpleMem::Request( SimpleMem::Request::Write,
							store_addr, store_width, payload ) );
					}
					
					store_ins->markExecuted();
					op_q.erase( op_q_itr );
				}
				
			} else {
				output->fatal(CALL_INFO, 8, 0, "Unknown type of item in LSQ, neither load nor store?\n");
			}
		}
		
	}
	
	virtual void processIncomingDataCacheEvent( SimpleMem::Request* ev ) {
		output->verbose(CALL_INFO, 16, 0, "recv incoming d-cache event, addr: 0x%llx, size: %" PRIu32 "\n",
			ev->addr, ev->data.size() );
			
		bool processed = false;
		
		for( auto op_q_itr = op_q.begin(); op_q_itr != op_q.end(); ) {
			// is the operation issued to the memory system and do we match ID
			if( (*op_q_itr)->isOperationIssued() && (
				ev->id == (*op_q_itr)->getRequestID() ) ) {
				
				output->verbose(CALL_INFO, 8, 0, "matched a load record (addr: 0x%llx)\n",
					(*op_q_itr)->getInstruction()->getInstructionAddress());
				
				if( (*op_q_itr)->isLoad() ) {
					VanadisLoadInstruction* load_ins = dynamic_cast<VanadisLoadInstruction*>( (*op_q_itr)->getInstruction() );
					VanadisRegisterFile* reg_file = registerFiles->at( load_ins->getHWThread() );
					uint64_t load_addr  = 0;
					uint16_t load_width = 0;
					uint16_t target_reg = load_ins->getPhysIntRegOut(0);
					uint16_t reg_offset = 0;
					
					load_ins->computeLoadAddress( output, reg_file, &load_addr, &load_width );
					reg_offset = load_ins->getRegisterOffset();
					
					output->verbose(CALL_INFO, 8, 0, "--> load info: addr: 0x%llx / width: %" PRIu16 " / target-reg: %" PRIu16 " / partial? %s / reg-offset: %" PRIu16 "\n",
						load_addr, load_width, target_reg,
						load_ins->isPartialLoad() ? "yes" : "no",
						reg_offset);

					if( target_reg != load_ins->getISAOptions()->getRegisterIgnoreWrites() ) {
						//if( load_ins->isPartialLoad() ) {
						char* reg_ptr = reg_file->getIntReg( target_reg );
						reg_file->setIntReg( target_reg, (uint64_t) 0 );
						
						for( uint16_t i = 0; i < ev->size; ++i ) {
							reg_ptr[ reg_offset + i ] = ev->data[i];
						}
					
						/*
						} else {
							int64_t value = 0;
					
							switch( load_width ) {
							case 1:
								 value = (int64_t)( *((int8_t*) ev->data[0]) );
								 break;
							case 2:
								value = (int64_t)( *((int16_t*) ev->data[0]) );
								 break;
							case 4:
								value = (int64_t)( *((int32_t*) ev->data[0]) );
								 break;
							case 8:
								value = (int64_t)( *((int64_t*) ev->data[0]) );
								 break;
							}
						
							reg_file->setIntReg( target_reg, value );
						}
						*/
					}
					
					// mark instruction as executed
					load_ins->markExecuted();
				}
				
				// Clean up
				delete (*op_q_itr);
				op_q.erase(op_q_itr);
				processed = true;
				
				break;
			} else {
				op_q_itr++;
			}
		}
		
		if( ! processed ) {
			output->verbose(CALL_INFO, 16, 0, "did not match any request.\n");
		}
		
		delete ev;
	}
	
	virtual void clearLSQByThreadID( const uint32_t thread ) {
		output->verbose(CALL_INFO, 8, 0, "clear for thread %" PRIu32 ", size: %" PRIu32 "\n",
			thread, op_q.size() );
	
		for( auto op_q_itr = op_q.begin(); op_q_itr != op_q.end(); ) {
			if( (*op_q_itr)->getInstruction()->getHWThread() == thread ) {
				op_q_itr = op_q.erase(op_q_itr);
			} else {
				op_q_itr++;
			}
		}
		
		output->verbose(CALL_INFO, 8, 0, "clear complete, size: %" PRIu32 "\n",
			op_q.size() );
	}
	
	virtual void init( unsigned int phase ) {
		output->verbose(CALL_INFO, 2, 0, "LSQ Memory Interface Init, Phase %u\n", phase);
		memInterface->init( phase );
	}
	
	virtual void setInitialMemory( const uint64_t addr, std::vector<uint8_t>& payload ) {
		output->verbose(CALL_INFO, 2, 0, "setting initial memory contents for address 0x%llx / size: %" PRIu64 "\n",
			addr, (uint64_t) payload.size() );
		memInterface->sendInitData( new SimpleMem::Request( SimpleMem::Request::Write, addr,
			payload.size(), payload) );
	}

protected:
	size_t max_q_size;
	std::deque<VanadisSequentialLoadStoreRecord*> op_q;
	SimpleMem* memInterface;
	
};

}
}

#endif
