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

#ifndef _H_SERRANO_BINARY_OP_CG_UNIT
#define _H_SERRANO_BINARY_OP_CG_UNIT

#include <functional>

#include "smsg.h"
#include "sercgunit.h"
#include "scircq.h"

namespace SST {
namespace Serrano {

enum SerranoStandardOp {
	OP_ADD,
	OP_SUB,
	OP_DIV,
	OP_MUL,
	OP_MOD,
	OP_MSG_DUPLICATE,
	OP_MSG_INTERLEAVE,
	OP_CUSTOM
};

class SerranoBasicUnit : public SerranoCoarseUnit {

public:
	SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(
		SST::Serrano::SerranoBasicUnit,
		"serrano",
		"SerranoBasicUnit",
		SST_ELI_ELEMENT_VERSION(1, 0, 0),
		"Basic coarse-grained functional unit for simple operations",
		SST::Serrano::SerranoCoarseUnit )

	SST_ELI_DOCUMENT_PARAMS()
	SST_ELI_DOCUMENT_STATISTICS()				

	SerranoBasicUnit( SST::ComponentId_t id, Params& params ) :
		SerranoCoarseUnit(id, params) {

		required_in_qs = 0;
		required_out_qs = 0;
	}

	~SerranoBasicUnit() {
		msgs_in.clear();
	}

	void configureFunction( SST::Output* output, SerranoStandardOp op, SerranoStandardType dt ) {
		switch( op ) {
		case OP_ADD:
			switch( dt ) {
			case TYPE_INT32: unit_func = std::bind( &SST::Serrano::SerranoBasicUnit::execute_i32_add, this, std::placeholders::_1, std::placeholders::_2 ); break;
			case TYPE_INT64: unit_func = std::bind( &SST::Serrano::SerranoBasicUnit::execute_i64_add, this, std::placeholders::_1, std::placeholders::_2 ); break;
			case TYPE_FP32: unit_func = std::bind( &SST::Serrano::SerranoBasicUnit::execute_f32_add, this, std::placeholders::_1, std::placeholders::_2 ); break;
			case TYPE_FP64: unit_func = std::bind( &SST::Serrano::SerranoBasicUnit::execute_f64_add, this, std::placeholders::_1, std::placeholders::_2 ); break;
			default:
				output->fatal(CALL_INFO, -1, "Unknown data type supplied to an add operation.\n");
				break;
			}

			required_in_qs = 2;
			required_out_qs = 1;

			break;
		default:
			output->verbose(CALL_INFO, 2, 0, "Function was not decoded, and so will not be set. This will likely cause a fatal later in execution.\n");
		}
	}

	void checkRequiredQueues( SST::Output* output ) {
		if( ( required_in_qs >= input_qs.size() ) &&
		    ( required_out_qs >= output_qs.size() ) ) {
			
		} else {
			output->fatal(CALL_INFO, -1, "Error: required queues were not matched. in (req/av): %d/%d, out (req/av): %d/%d\n",
				(int) required_in_qs, (int) input_qs.size(), (int) required_out_qs, (int) output_qs.size() );
		}
	}

	virtual const char* getUnitTypeString() {
		return "STD-UNIT";
	}

	virtual bool stillProcessing() { return false; }

	virtual void execute( const uint64_t current_cycle ) {
		if( nullptr == unit_func ) {
			output->fatal(CALL_INFO, -1, "Error: function to execute has not been defined or was not decoded correctly.\n");
		}

		bool all_ins_ready = true;
		bool out_ready     = (! output_qs[0]->full());

		for( SerranoCircularQueue<SerranoMessage*>* in_q : input_qs ) {
			all_ins_ready &= (!in_q->empty());
		}

		if( all_ins_ready & out_ready ) {
			// We are good to go, all inputs have a message, output has a slot
			for( SerranoCircularQueue<SerranoMessage*>* in_q : input_qs ) {
				msgs_in.push_back( in_q->pop() );
			}

			// Execute the function
			unit_func( output, msgs_in );

			// Delete the messages from the incoming queues to free memory
			for( SerranoMessage* in_msg : msgs_in ) {
				delete in_msg;
			}

			// Clear the vector this cycle
			msgs_in.clear();
		} else {
			output->verbose(CALL_INFO, 8, 0, "Unable to execute this cycle due to queue-check failing: in-q: %s / out-q: %s\n",
				(all_ins_ready) ? "ready" : "not-ready", (out_ready) ? "ready" : "not-ready" );			
		}
	}

protected:
	template<class T> void execute_add( std::vector<SerranoMessage*>& msg_in, const T init_value ) {
		T result = init_value;

		for( SerranoMessage* msg : msg_in ) {
                        result += extractValue<T>( output, msg );
                }

		output_qs[0]->push( constructMessage<T>( result ) );
	}

	template<class T> void execute_sub( std::vector<SerranoMessage*>& msg_in, const T init_value ) {
		T result = init_value;

		for( SerranoMessage* msg : msg_in ) {
                        result -= extractValue<T>( output, msg );
                }

		output_qs[0]->push( constructMessage<T>( result ) );
	}

	void execute_i32_add( SST::Output* output, std::vector<SerranoMessage*>& msg_in ) {
		execute_add<int32_t>(msg_in, 0);
	}

	void execute_u32_add( SST::Output* output, std::vector<SerranoMessage*>& msg_in ) {
		execute_add<uint32_t>(msg_in, 0);
	}

	void execute_i64_add( SST::Output* output, std::vector<SerranoMessage*>& msg_in ) {
		execute_add<int64_t>(msg_in, 0);
	}
	
	void execute_u64_add( SST::Output* output, std::vector<SerranoMessage*>& msg_in ) {
		execute_add<uint64_t>(msg_in, 0);
	}

	void execute_f32_add( SST::Output* output, std::vector<SerranoMessage*>& msg_in ) {
		execute_add<float>(msg_in, 0.0);
	}

	void execute_f64_add( SST::Output* output, std::vector<SerranoMessage*>& msg_in ) {
		execute_add<double>(msg_in, 0.0);
	}
	
	std::vector<SerranoMessage*> msgs_in;
	std::function< void( SST::Output*, std::vector<SerranoMessage*>& )> unit_func;

	size_t required_in_qs;
	size_t required_out_qs;

};

}
}

#endif
