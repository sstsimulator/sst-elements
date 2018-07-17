// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2018, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

/*
        OTF2_GlobalDefReader* traceGlobalDefReader;
        OTF2_GlobalDefReaderCallbacks* traceGlobalCallbacks;
     	OTF2_Reader* traceReader;
*/

#include <sst_config.h>

#include "emberotf2.h"

using namespace SST::Ember;

static OTF2_CallbackCode EmberOTF2EnterRegion( OTF2_LocationRef location,
	OTF2_TimeStamp time, 
	uint64_t eventPosition,
	void* userData,
	OTF2_AttributeList* attributes,
	OTF2_RegionRef region ) {

	printf("EVENT LOCATION: %d ENTER REGION\n", (int) location );

	return OTF2_CALLBACK_SUCCESS;
}

static OTF2_CallbackCode EmberOTF2ExitRegion( OTF2_LocationRef location,
	OTF2_TimeStamp time,
	uint64_t eventPosition,
	void* userData,
	OTF2_AttributeList* attributes,
	OTF2_RegionRef regon ) {

	printf("EVENT LOCATION %d EXIT REGION\n", (int) location );

	return OTF2_CALLBACK_SUCCESS;
}

static OTF2_CallbackCode EmberOTF2MPISend( OTF2_LocationRef location,
	OTF2_TimeStamp time,
	uint64_t eventPosition,
	void* userData,
	OTF2_AttributeList* attributes,
	uint32_t receiver,
	OTF2_CommRef communicator,
	uint32_t tag,
	uint64_t msgLen) {

	printf("MPI-SEND: to: %" PRIu32 ", tag=%" PRIu32 ", len=%" PRIu64 "\n",
		receiver, tag, msgLen);

	return OTF2_CALLBACK_SUCCESS;
}

static OTF2_CallbackCode EmberOTF2RegisterLocation( void* userData,
	OTF2_LocationRef location, OTF2_StringRef locationName,
	OTF2_LocationType locationType, uint64_t locationEvents,
	OTF2_LocationGroupRef locationGroup ) {

	std::vector<uint64_t>* locationRefs = (std::vector<uint64_t>*) (userData);
	locationRefs->push_back( static_cast<uint64_t>( location ) );

	return OTF2_CALLBACK_SUCCESS;
}

EmberOTF2Generator::EmberOTF2Generator(SST::Component* owner, Params& params) :
	EmberMessagePassingGenerator(owner, params, "OTF2"),
	traceLocationCount(0), currentLocation(0)
{
	std::string tracePrefix = params.find<std::string>("arg.tracePrefix", "");

	if( "" == tracePrefix ) {
		fatal( CALL_INFO, -1, "Error: no trace was specified by the \"tracePrefix\" parameter.\n" );
	}

	verbose( CALL_INFO, 2, 0, "Opening: %s as trace prefix...\n", tracePrefix.c_str() );

	traceReader = OTF2_Reader_Open( tracePrefix.c_str() );

	if( NULL == traceReader ) {
		fatal( CALL_INFO, -1, "Error: unable to load trace at: %s\n", tracePrefix.c_str() );
	} else {
		verbose( CALL_INFO, 2, 0, "Trace open was successful.\n" );
	}

	OTF2_Reader_SetSerialCollectiveCallbacks( traceReader );

	OTF2_Reader_GetNumberOfLocations( traceReader, &traceLocationCount );
	verbose( CALL_INFO, 2, 0, "Found %" PRIu64 " locations, motif is rank %" PRIu64 ".\n", traceLocationCount,
		static_cast<uint64_t>(rank()) );

	if( static_cast<uint64_t>(size()) != traceLocationCount ) {
		verbose(CALL_INFO, 1, 0, "Error: trace contains %" PRIu64 " locations, but the simulation has %" PRIu64 " ranks.\n",
			traceLocationCount, static_cast<uint64_t>(size()));
	}

	// Open up all the event files
	// OTF2_Reader_OpenEvtFiles( traceReader );

	// Get my local definition reader
	// traceLocalDefReader = OTF2_Reader_GetDefReader( traceReader, static_cast<OTF2_LocationRef>( rank() ) );
	// traceLocalCallbacks = OTF2_DefReaderCallbacks_New();

/*
	OTF2_DefReader

	OTF2_GlobalDefReaderCallbacks_SetLocationCallback( traceGlobalCallbacks, &EmberOTF2RegisterLocation );
	OTF2_Reader_RegisterGlobalDefCallbacks( traceReader, traceGlobalDefReader, traceGlobalCallbacks, &traceLocations );

	if( OTF2_SUCCESS != OTF2_Reader_OpenDefFiles( traceReader) ) {
		fatal(CALL_INFO, -1, "Error - OTF2 is unable to open definition files.\n");
	}

	OTF2_Reader_OpenEvtFiles( traceReader );

	for( int i = 0; i < traceLocations.size(); ++i ) {
		verbose(CALL_INFO, 2, 0, "Processing location %" PRIu64 "...\n", currentLocation );

		OTF2_DefReader* traceDefReader = OTF2_Reader_GetDefReader( traceReader, traceLocations[i] );

		if( NULL != traceDefReader ) {
			uint64_t defRead = 0;
			OTF2_Reader_ReadAllLocalDefinitions( traceReader, traceDefReader, &defRead );
			OTF2_Reader_CloseDefReader( traceReader, traceDefReader );
		}

		OTF2_EvtReader* traceEventReader = OTF2_Reader_GetEvtReader( traceReader, traceLocations[i] );
	}

	OTF2_Reader_CloseDefFiles( traceReader );

	traceEvtReader = OTF2_Reader_GetGlobalEvtReader( traceReader );

	OTF2_GlobalEvtReaderCallbacks* traceGlobalEvtCallbacks = OTF2_GlobalEvtReaderCallbacks_New();
	OTF2_Reader_RegisterGlobalEvtCallbacks( traceReader, traceEvtReader, traceGlobalEvtCallbacks, NULL );

	OTF2_GlobalEvtReaderCallbacks_Delete( traceGlobalEvtCallbacks );
*/

	uint64_t globalDefinitions = 0;
	OTF2_Reader_GetNumberOfGlobalDefinitions( traceReader, &globalDefinitions );
	verbose( CALL_INFO, 1, 0, "There are: %" PRIu64 " global definitions.\n", globalDefinitions );

	traceGlobalDefReader = OTF2_Reader_GetGlobalDefReader( traceReader );

	if( NULL == traceGlobalDefReader ) {
		fatal( CALL_INFO, -1, "Error: unable to create a global definition reader.\n");
	}

	uint64_t globalDefinitionsRead = 0;
	OTF2_Reader_ReadAllGlobalDefinitions( traceReader, traceGlobalDefReader, &globalDefinitionsRead );
	verbose( CALL_INFO, 1, 0, "Read %" PRIu64 " global definitions from trace on rank %" PRIu64 "\n",
		globalDefinitionsRead, rank() );

	traceOpenedDefFiles = OTF2_Reader_OpenDefFiles( traceReader );

	if( traceOpenedDefFiles ) {
		verbose( CALL_INFO, 2, 0, "Successfully opened event files.\n");
	} else {
		verbose( CALL_INFO, 2, 0, "Did not open event files, this is optional so will ignore.\n" );
	}

	if( OTF2_SUCCESS != OTF2_Reader_OpenEvtFiles( traceReader ) ) {
		fatal( CALL_INFO, -1, "Error: unable to open event files.\n");
	}

	// Select only the location of the rank represented by this motif
	if( OTF2_SUCCESS != OTF2_Reader_SelectLocation( traceReader, static_cast<uint64_t>(rank()) ) ) {
		fatal( CALL_INFO, -1, "Error: unable to select location %" PRIu64 "\n", rank() );
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////

	if( traceOpenedDefFiles ) {
		traceLocalDefReader = OTF2_Reader_GetDefReader( traceReader, static_cast<uint64_t>(rank()) );

		if( NULL == traceLocalDefReader ) {
			fatal( CALL_INFO, -1, "Error: unable to open a local definition reader on rank %" PRIu64 "\n", rank() );
		} else {
			verbose( CALL_INFO, 1, 0, "Opened local definition reader on rank %" PRIu64 "\n", rank() );
		}

		uint64_t localDefs = 0;
		OTF2_Reader_ReadAllLocalDefinitions( traceReader, traceLocalDefReader, &localDefs );

		OTF2_Reader_CloseDefReader( traceReader, traceLocalDefReader );
	}

	traceEvtReader = OTF2_Reader_GetEvtReader( traceReader, static_cast<uint64_t>( rank() ) );

	if( NULL == traceEvtReader ) {
		fatal( CALL_INFO, -1, "Error: unable to open a local trace event reader.\n" );
	}

	if( traceOpenedDefFiles ) {
		OTF2_Reader_CloseDefFiles( traceReader );
	}

/*
	verbose(CALL_INFO, 1, 0, "Read %" PRIu64 " local definitions.\n", localDefs);
	traceEvtReader = OTF2_Reader_GetEvtReader( traceReader, static_cast<OTF2_LocationRef>(rank()) );

	if( NULL == traceEvtReader ) {
		fatal( CALL_INFO, -1, "Error: unable to successfully create a local event reader. Call returned NULL.\n" );
	}

	traceLocalEvtCallbacks = OTF2_EvtReaderCallbacks_New();

	if( OTF2_SUCCESS != OTF2_EvtReaderCallbacks_SetEnterCallback( traceLocalEvtCallbacks, EmberOTF2EnterRegion ) ) {
		fatal( CALL_INFO, -1, "Unable to register callback for enter region.\n" );
	}

	if( OTF2_SUCCESS != OTF2_EvtReaderCallbacks_SetLeaveCallback( traceLocalEvtCallbacks, EmberOTF2ExitRegion ) ) {
		fatal( CALL_INFO, -1, "Unable to register callback for leave region.\n" );
	}

	if( OTF2_SUCCESS != OTF2_EvtReaderCallbacks_SetMpiSendCallback( traceLocalEvtCallbacks, EmberOTF2MPISend ) ) {
		fatal( CALL_INFO, -1, "Unable to register callback for MPI_Send\n");
	}

	if( OTF2_SUCCESS != OTF2_EvtReader_SetCallbacks( traceEvtReader, traceLocalEvtCallbacks, NULL ) ) {
		fatal( CALL_INFO, -1, "Unable to register callbacks connected to trace reader on rank %" PRIu64 "\n", rank() );
	}

	verbose( CALL_INFO, 2, 0, "Successfully initialized OTF2 reader for rank %" PRIu64 ".\n", static_cast<uint64_t>(rank()) );
*/

	traceGlobalEvtReader = OTF2_Reader_GetGlobalEvtReader( traceReader );
}

bool EmberOTF2Generator::generate( std::queue<EmberEvent*>& evQ ) {
	uint64_t eventsRead = 0;

//	OTF2_Reader_ReadAllGlobalEvents( traceReader, traceGlobalEvtReader, &eventsRead );
//	printf("READ %d EVENTS!\n", (int) eventsRead );	

//	if( OTF2_SUCCESS != OTF2_Reader_ReadGlobalEvents( traceReader, traceGlobalEvtReader, 1, &eventsRead ) ) {
//		fatal(CALL_INFO, -1, "Error reading an event from the trace.\n" );
//	} else {
//		printf("read %d events...\n", (int) eventsRead );
//	}

//	uint64_t total = 0;
//
//	while( OTF2_Reader_ReadGlobalEvents( traceReader, traceGlobalEvtReader, 1, &eventsRead) == OTF2_SUCCESS ) {
//		printf("Reaed %" PRIu64 " events on rank %" PRIu64 ", total=%" PRIu64 "\n", eventsRead, rank(), total );
//		total += eventsRead;
//
//		if( eventsRead == 0 ) {
//			break;
//		}
//	}

	int hasEvent = 0;
	OTF2_Reader_HasGlobalEvent( traceReader, traceGlobalEvtReader, &hasEvent );

	if( hasEvent ) {
		verbose( CALL_INFO, 4, 0, "Rank %" PRIu64 " has events to process, begin trace read...\n", rank() );

		while( (hasEvent != 0) && (evQ.size() == 0) ) {
			if( OTF2_SUCCESS != OTF2_Reader_ReadGlobalEvents( traceReader, traceGlobalEvtReader, 1, &eventsRead ) ) {
				fatal( CALL_INFO, -1, "Error reading global event on rank %" PRIu64 "\n", rank() );
			} else {
				verbose( CALL_INFO, 4, 0, "Read %" PRIu64 " events from trace on rank: %" PRIu64 "\n",
					eventsRead, rank() );
			}

			OTF2_Reader_HasGlobalEvent( traceReader, traceGlobalEvtReader, &hasEvent );
		}

		verbose( CALL_INFO, 4, 0, "Rank %" PRIu64 " has completed its read period from trace.\n", rank() );

		return false;
	} else {
		verbose( CALL_INFO, 4, 0, "Rank %" PRIu64 " has no events to read in global trace. Motif will mark complete.\n", rank() );
		return true;
	}

//	while( ! (OTF2_SUCCESS == OTF2_Reader_ReadLocalEvents( traceReader, traceEvtReader, 1, &eventsRead ) ) ) {
//		printf("read %d events.\n", (int) eventsRead );
//	}

//	if( OTF2_SUCCESS != OTF2_Reader_ReadLocalEvents( traceReader, traceEvtReader, 1, &eventsRead ) ) {
//		fatal( CALL_INFO, -1, "Error - unable to read next event from trace on rank %" PRIu64 "\n", static_cast<uint64_t>(rank()) );
//	} else {
//		verbose( CALL_INFO, 4, 0, "Successfully read %" PRIu64 " events.\n", eventsRead );
//	}

//	return false;
}

EmberOTF2Generator::~EmberOTF2Generator() {
	verbose( CALL_INFO, 1, 0, "Completed generator, closing down trace handlers." );

//	OTF2_Reader_CloseGlobalEvtReader( traceReader, traceEvtReader );
	OTF2_Reader_CloseEvtFiles( traceReader );
	OTF2_Reader_Close( traceReader );

	traceLocations.clear();
}
