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

#include "serrano.h"

#include "sercgunit.h"
#include "serstdunit.h"
#include "seriterunit.h"
#include "serprintunit.h"

#include <limits>

using namespace SST::Serrano;

SerranoComponent::SerranoComponent( SST::ComponentId_t id, SST::Params& params ) :
	Component(id) {

	char* comp_prefix = new char[128];
	snprintf(comp_prefix, 128, "[Ser: %5d] ", (int) id);

	const int verbosity = params.find<int>("verbose", 0);
	output = new SST::Output( comp_prefix, verbosity, 0, Output::STDOUT );
	delete[] comp_prefix;

	const std::string clock = params.find<std::string>("clock", "1GHz");
	output->verbose(CALL_INFO, 2, 0, "Configuring Serrano for clock of %s...\n", clock.c_str());
	registerClock( clock, new Clock::Handler<SerranoComponent>( this, &SerranoComponent::tick ) );

	constexpr int kernel_name_len = 128;
	char* kernel_name = new char[kernel_name_len];
	for( int i = 0; i < std::numeric_limits<int>::max(); ++i ) {
		snprintf( kernel_name, kernel_name_len, "kernel%d", i);
		std::string kernel_name_file = params.find<std::string>( kernel_name, "" );

		if( "" != kernel_name_file) {
			output->verbose(CALL_INFO, 4, 0, "Found Kernel (%s): %s\n", kernel_name, kernel_name_file.c_str());
			kernel_queue.push_back( kernel_name_file );
		} else {
			break;
		}
	}
	delete[] kernel_name;

	if( kernel_queue.size() > 0 ) {
		constructGraph( output, kernel_queue.front().c_str() );
		kernel_queue.pop_front();
	}

	registerAsPrimaryComponent();
    	primaryComponentDoNotEndSim();
}

SerranoComponent::~SerranoComponent() {
	delete output;
}

bool SerranoComponent::tick( SST::Cycle_t currentCycle ) {

	output->verbose(CALL_INFO, 4, 0, "Clocking Serrano cycle %" PRIu64 "...\n", currentCycle );

	// Tick all units
	for( auto next_unit : units ) {
		next_unit.second->execute( currentCycle );
	}

	bool units_continue  = false;
	bool queues_continue = false;

	// Do we have any units which want to continue processing
	for( auto next_unit : units ) {
		output->verbose(CALL_INFO, 16, 0, "Unit-ID: %" PRIu64 " status: %s\n", next_unit.first,
			( next_unit.second->stillProcessing() ? "keep-processing" : "completed" ) );
		units_continue |= next_unit.second->stillProcessing();
	}

	// Check that any queue is not empty
	for( auto next_q : msg_queues ) {
		queues_continue |= ( ! next_q.second->empty() );
	}

	if( units_continue ) {
		output->verbose(CALL_INFO, 4, 0, "Work units are still processing, continue for another cycle\n");
		return false;
	} else {
		if( queues_continue ) {
			output->verbose(CALL_INFO, 4, 0, "Queues contain entries that may need processing, continue for another cycle.\n");
			return false;
		} else {
			output->verbose(CALL_INFO, 4, 0, "Neither queues or units have no work, no need to continue processing.\n");
			primaryComponentOKToEndSim();
			return true;
		}
	}

}

void SerranoComponent::constructGraph( SST::Output* output, const char* kernel_file ) {
	output->verbose(CALL_INFO, 4, 0, "Parsing kernel at: %s...\n", kernel_file);
	FILE* graph_file = fopen( kernel_file, "rt" );

	if( nullptr == graph_file ) {
		output->fatal(CALL_INFO, -1, "Error: unable to open file: %s\n", kernel_file );
	}

	constexpr int buff_max = 1024;
	char* line = new char[buff_max];
	int index = 0;

	Params empty_params;

	while( ! feof( graph_file ) ) {
		read_line( graph_file, line, buff_max);
		printf("Line[%s]\n", line);

		if( ( 0 == strcmp( line, "" ) ) || ( line[0] == '#') ) {
			continue;
		}

		char* token   = strtok( line, " " );
		char* item_id = strtok( nullptr, " " );
		const uint64_t id   = std::atoll( item_id );

		if( 0 == strcmp( token, "NODE" ) ) {
			char* unit_type      = strtok( nullptr, " " );
			char* unit_data_type = strtok( nullptr, " " );

			int iterator_type = 0;
			SerranoStandardType node_op_type;

			if( 0 == strcmp( unit_data_type, "INT32" ) ) {
				node_op_type = TYPE_INT32;
				iterator_type = 1;
			} else if( 0 == strcmp( unit_data_type, "INT64" ) ) {
				node_op_type = TYPE_INT64;
				iterator_type = 2;
			} else if( 0 == strcmp( unit_data_type, "FP32" ) ) {
				node_op_type = TYPE_FP32;
				iterator_type = 4;
			} else if( 0 == strcmp( unit_data_type, "FP64" ) ) {
				node_op_type = TYPE_FP64;
				iterator_type = 8;
			}

			Params unit_params;

			char* verbose_param = new char[16];
			snprintf( verbose_param, 16, "%d", output->getVerboseLevel() );
			unit_params.insert( "verbose", verbose_param );
			delete[] verbose_param;

			char* param_name = strtok( nullptr, " " );
			while( nullptr != param_name ) {
				char* value = strtok( nullptr, " " );
				output->verbose(CALL_INFO, 4, 0, "param: %s=%s\n", param_name, value);
				unit_params.insert( param_name, value );
				param_name = strtok( nullptr, " ");
			}

			SerranoCoarseUnit* new_unit = nullptr;

			output->verbose(CALL_INFO, 4, 0, "Creating a new coarse unit type: %s\n", unit_type);

			char* dtype_str = new char[16];
			snprintf( dtype_str, 16, "%d", iterator_type );
			unit_params.insert( "data_type", dtype_str );
			delete[] dtype_str;

			if( 0 == strcmp( unit_type, "ITERATOR" ) ) {
				new_unit = loadAnonymousSubComponent<SerranoCoarseUnit>( "serrano.SerranoIteratorUnit", "slot", 0, ComponentInfo::SHARE_NONE, unit_params );
			} else if( 0 == strcmp( unit_type, "ADD" ) ) {
				new_unit = loadAnonymousSubComponent<SerranoCoarseUnit>( "serrano.SerranoBasicUnit", "slot", 0, ComponentInfo::SHARE_NONE, unit_params );
				SerranoBasicUnit* new_unit_basic = (SerranoBasicUnit*) new_unit;
				new_unit_basic->configureFunction( output, OP_ADD, node_op_type );
			} else if( 0 == strcmp( unit_type, "SUB" ) ) {
				new_unit = loadAnonymousSubComponent<SerranoCoarseUnit>( "serrano.SerranoBasicUnit", "slot", 0, ComponentInfo::SHARE_NONE, unit_params );
				SerranoBasicUnit* new_unit_basic = (SerranoBasicUnit*) new_unit;
				new_unit_basic->configureFunction( output, OP_SUB, node_op_type );
			} else if( 0 == strcmp( unit_type, "PRINTER" ) ) {
				new_unit = loadAnonymousSubComponent<SerranoCoarseUnit>( "serrano.SerranoPrinterUnit", "slot", 0, ComponentInfo::SHARE_NONE, unit_params );
			} else {
				output->fatal(CALL_INFO, -1, "Error: unable to parse node type (%s)\n", token );
			}

			units.insert( std::pair< uint64_t, SerranoCoarseUnit* >( id, new_unit ) );
		} else if( 0 == strcmp( token, "LINK" ) ) {
			char* in_unit      = strtok( nullptr, " " );
			char* out_unit     = strtok( nullptr, " " );
		
			const uint64_t u64_in_unit  = std::atoll( in_unit );
			const uint64_t u64_out_unit = std::atoll( out_unit );

			SerranoCircularQueue<SerranoMessage*>* new_q = new SerranoCircularQueue<SerranoMessage*>(2);
	
			if( ( units.find( u64_in_unit ) != units.end() ) && ( units.find( u64_out_unit ) != units.end() ) ) {
				output->verbose(CALL_INFO, 4, 0, "Connecting %" PRIu64 " -> %" PRIu64 " (link-id: %" PRIu64 ")\n",
					u64_in_unit, u64_out_unit, id);
				// These are swapped, input to the link is the output of a unit and vice versa
				units[ u64_in_unit  ]->addOutputQueue( new_q );
				units[ u64_out_unit ]->addInputQueue( new_q );
			} else {
				output->fatal(CALL_INFO, -1, "Error: link does not connect an existing input or output component.\n");
			}
		} else {

		}
	}

	delete[] line;
	fclose( graph_file );

	/* cycle over and check queues are good, these will fatal */
	for( auto next_unit : units ) {
		next_unit.second->checkRequiredQueues( output );
	}	
}

int SerranoComponent::read_line( FILE* file_h, char* buffer, const size_t buffer_max ) {
	int status = 0;
	int index  = 0;
	bool keep_looping = true;

	while( keep_looping ) {
		char nxt_c = (char) fgetc( file_h );

		switch( nxt_c ) {
		case EOF:
			keep_looping = false;
			break;
		case '\n':
			keep_looping = false;
			break;
		default:
			buffer[index++] = nxt_c;
			break;
		}
	}

	buffer[index] = '\0';
	return status;
}

void SerranoComponent::clearGraph() {
	output->verbose(CALL_INFO, 2, 0, "Clearing current graph...\n");

	for( auto next_q : msg_queues ) {
		delete next_q.second;;
	}

	msg_queues.clear();

	for( auto next_unit : units ) {
		delete next_unit.second;
	}

	units.clear();

	output->verbose(CALL_INFO, 2, 0, "Graph clear done. Reset is complete\n");
}
