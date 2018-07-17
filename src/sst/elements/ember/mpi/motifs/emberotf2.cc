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

static OTF2_CallbackCode EmberOTF2StartProgram(
	OTF2_LocationRef locationID,
	OTF2_TimeStamp time,
	void *userData,
	OTF2_AttributeList *attributeList,
	OTF2_StringRef programName,
	uint32_t numberOfArguments,
	const OTF2_StringRef *programArguments) {

	EmberOTF2Generator* gen = (EmberOTF2Generator*) userData;
	gen->setCurrentTime( time );

	return OTF2_CALLBACK_SUCCESS;
}

static OTF2_CallbackCode EmberOTF2EndProgram(
	OTF2_LocationRef locationID,
	OTF2_TimeStamp time,
	void *userData,
	OTF2_AttributeList *attributeList,
	int64_t exitStatus
	) {

	return OTF2_CALLBACK_SUCCESS;
}

static OTF2_CallbackCode EmberOTF2EnterRegion( 
	OTF2_LocationRef location,
	OTF2_TimeStamp time, 
	void* userData,
	OTF2_AttributeList* attributes,
	OTF2_RegionRef region ) {

	EmberOTF2Generator* gen = (EmberOTF2Generator*) userData;

	if( gen->getCurrentTime() == 0 ) {
		gen->setCurrentTime( time );
	}

	return OTF2_CALLBACK_SUCCESS;
}

static OTF2_CallbackCode EmberOTF2LeaveRegion(
	OTF2_LocationRef location,
	OTF2_TimeStamp time,
	void* userData,
	OTF2_AttributeList* attributes,
	OTF2_RegionRef regon ) {

	EmberOTF2Generator* gen = (EmberOTF2Generator*) userData;

	OTF2_TimeStamp timeDiff = time - gen->getCurrentTime();
	gen->verbose( CALL_INFO, 4, 0, "Time-Difference: %" PRIu64 "\n", timeDiff );
	gen->setCurrentTime( time );

	return OTF2_CALLBACK_SUCCESS;
}

static OTF2_CallbackCode EmberOTF2MPISend(
	OTF2_LocationRef location,
	OTF2_TimeStamp time,
	void* userData,
	OTF2_AttributeList* attributes,
	uint32_t receiver,
	OTF2_CommRef communicator,
	uint32_t tag,
	uint64_t msgLen) {

	EmberOTF2Generator* gen = (EmberOTF2Generator*) userData;
	gen->enQ_send( *(gen->getEventQueue()), 0, msgLen, gen->extractDataTypeFromAttributeList(attributes), receiver, tag, GroupWorld );

	return OTF2_CALLBACK_SUCCESS;
}

static OTF2_CallbackCode EmberOTF2MPIISend(
       	OTF2_LocationRef location,
        OTF2_TimeStamp time,
        void* userData,
        OTF2_AttributeList* attributes,
        uint32_t receiver,
        OTF2_CommRef communicator,
        uint32_t tag,
        uint64_t msgLen,
	uint64_t reqID) {

	MessageRequest* newReq = new MessageRequest();

	EmberOTF2Generator* gen = (EmberOTF2Generator*) userData;
	gen->enQ_isend( *(gen->getEventQueue()), 0, msgLen, gen->extractDataTypeFromAttributeList(attributes), receiver, tag, GroupWorld, newReq );

	gen->getRequestMap().insert( std::pair<uint64_t, MessageRequest*>( reqID, newReq ) );

        return OTF2_CALLBACK_SUCCESS;
}

static OTF2_CallbackCode EmberOTF2MPIISendComplete(
	OTF2_LocationRef location,
       	OTF2_TimeStamp time,
       	void* userData,
       	OTF2_AttributeList* attributes,
	uint64_t reqID) {

	EmberOTF2Generator* gen = (EmberOTF2Generator*) userData;
	gen->verbose( CALL_INFO, 4, 0, "Trace::ISendComplete, reqID=%" PRIu64 "\n", reqID );

	auto checkReq = gen->getRequestMap().find(reqID);

	if( checkReq == gen->getRequestMap().end() ) {
		gen->printRequestMap();
		gen->fatal( CALL_INFO, -1, "Error: search for request ID %" PRIu64 " did not find a request.\n",
			reqID);
	}

	gen->enQ_wait( *(gen->getEventQueue()), checkReq->second, NULL );
	gen->getRequestMap().erase( checkReq );

	return OTF2_CALLBACK_SUCCESS;
}

static OTF2_CallbackCode EmberOTF2MPIIRecvRequest(
	OTF2_LocationRef location,
        OTF2_TimeStamp time,
        void* userData,
        OTF2_AttributeList* attributes,
        uint64_t reqID) {

/*
	EmberOTF2Generator* gen = (EmberOTF2Generator*) userData;
	gen->verbose( CALL_INFO, 4, 0, "Trace::IRecvRequest, reqID=%" PRIu64 "\n", reqID );

	auto checkReq = gen->getRequestMap().find(reqID);

	if( checkReq == gen->getRequestMap().end() ) {
		gen->printRequestMap();
		gen->fatal( CALL_INFO, -1, "Error: search for request ID %" PRIu64 " did not find a request.\n",
			reqID);
	}

	gen->enQ_wait( *(gen->getEventQueue()), checkReq->second, NULL );
	gen->getRequestMap().erase( checkReq );
*/
        return OTF2_CALLBACK_SUCCESS;
}

static OTF2_CallbackCode EmberOTF2MPIRecv(
	OTF2_LocationRef location,
        OTF2_TimeStamp time,
        void* userData,
        OTF2_AttributeList* attributes,
        uint32_t sender,
        OTF2_CommRef communicator,
        uint32_t tag,
        uint64_t msgLen) {

       	EmberOTF2Generator* gen = (EmberOTF2Generator*) userData;
	gen->verbose( CALL_INFO, 4, 0, "Trace::Recv sender=%" PRIu32 ", tag=%" PRIu32 ", len=%" PRIu64 "\n",
		sender, tag, msgLen);

        gen->enQ_recv( *(gen->getEventQueue()), 0, msgLen, gen->extractDataTypeFromAttributeList(attributes), sender, tag, GroupWorld );

	return OTF2_CALLBACK_SUCCESS;
}

static OTF2_CallbackCode EmberOTF2MPIIRecv(
	OTF2_LocationRef location,
        OTF2_TimeStamp time,
        void* userData,
        OTF2_AttributeList* attributes,
        uint32_t sender,
        OTF2_CommRef communicator,
        uint32_t tag,
        uint64_t msgLen,
	uint64_t reqID) {

/*
       	EmberOTF2Generator* gen = (EmberOTF2Generator*) userData;
	gen->verbose( CALL_INFO, 4, 0, "Trace::IRecv sender=%" PRIu32 ", tag=%" PRIu32 ", len=%" PRIu64 ", reqID=%" PRIu64 "\n",
		sender, tag, msgLen, reqID);

	MessageRequest* newReq = new MessageRequest();

        gen->enQ_irecv( *(gen->getEventQueue()), 0, msgLen, gen->extractDataTypeFromAttributeList(attributes), sender, tag, GroupWorld, newReq );

        gen->getRequestMap().insert( std::pair<uint64_t, MessageRequest*>( reqID, newReq ) );	
*/

       	EmberOTF2Generator* gen = (EmberOTF2Generator*) userData;
	gen->verbose( CALL_INFO, 4, 0, "Trace::Recv sender=%" PRIu32 ", tag=%" PRIu32 ", len=%" PRIu64 "\n",
		sender, tag, msgLen);

        gen->enQ_recv( *(gen->getEventQueue()), 0, msgLen, gen->extractDataTypeFromAttributeList(attributes), sender, tag, GroupWorld );

        return OTF2_CALLBACK_SUCCESS;
}

static OTF2_CallbackCode EmberOTF2MPICollectiveBegin(
		OTF2_LocationRef location,
		OTF2_TimeStamp time,
		void* userData,
		OTF2_AttributeList* attributes) {

	return OTF2_CALLBACK_SUCCESS;
}

static OTF2_CallbackCode EmberOTF2MPICollectiveEnd(
		OTF2_LocationRef location,
		OTF2_TimeStamp time,
		void *userData,
		OTF2_AttributeList *attributeList,
		OTF2_CollectiveOp collectiveOp,
		OTF2_CommRef communicator,
		uint32_t root,
		uint64_t sizeSent,
		uint64_t sizeReceived) {

	EmberOTF2Generator* gen = (EmberOTF2Generator*) userData;
	gen->verbose( CALL_INFO, 4, 0, "Trace::CollectiveEnd (Triggers Start in Ember) from Root=%" PRIu32 "\n",
		root );

	switch( collectiveOp ) {
	case OTF2_COLLECTIVE_OP_BARRIER:
		gen->verbose( CALL_INFO, 4, 0, "Trace::Barrier\n");
		gen->enQ_barrier( *(gen->getEventQueue()), GroupWorld );
		break;

	case OTF2_COLLECTIVE_OP_BCAST:
		gen->verbose( CALL_INFO, 4, 0, "Trace::Bcast, Root=%" PRIu32 ", Size=%" PRIu64 "\n", root, sizeSent );
		gen->enQ_bcast( *(gen->getEventQueue()), 0, sizeSent, CHAR, static_cast<int>(root),
			GroupWorld);
		break;

	case OTF2_COLLECTIVE_OP_ALLREDUCE:
		gen->verbose( CALL_INFO, 4, 0, "Trace::Allreduce, Root=%" PRIu32 ", Size=%" PRIu64 "\n",
			root, sizeSent);
		gen->enQ_allreduce( *(gen->getEventQueue()), 0, 0, sizeSent, CHAR, SUM, GroupWorld );
		break;

	case OTF2_COLLECTIVE_OP_REDUCE:
		gen->verbose( CALL_INFO, 4, 0, "Trace::Reduce, Root=%" PRIu32 ", Size=%" PRIu64 "\n",
			root, sizeSent);
		gen->enQ_reduce( *(gen->getEventQueue()), 0, 0, sizeSent, CHAR, SUM, static_cast<int>(root),
			GroupWorld);
		break;

	default:
		gen->verbose( CALL_INFO, 16, 0, "Not supported collective operation, set to ignore event.\n" );
		break;
	}

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
	traceLocationCount(0), currentLocation(0), currentTime(0)
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

	verbose( CALL_INFO, 1, 0, "Registering callbacks...\n" );

	traceGlobalEvtCallbacks = OTF2_GlobalEvtReaderCallbacks_New();

	OTF2_GlobalEvtReaderCallbacks_SetProgramBeginCallback( traceGlobalEvtCallbacks, EmberOTF2StartProgram );
	OTF2_GlobalEvtReaderCallbacks_SetProgramEndCallback( traceGlobalEvtCallbacks, EmberOTF2EndProgram );
	OTF2_GlobalEvtReaderCallbacks_SetMpiSendCallback( traceGlobalEvtCallbacks, EmberOTF2MPISend );
	OTF2_GlobalEvtReaderCallbacks_SetMpiRecvCallback( traceGlobalEvtCallbacks, EmberOTF2MPIRecv );
	OTF2_GlobalEvtReaderCallbacks_SetMpiIsendCallback( traceGlobalEvtCallbacks, EmberOTF2MPIISend );
	OTF2_GlobalEvtReaderCallbacks_SetMpiIrecvCallback( traceGlobalEvtCallbacks, EmberOTF2MPIIRecv );
	OTF2_GlobalEvtReaderCallbacks_SetMpiIrecvRequestCallback( traceGlobalEvtCallbacks, EmberOTF2MPIIRecvRequest );
	OTF2_GlobalEvtReaderCallbacks_SetMpiIsendCompleteCallback( traceGlobalEvtCallbacks, EmberOTF2MPIISendComplete );
	OTF2_GlobalEvtReaderCallbacks_SetEnterCallback( traceGlobalEvtCallbacks, EmberOTF2EnterRegion );
	OTF2_GlobalEvtReaderCallbacks_SetLeaveCallback( traceGlobalEvtCallbacks, EmberOTF2LeaveRegion );
	OTF2_GlobalEvtReaderCallbacks_SetMpiCollectiveEndCallback( traceGlobalEvtCallbacks, EmberOTF2MPICollectiveEnd );

	traceGlobalEvtReader = OTF2_Reader_GetGlobalEvtReader( traceReader );

	OTF2_GlobalEvtReader_SetCallbacks( traceGlobalEvtReader, traceGlobalEvtCallbacks, this );
}

bool EmberOTF2Generator::generate( std::queue<EmberEvent*>& evQ ) {
	setEventQueue( &evQ );

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
