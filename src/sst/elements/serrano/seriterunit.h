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

#ifndef _H_SERRANO_ITERATOR_UNIT
#define _H_SERRANO_ITERATOR_UNIT

#include "sercgunit.h"
#include <functional>

namespace SST {
namespace Serrano {

class SerranoIteratorUnit : public SerranoCoarseUnit {

public:
	SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(
		SST::Serrano::SerranoIteratorUnit,
		"serrano",
		"SerranoIteratorUnit",
		SST_ELI_ELEMENT_VERSION(1, 0, 0),
		"Performs iteration-like behavior",
		SST::Serrano::SerranoCoarseUnit )

	SST_ELI_DOCUMENT_PARAMS(
		{ "start", "Value to start iterating at."      },
		{ "end",   "Value to stop iterating at."       },
		{ "step",  "Value to step the iteration with." },
		{ "data_type", "Type of the iteration value"   }
	)

	SST_ELI_DOCUMENT_STATISTICS()

	SerranoIteratorUnit( SST::ComponentId_t id, SST::Params& params ) :
		SerranoCoarseUnit(id, params) {

		func          = nullptr;
		keep_processing = true;

		const int params_type = params.find<int>("data_type", 1);

		output->verbose(CALL_INFO, 2, 0, "Creating iterator with data-type: %d\n", params_type );

		switch(params_type) {
		case 1:
			d_type = TYPE_INT32;
			configureIterations( params.find<int32_t>("start", 0), 
				params.find<int32_t>("step", 1),
				params.find<int32_t>("end", std::numeric_limits<int32_t>::max() ) );
			func = std::bind( &SST::Serrano::SerranoIteratorUnit::execute_int32, this );
			break;
		case 2:
			d_type = TYPE_INT64;
			configureIterations( params.find<int64_t>("start", 0), 
				params.find<int64_t>("step", 1),
				params.find<int64_t>("end", std::numeric_limits<int64_t>::max() ) );
			func = std::bind( &SST::Serrano::SerranoIteratorUnit::execute_int64, this );
			break;
		case 4:
			d_type = TYPE_FP32;
			configureIterations( params.find<float>("start", 0 ),
				params.find<float>("step", 1.0 ),
				params.find<float>("end", std::numeric_limits<float>::max() ) );
			func = std::bind( &SST::Serrano::SerranoIteratorUnit::execute_fp32, this );
			break;
		case 8:
			d_type = TYPE_FP64;
			configureIterations( params.find<double>("start", 0 ),
				params.find<double>("step", 1.0 ),
				params.find<double>("end", std::numeric_limits<double>::max() ) );
			func = std::bind( &SST::Serrano::SerranoIteratorUnit::execute_fp64, this );
			break;
		default:
			output->fatal(CALL_INFO, -1, "Error: unknown data type to process.\n");
			break;
		}
	}

	~SerranoIteratorUnit() {

	}

	virtual const char* getUnitTypeString() {
		return "ITERATOR";
	}

	virtual bool stillProcessing() {
		return keep_processing;
	}

	virtual void checkRequiredQueues( SST::Output* output ) {
		if( output_qs.size() == 0 ) {
			output->fatal(CALL_INFO, -1, "Need an output queue for an iterator to work.\n");
		}
	}

	virtual void execute( const uint64_t currentCycle ) {
		output->verbose(CALL_INFO, 8, 0, "Executing iteration generator...\n");

		if( nullptr != func ) {
			func();
		}
	}

protected:
	SerranoStandardType d_type;
	std::function<void()> func;
	void* current_value;
	void* max_value;
	void* step_value;
	bool keep_processing;	

	void execute_int32() {
		executeStep<int32_t>();
	}

	void execute_int64() {
		executeStep<int64_t>();
	}

	void execute_fp32() {
		executeStep<float>();
	}

	void execute_fp64() {
		executeStep<double>();
	}

	template<class T> void executeStep() {
		T* t_current_value   = (T*) current_value;
		T* t_max_value       = (T*) max_value;
		T* t_step_value      = (T*) step_value;

		if( (*t_current_value) < (*t_max_value) ) {
			if( ! output_qs[0]->full() ) {
				output_qs[0]->push( new SerranoMessage( sizeof(T), t_current_value ) );
				(*t_current_value) += (*t_step_value);
			}
		} else {
			output->verbose(CALL_INFO, 16, 0, "Hit the upper limit of the iteration value, processing is complete for iterator.\n");
			keep_processing = false;
		}
	}
	
	template<class T> void configureIterations( const T start, const T step, const T end ) {
		current_value       = (void*) ( new T[1] );
		max_value           = (void*) ( new T[1] );
		step_value          = (void*) ( new T[1] );

		T* t_current_value  = (T*) current_value;
		T* t_max_value      = (T*) max_value;
		T* t_step_value     = (T*) step_value;

		(*t_current_value ) = start;
		(*t_max_value     ) = end;
		(*t_step_value    ) = step;
	}

};

}
}

#endif
