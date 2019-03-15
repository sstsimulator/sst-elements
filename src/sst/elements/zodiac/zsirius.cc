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


#include "sst_config.h"
#include "zsirius.h"

#include <assert.h>

#include "sst/core/element.h"
#include "sst/core/params.h"


using namespace std;
using namespace SST;
using namespace SST::Zodiac;

ZodiacSiriusTraceReader::ZodiacSiriusTraceReader(ComponentId_t id, Params& params) :
  Component(id),
  allreduceFunctor(DerivedFunctor(this, &ZodiacSiriusTraceReader::completedAllreduceFunction)),
  barrierFunctor(DerivedFunctor(this, &ZodiacSiriusTraceReader::completedBarrierFunction)),
  finalizeFunctor(DerivedFunctor(this, &ZodiacSiriusTraceReader::completedFinalizeFunction)),
  initFunctor(DerivedFunctor(this, &ZodiacSiriusTraceReader::completedInitFunction)),
  irecvFunctor(DerivedFunctor(this, &ZodiacSiriusTraceReader::completedIrecvFunction)),
  recvFunctor(DerivedFunctor(this, &ZodiacSiriusTraceReader::completedRecvFunction)),
  retFunctor(DerivedFunctor(this, &ZodiacSiriusTraceReader::completedFunction)),
  sendFunctor(DerivedFunctor(this, &ZodiacSiriusTraceReader::completedSendFunction)),
  waitFunctor(DerivedFunctor(this, &ZodiacSiriusTraceReader::completedWaitFunction)),
  trace(NULL)
{
    scaleCompute = params.find("scalecompute", 1.0);

    string osModule = params.find<std::string>("os.module");
    assert( ! osModule.empty() );

   	Params hermesParams = params.find_prefix_params("hermesParams." );

    os = dynamic_cast<OS*>(loadSubComponent(
                            osModule, this, hermesParams));
    assert(os);

    params.print_all_params(std::cout);
    Params osParams = params.find_prefix_params("os.");
    std::string osName = osParams.find<std::string>("name");
    Params modParams = params.find_prefix_params( osName + "." );
    msgapi = dynamic_cast<MP::Interface*>(loadSubComponent(
                            "firefly.hadesMP", this, modParams));
    assert(msgapi);

    msgapi->setOS( os );

    trace_file = params.find<std::string>("trace");
    if("" == trace_file) {
        std::cerr << "Error: could not find a file contain a trace "
            "to simulate!" << std::endl;
	    exit(-1);
    } else {
        std::cout << "Trace prefix: " << trace_file << std::endl;
    }

    eventQ = new std::queue<ZodiacEvent*>();

    verbosityLevel = params.find("verbose", 0);
    std::cout << "Set verbosity level to " << verbosityLevel << std::endl;

    selfLink = configureSelfLink("Self", "1ns", 
	new Event::Handler<ZodiacSiriusTraceReader>(this, &ZodiacSiriusTraceReader::handleSelfEvent));

    tConv = Simulation::getSimulation()->getTimeLord()->getTimeConverter("1ns");

    emptyBufferSize = (uint32_t) params.find("buffer", 4096);
    emptyBuffer = (char*) malloc(sizeof(char) * emptyBufferSize);

    // Make sure we don't stop the simulation until we are ready
    registerAsPrimaryComponent();
    primaryComponentDoNotEndSim();

    // Allow the user to control verbosity from the log file.
    verbosityLevel = params.find("verbose", 2);
    zOut.init("ZSirius", (uint32_t) verbosityLevel, (uint32_t) 1, Output::STDOUT);
}

void ZodiacSiriusTraceReader::setup() {
    os->_componentSetup();
    msgapi->setup();

    rank = os->getRank();

    eventQ = new std::queue<ZodiacEvent*>();

    char trace_name[trace_file.length() + 20];
    sprintf(trace_name, "%s.%d", trace_file.c_str(), rank);

    printf("Opening trace file: %s\n", trace_name);
    trace = new SiriusReader(trace_name, rank, 64, eventQ, verbosityLevel);
    trace->setOutput(&zOut);

    int count = trace->generateNextEvents();
    std::cout << "Obtained: " << count << " events" << std::endl;

    if(eventQ->size() > 0) {
	selfLink->send(eventQ->front());
	eventQ->pop();
    }

    char logPrefix[512];
    sprintf(logPrefix, "@t:%d:ZSirius::@p:@l: ", rank);
    string logPrefixStr = logPrefix;
    zOut.setPrefix(logPrefixStr);

    // Clear counters
    zSendCount = 0;
    zRecvCount = 0;
    zIRecvCount = 0;
    zAllreduceCount = 0;
    zWaitCount = 0;
    zSendBytes = 0;
    zRecvBytes = 0;
    zIRecvBytes = 0;
    zAllreduceBytes = 0;

    nanoCompute = 0;
    nanoSend = 0;
    nanoRecv = 0;
    nanoAllreduce = 0;
    nanoBarrier = 0;
    nanoInit = 0;
    nanoFinalize = 0;
    nanoWait = 0;
    nanoIRecv = 0;

    accumulateTimeInto = &nanoCompute;
    nextEventStartTimeNano = 0;
}

void ZodiacSiriusTraceReader::init(unsigned int phase) {
	os->_componentInit(phase);
}

void ZodiacSiriusTraceReader::finish() {
    msgapi->finish();
	zOut.verbose(CALL_INFO, 1, 0, "Completed simulation at: %" PRIu64 "ns\n",
		getCurrentSimTimeNano());
	zOut.verbose(CALL_INFO, 1, 0, "Statistics for run are:\n");
	zOut.verbose(CALL_INFO, 1, 0, "- Total Send Count:           %" PRIu64 "\n", zSendCount);
	zOut.verbose(CALL_INFO, 1, 0, "- Total Recv Count:           %" PRIu64 "\n", zRecvCount);
	zOut.verbose(CALL_INFO, 1, 0, "- Total IRecv Count:          %" PRIu64 "\n", zIRecvCount);
	zOut.verbose(CALL_INFO, 1, 0, "- Total Wait Count:           %" PRIu64 "\n", zWaitCount);
	zOut.verbose(CALL_INFO, 1, 0, "- Total Allreduce Count:      %" PRIu64 "\n", zAllreduceCount);
	zOut.verbose(CALL_INFO, 1, 0, "- Total Send Bytes:           %" PRIu64 "\n", zSendBytes);
	zOut.verbose(CALL_INFO, 1, 0, "- Total Recv Bytes:           %" PRIu64 "\n", zRecvBytes);
	zOut.verbose(CALL_INFO, 1, 0, "- Total Posted-IRecv Bytes:   %" PRIu64 "\n", zIRecvBytes);
	zOut.verbose(CALL_INFO, 1, 0, "- Total Allreduce Bytes       %" PRIu64 "\n", zAllreduceBytes);
        zOut.verbose(CALL_INFO, 1, 0, "- Time spend in compute:      %" PRIu64 " ns\n", nanoCompute);
        zOut.verbose(CALL_INFO, 1, 0, "- Time spend in send:         %" PRIu64 " ns (mean: %f)\n", nanoSend,
		zSendCount > 0 ? ((double) nanoSend) / ((double) zSendCount) : 0.0);
        zOut.verbose(CALL_INFO, 1, 0, "- Time spend in recv:         %" PRIu64 " ns (mean: %f)\n", nanoRecv,
		zRecvCount > 0 ? ((double) nanoRecv) / ((double) zRecvCount) : 0.0);
        zOut.verbose(CALL_INFO, 1, 0, "- Time spend in irecv issue:  %" PRIu64 " ns\n", nanoIRecv);
        zOut.verbose(CALL_INFO, 1, 0, "- Time spend in wait:         %" PRIu64 " ns\n", nanoWait);
        zOut.verbose(CALL_INFO, 1, 0, "- Time spend in all-reduce:   %" PRIu64 " ns\n", nanoAllreduce);
        zOut.verbose(CALL_INFO, 1, 0, "- Time spend in barrier:      %" PRIu64 " ns\n", nanoBarrier);
        zOut.verbose(CALL_INFO, 1, 0, "- Time spend in init:         %" PRIu64 " ns\n", nanoInit);
        zOut.verbose(CALL_INFO, 1, 0, "- Time spend in finalize:     %" PRIu64 " ns\n", nanoFinalize);

	zOut.output("Completed at %" PRIu64 " ns\n", getCurrentSimTimeNano());
}

ZodiacSiriusTraceReader::~ZodiacSiriusTraceReader() {
    if ( trace ) {
        if(! trace->hasReachedFinalize()) {
            zOut.output("WARNING: Component did not reach a finalize event, yet the component destructor has been called.\n");
        }

        trace->close();
    }
}

ZodiacSiriusTraceReader::ZodiacSiriusTraceReader() :
    Component(-1)
{
    // for serialization only
}

void ZodiacSiriusTraceReader::handleEvent(Event* ev)
{

}

void ZodiacSiriusTraceReader::handleSelfEvent(Event* ev)
{
	// Accumulate the event time into the correct counter, then reset
	// the last event time so we can keep track
	const uint64_t sim_time_now = (uint64_t) getCurrentSimTimeNano();
        *accumulateTimeInto += ( sim_time_now - nextEventStartTimeNano );
	nextEventStartTimeNano = sim_time_now;

	// Continune onwards to process the next event
	ZodiacEvent* zEv = static_cast<ZodiacEvent*>(ev);

	zOut.verbose(__LINE__, __FILE__, "handleSelfEvent",
		3, 1, "Generating the next event...\n");

	if(zEv) {
		switch(zEv->getEventType()) {
		case Z_COMPUTE:
			handleComputeEvent(zEv);
			break;

		case Z_SEND:
			handleSendEvent(zEv);
			break;

		case Z_RECV:
			handleRecvEvent(zEv);
			break;

		case Z_IRECV:
			handleIRecvEvent(zEv);
			break;

		case Z_WAIT:
			handleWaitEvent(zEv);
			break;

		case Z_ALLREDUCE:
			handleAllreduceEvent(zEv);
			break;

		case Z_BARRIER:
			handleBarrierEvent(zEv);
			break;

		case Z_INIT:
			handleInitEvent(zEv);
			break;

		case Z_FINALIZE:
			handleFinalizeEvent(zEv);
			break;

		case Z_SKIP:
			break;

		default:
			zOut.verbose(__LINE__, __FILE__, "handleSelfEvent",
				0, 1, "Attempted to process an ZodiacEvent which is not included in the event decoding step.\n");
			exit(-1);
			break;
		}
	} else {
		zOut.verbose(__LINE__, __FILE__, "handleSelfEvent",
			0, 1, "Attempted to process an event which is not supported by the Zodiac engine.\n");
		exit(-1);
	}

	zOut.verbose(__LINE__, __FILE__, "handleSelfEvent", 0, 16,
		"Attempting to delete processed event...");
	delete zEv;
	zOut.verbose(__LINE__, __FILE__, "handleSelfEvent", 0, 16,
		"Successfully deleted event.");

	zOut.verbose(__LINE__, __FILE__, "handleSelfEvent",
		3, 1, "Finished event processing cycle.\n");
}

void ZodiacSiriusTraceReader::handleBarrierEvent(ZodiacEvent* zEv) {
	ZodiacBarrierEvent* zBEv = static_cast<ZodiacBarrierEvent*>(zEv);
	assert(zBEv);

	zOut.verbose(__LINE__, __FILE__, "handleBarrierEvent",
		2, 1, "Processing an Barrier event.\n");

	msgapi->barrier(zBEv->getCommunicatorGroup(), &barrierFunctor);
	accumulateTimeInto = &nanoBarrier;
}

void ZodiacSiriusTraceReader::handleAllreduceEvent(ZodiacEvent* zEv) {
	ZodiacAllreduceEvent* zAEv = static_cast<ZodiacAllreduceEvent*>(zEv);
	assert(zAEv);
	assert((zAEv->getLength() * 2 * 8) < emptyBufferSize);

	zOut.verbose(__LINE__, __FILE__, "handleAllreduceEvent",
		2, 1, "Processing an Allreduce event.\n");

	Hermes::MemAddr addr0( 0, emptyBuffer );
	Hermes::MemAddr addr1( 0, &emptyBuffer[zAEv->getLength()] );

	msgapi->allreduce( addr0,
		addr1,
		zAEv->getLength(),
		zAEv->getDataType(),
		zAEv->getOp(),
		zAEv->getCommunicatorGroup(), &allreduceFunctor);
	accumulateTimeInto = &nanoAllreduce;

	zAllreduceCount++;
	zAllreduceBytes += zAEv->getLength();
}

void ZodiacSiriusTraceReader::handleInitEvent(ZodiacEvent* zEv) {
	zOut.verbose(__LINE__, __FILE__, "handleInitEvent",
		2, 1, "Processing a Init event.\n");

	// Just initialize the library nothing fancy to do here
	msgapi->init(&initFunctor);
	accumulateTimeInto = &nanoInit;
}

void ZodiacSiriusTraceReader::handleFinalizeEvent(ZodiacEvent* zEv) {

	zOut.verbose(__LINE__, __FILE__, "handleFinalizeEvent",
		2, 1, "Processing a Finalize event.\n");

	// Just finalize the library nothing fancy to do here
	msgapi->fini(&finalizeFunctor);
	accumulateTimeInto = &nanoFinalize;
}

void ZodiacSiriusTraceReader::handleSendEvent(ZodiacEvent* zEv) {
	ZodiacSendEvent* zSEv = static_cast<ZodiacSendEvent*>(zEv);
	assert(zSEv);
	assert((zSEv->getLength() * 8) < emptyBufferSize);

	zOut.verbose(__LINE__, __FILE__, "handleSendEvent",
		2, 1, "Processing a Send event (length=%" PRIu32 ", tag=%d, dest=%" PRIu32 ")\n",
		zSEv->getLength(), zSEv->getMessageTag(), zSEv->getDestination());

	Hermes::MemAddr addr(0,emptyBuffer);

	msgapi->send( addr, zSEv->getLength(),
		zSEv->getDataType(), (RankID) zSEv->getDestination(),
		zSEv->getMessageTag(), zSEv->getCommunicatorGroup(), &sendFunctor);

	zSendBytes += (msgapi->sizeofDataType(zSEv->getDataType()) * zSEv->getLength());
	zSendCount++;
	accumulateTimeInto = &nanoSend;
}

void ZodiacSiriusTraceReader::handleRecvEvent(ZodiacEvent* zEv) {
	ZodiacRecvEvent* zREv = static_cast<ZodiacRecvEvent*>(zEv);
	assert(zREv);
	assert((zREv->getLength() * 8) < emptyBufferSize);

	currentRecv = (MessageResponse*) malloc(sizeof(MessageResponse));
	memset(currentRecv, 1, sizeof(MessageResponse));

	zOut.verbose(__LINE__, __FILE__, "handleRecvEvent",
		2, 1, "Processing a Recv event (length=%" PRIu32 ", tag=%d, source=%" PRIu32 ")\n",
		zREv->getLength(), zREv->getMessageTag(), zREv->getSource());

	Hermes::MemAddr addr(0,emptyBuffer);

	msgapi->recv( addr, zREv->getLength(),
		zREv->getDataType(), (RankID) zREv->getSource(),
		zREv->getMessageTag(), zREv->getCommunicatorGroup(),
		currentRecv, &recvFunctor);

	zRecvBytes += (msgapi->sizeofDataType(zREv->getDataType()) * zREv->getLength());
	zRecvCount++;
	accumulateTimeInto = &nanoRecv;
}

void ZodiacSiriusTraceReader::handleWaitEvent(ZodiacEvent* zEv) {
	ZodiacWaitEvent* zWEv = static_cast<ZodiacWaitEvent*>(zEv);
	assert(zWEv);

	// Set the current processing event so when we return we know
	// what to remove from our map.
	currentlyProcessingWaitEvent = zWEv->getRequestID();

	MessageRequest* msgReq = reqMap[zWEv->getRequestID()];
	currentRecv = (MessageResponse*) malloc(sizeof(MessageResponse));
	memset(currentRecv, 1, sizeof(MessageResponse));

	zOut.verbose(__LINE__, __FILE__, "handleWaitEvent",
		2, 1, "Processing a Wait event.\n");

	msgapi->wait( *msgReq, currentRecv, &waitFunctor);
	zWaitCount++;
	accumulateTimeInto = &nanoWait;
}

void ZodiacSiriusTraceReader::handleIRecvEvent(ZodiacEvent* zEv) {
	ZodiacIRecvEvent* zREv = static_cast<ZodiacIRecvEvent*>(zEv);
	assert(zREv);
	assert((zREv->getLength() * 8) < emptyBufferSize);

	MessageRequest* msgReq = (MessageRequest*) malloc(sizeof(MessageRequest));
	reqMap.insert(std::pair<uint64_t, MessageRequest*>(zREv->getRequestID(), msgReq));

	zOut.verbose(__LINE__, __FILE__, "handleIrecvEvent",
		2, 1, "Processing a Irecv event (length=%" PRIu32 ", tag=%d, source=%" PRIu32 ")\n",
		zREv->getLength(), zREv->getMessageTag(), zREv->getSource());

	Hermes::MemAddr addr(0,emptyBuffer);
	msgapi->irecv( addr, zREv->getLength(),
		zREv->getDataType(), (RankID) zREv->getSource(),
		zREv->getMessageTag(), zREv->getCommunicatorGroup(),
		msgReq, &irecvFunctor);

	zOut.verbose(__LINE__, __FILE__, "handleIrecvEvent",
		2, 1, "IRecv handover to Msg-API completed.\n");

	zIRecvBytes += (msgapi->sizeofDataType(zREv->getDataType()) * zREv->getLength());
	zIRecvCount++;
	accumulateTimeInto = &nanoIRecv;
}

void ZodiacSiriusTraceReader::handleComputeEvent(ZodiacEvent* zEv) {
	ZodiacComputeEvent* zCEv = static_cast<ZodiacComputeEvent*>(zEv);
	assert(zCEv);

	zOut.verbose(__LINE__, __FILE__, "handleComputeEvent",
		2, 1, "Processing a compute event (duration=%f seconds)\n",
		zCEv->getComputeDuration());

	if((0 == eventQ->size()) && (!trace->hasReachedFinalize())) {
		trace->generateNextEvents();
	}

	if(eventQ->size() > 0) {
		ZodiacEvent* nextEv = eventQ->front();
		zOut.verbose(__LINE__, __FILE__, "handleComputeEvent",
			2, 1, "Enqueuing next event at a delay of %f seconds, scaled by %f = %f seconds)\n",
			zCEv->getComputeDuration(), scaleCompute, scaleCompute * zCEv->getComputeDuration());
		eventQ->pop();
		selfLink->send(scaleCompute * zCEv->getComputeDurationNano(), tConv, nextEv);
	} else {
		zOut.output("No more events to process.\n");
		std::cout << "ZSirius: Has no more events to process" << std::endl;

		// We have run out of events
		primaryComponentOKToEndSim();
	}

	accumulateTimeInto = &nanoCompute;
}

bool ZodiacSiriusTraceReader::clockTic( Cycle_t ) {
  // return false so we keep going
  return false;
}

bool ZodiacSiriusTraceReader::completedFunction(int retVal) {
	zOut.verbose(CALL_INFO, 4, 0, "Returned from a call to the message API.\n");
	enqueueNextEvent();
	return false;
}

bool ZodiacSiriusTraceReader::completedRecvFunction(int retVal) {
	free(currentRecv);

	zOut.verbose(CALL_INFO, 4, 0, "Returned from processing a call to the recv API.\n");
	enqueueNextEvent();
	return false;
}

bool ZodiacSiriusTraceReader::completedInitFunction(int retVal) {
	zOut.verbose(CALL_INFO, 4, 0, "Returned from processing a call to the init API.\n");
	enqueueNextEvent();
	return false;
}

bool ZodiacSiriusTraceReader::completedAllreduceFunction(int retVal) {
	zOut.verbose(CALL_INFO, 4, 0, "Returned from processing a call to the allreduce API.\n");
	enqueueNextEvent();
	return false;
}

bool ZodiacSiriusTraceReader::completedSendFunction(int retVal) {
	zOut.verbose(CALL_INFO, 4, 0, "Returned from processing a call to the send API.\n");
	enqueueNextEvent();
	return false;
}

bool ZodiacSiriusTraceReader::completedFinalizeFunction(int retVal) {
	zOut.verbose(CALL_INFO, 4, 0, "Returned from processing a call to the finalize API.\n");
	enqueueNextEvent();
	return false;
}

bool ZodiacSiriusTraceReader::completedBarrierFunction(int retVal) {
	zOut.verbose(CALL_INFO, 4, 0, "Returned from processing a call to the barrier API.\n");
	enqueueNextEvent();
	return false;
}

bool ZodiacSiriusTraceReader::completedIrecvFunction(int retVal) {
	zOut.verbose(CALL_INFO, 4, 0, "Returned from processing a call to the irecv API.\n");
	enqueueNextEvent();
	return false;
}

bool ZodiacSiriusTraceReader::completedWaitFunction(int retVal) {
	zOut.verbose(CALL_INFO, 4, 0, "Returned from processing a call to the wait API.\n");

	std::map<uint64_t, MessageRequest*>::iterator req_map_itr = reqMap.find(currentlyProcessingWaitEvent);
	if(req_map_itr == reqMap.end()) {
		zOut.fatal(CALL_INFO, -1, "Error: unable to find a wait request in the ID to MessageRequest map.\n");
	} else {
		MessageRequest* done_req = req_map_itr->second;
		free(done_req);

		// Remove the request from the map and clean up
		reqMap.erase(req_map_itr);
	}

	enqueueNextEvent();
	return false;
}

void ZodiacSiriusTraceReader::enqueueNextEvent() {
	if((0 == eventQ->size()) && (!trace->hasReachedFinalize())) {
		zOut.verbose(CALL_INFO, 8, 0, "Generating next set of events from trace...\n");
		trace->generateNextEvents();
		zOut.verbose(CALL_INFO, 8, 0, "Completed generating next set of events from trace.\n");
	}

	if(eventQ->size() > 0) {
		zOut.verbose(CALL_INFO,
			8, 0, "Enqueuing next event into a self link...\n");

		ZodiacEvent* nextEv = eventQ->front();
		eventQ->pop();
		selfLink->send(nextEv);
	} else {
		zOut.verbose(CALL_INFO, 2, 0, "No more events to process, Zodiac will mark component as complete.\n");

		// We have run out of events
		primaryComponentOKToEndSim();
	}
}

