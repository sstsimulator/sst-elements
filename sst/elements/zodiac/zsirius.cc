
#include "sst_config.h"
#include "sst/core/serialization.h"
#include "zsirius.h"

#include <assert.h>

#include "sst/core/element.h"
#include "sst/core/params.h"


using namespace std;
using namespace SST;
using namespace SST::Zodiac;

ZodiacSiriusTraceReader::ZodiacSiriusTraceReader(ComponentId_t id, Params& params) :
  Component(id),
  retFunctor(DerivedFunctor(this, &ZodiacSiriusTraceReader::completedFunction)),
  recvFunctor(DerivedFunctor(this, &ZodiacSiriusTraceReader::completedRecvFunction))
{

    string msgiface = params.find_string("msgapi");
    scaleCompute = params.find_floating("scalecompute", 1.0);

    if ( msgiface == "" ) {
        msgapi = new MessageInterface();
    } else {
	// Took code from Mike, Scott is going to fix.
	std::ostringstream ownerName;
    	ownerName << this;
    	Params hermesParams = params.find_prefix_params("hermesParams." );
    	hermesParams.insert(
        	std::pair<std::string,std::string>("owner", ownerName.str()));

	msgapi = dynamic_cast<MessageInterface*>(loadModule(msgiface, hermesParams));

        if(NULL == msgapi) {
		std::cerr << "Message API: " << msgiface << " could not be loaded." << std::endl;
		exit(-1);
        }
    }

    trace_file = params.find_string("trace");
    if("" == trace_file) {
	std::cerr << "Error: could not find a file contain a trace to simulate!" << std::endl;
	exit(-1);
    } else {
	std::cout << "Trace prefix: " << trace_file << std::endl;
     }

    eventQ = new std::queue<ZodiacEvent*>();

    verbosityLevel = params.find_integer("verbose", 0);
    std::cout << "Set verbosity level to " << verbosityLevel << std::endl;

    selfLink = configureSelfLink("Self", "1ns", 
	new Event::Handler<ZodiacSiriusTraceReader>(this, &ZodiacSiriusTraceReader::handleSelfEvent));

    tConv = Simulation::getSimulation()->getTimeLord()->getTimeConverter("1ns");

    emptyBufferSize = (uint32_t) params.find_integer("buffer", 4096);
    emptyBuffer = (char*) malloc(sizeof(char) * emptyBufferSize);

    // Make sure we don't stop the simulation until we are ready
    registerAsPrimaryComponent();
    primaryComponentDoNotEndSim();

    // Allow the user to control verbosity from the log file.
    uint32_t verbosityLevel = params.find_integer("verbose", 2);
    zOut.init("ZSirius", (uint32_t) verbosityLevel, (uint32_t) 1, Output::STDOUT);
}

void ZodiacSiriusTraceReader::setup() {
    msgapi->_componentSetup();

    rank = msgapi->myWorldRank();
    eventQ = new std::queue<ZodiacEvent*>();

    char trace_name[trace_file.length() + 20];
    sprintf(trace_name, "%s.%d", trace_file.c_str(), rank);

    printf("Opening trace file: %s\n", trace_name);
    trace = new SiriusReader(trace_name, rank, 64, eventQ);
    trace->setOutput(&zOut);

    int count = trace->generateNextEvents();
    std::cout << "Obtained: " << count << " events" << std::endl;

    if(eventQ->size() > 0) {
	selfLink->send(eventQ->front());
	eventQ->pop();
    }

    char logPrefix[512];
    sprintf(logPrefix, "ZSirius::SimulatedRank[%d]: ", rank);
    string logPrefixStr = logPrefix;
    zOut.setPrefix(logPrefixStr);

    // Clear counters
    zSendCount = 0;
    zRecvCount = 0;
    zIRecvCount = 0;
    zWaitCount = 0;
    zSendBytes = 0;
    zRecvBytes = 0;
    zIRecvBytes = 0;
}

void ZodiacSiriusTraceReader::init(unsigned int phase) {
	msgapi->_componentInit(phase);
}

void ZodiacSiriusTraceReader::finish() {
	zOut.verbose(__LINE__, __FILE__, "Finish", 1, 1, "Completed simulation at: %"PRIu64"ns\n",
		getCurrentSimTimeNano());
	zOut.verbose(__LINE__, __FILE__, "Finish", 1, 1, "Statistics for run are:\n");
	zOut.verbose(__LINE__, __FILE__, "Finish", 1, 1, "- Total Send Count:         %"PRIu64"\n", zSendCount);
	zOut.verbose(__LINE__, __FILE__, "Finish", 1, 1, "- Total Recv Count:         %"PRIu64"\n", zRecvCount);
	zOut.verbose(__LINE__, __FILE__, "Finish", 1, 1, "- Total IRecv Count:        %"PRIu64"\n", zIRecvCount);
	zOut.verbose(__LINE__, __FILE__, "Finish", 1, 1, "- Total Wait Count:         %"PRIu64"\n", zWaitCount);
	zOut.verbose(__LINE__, __FILE__, "Finish", 1, 1, "- Total Send Bytes:         %"PRIu64"\n", zSendBytes);
	zOut.verbose(__LINE__, __FILE__, "Finish", 1, 1, "- Total Recv Bytes:         %"PRIu64"\n", zRecvBytes);
	zOut.verbose(__LINE__, __FILE__, "Finish", 1, 1, "- Total Posted-IRecv Bytes: %"PRIu64"\n", zIRecvBytes);

	zOut.output("Completed at %"PRIu64"ns\n", getCurrentSimTimeNano());
}

ZodiacSiriusTraceReader::~ZodiacSiriusTraceReader() {
	if(! trace->hasReachedFinalize()) {
		zOut.output("WARNING: Component did not reach a finalize event, yet the component destructor has been called.\n");
	}

	trace->close();
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

	msgapi->barrier(zBEv->getCommunicatorGroup(), &retFunctor);
}

void ZodiacSiriusTraceReader::handleAllreduceEvent(ZodiacEvent* zEv) {
	ZodiacAllreduceEvent* zAEv = static_cast<ZodiacAllreduceEvent*>(zEv);
	assert(zAEv);
	assert((zAEv->getLength() * 2 * 8) < emptyBufferSize);

	zOut.verbose(__LINE__, __FILE__, "handleAllreduceEvent",
		2, 1, "Processing an Allreduce event.\n");

	msgapi->allreduce((Addr) emptyBuffer,
		(Addr) (&emptyBuffer[zAEv->getLength()]),
		zAEv->getLength(),
		zAEv->getDataType(),
		zAEv->getOp(),
		zAEv->getCommunicatorGroup(), &retFunctor);
}

void ZodiacSiriusTraceReader::handleInitEvent(ZodiacEvent* zEv) {
	zOut.verbose(__LINE__, __FILE__, "handleInitEvent",
		2, 1, "Processing a Init event.\n");

	// Just initialize the library nothing fancy to do here
	msgapi->init(&retFunctor);
}

void ZodiacSiriusTraceReader::handleFinalizeEvent(ZodiacEvent* zEv) {

	zOut.verbose(__LINE__, __FILE__, "handleFinalizeEvent",
		2, 1, "Processing a Finalize event.\n");

	// Just finalize the library nothing fancy to do here
	msgapi->fini(&retFunctor);
}

void ZodiacSiriusTraceReader::handleSendEvent(ZodiacEvent* zEv) {
	ZodiacSendEvent* zSEv = static_cast<ZodiacSendEvent*>(zEv);
	assert(zSEv);
	assert((zSEv->getLength() * 8) < emptyBufferSize);

	zOut.verbose(__LINE__, __FILE__, "handleSendEvent",
		2, 1, "Processing a Send event (length=%"PRIu32", tag=%d, dest=%"PRIu32")\n",
		zSEv->getLength(), zSEv->getMessageTag(), zSEv->getDestination());

	msgapi->send((Addr) emptyBuffer, zSEv->getLength(),
		zSEv->getDataType(), (RankID) zSEv->getDestination(),
		zSEv->getMessageTag(), zSEv->getCommunicatorGroup(), &retFunctor);

	zSendBytes += (msgapi->sizeofDataType(zSEv->getDataType()) * zSEv->getLength());
	zSendCount++;
}

void ZodiacSiriusTraceReader::handleRecvEvent(ZodiacEvent* zEv) {
	ZodiacRecvEvent* zREv = static_cast<ZodiacRecvEvent*>(zEv);
	assert(zREv);
	assert((zREv->getLength() * 8) < emptyBufferSize);

	currentRecv = (MessageResponse*) malloc(sizeof(MessageResponse));
	memset(currentRecv, 1, sizeof(MessageResponse));

	zOut.verbose(__LINE__, __FILE__, "handleRecvEvent",
		2, 1, "Processing a Recv event (length=%"PRIu32", tag=%d, source=%"PRIu32")\n",
		zREv->getLength(), zREv->getMessageTag(), zREv->getSource());

	msgapi->recv((Addr) emptyBuffer, zREv->getLength(),
		zREv->getDataType(), (RankID) zREv->getSource(),
		zREv->getMessageTag(), zREv->getCommunicatorGroup(),
		currentRecv, &recvFunctor);

	zRecvBytes += (msgapi->sizeofDataType(zREv->getDataType()) * zREv->getLength());
	zRecvCount++;
}

void ZodiacSiriusTraceReader::handleWaitEvent(ZodiacEvent* zEv) {
	ZodiacWaitEvent* zWEv = static_cast<ZodiacWaitEvent*>(zEv);
	assert(zWEv);

	MessageRequest* msgReq = reqMap[zWEv->getRequestID()];
	currentRecv = (MessageResponse*) malloc(sizeof(MessageResponse));
	memset(currentRecv, 1, sizeof(MessageResponse));

	zOut.verbose(__LINE__, __FILE__, "handleWaitEvent",
		2, 1, "Processing a Wait event.\n");

	msgapi->wait(msgReq, currentRecv, &retFunctor);
	zWaitCount++;
}

void ZodiacSiriusTraceReader::handleIRecvEvent(ZodiacEvent* zEv) {
	ZodiacIRecvEvent* zREv = static_cast<ZodiacIRecvEvent*>(zEv);
	assert(zREv);
	assert((zREv->getLength() * 8) < emptyBufferSize);

	MessageRequest* msgReq = (MessageRequest*) malloc(sizeof(MessageRequest));
	reqMap.insert(std::pair<uint64_t, MessageRequest*>(zREv->getRequestID(), msgReq));

	zOut.verbose(__LINE__, __FILE__, "handleIrecvEvent",
		2, 1, "Processing a Irecv event (length=%"PRIu32", tag=%d, source=%"PRIu32")\n",
		zREv->getLength(), zREv->getMessageTag(), zREv->getSource());

	msgapi->irecv((Addr) emptyBuffer, zREv->getLength(),
		zREv->getDataType(), (RankID) zREv->getSource(),
		zREv->getMessageTag(), zREv->getCommunicatorGroup(),
		msgReq, &retFunctor);

	zOut.verbose(__LINE__, __FILE__, "handleIrecvEvent",
		2, 1, "IRecv handover to Msg-API completed.\n");

	zIRecvBytes += (msgapi->sizeofDataType(zREv->getDataType()) * zREv->getLength());
	zIRecvCount++;
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
}

bool ZodiacSiriusTraceReader::clockTic( Cycle_t ) {
  // return false so we keep going
  return false;
}

void ZodiacSiriusTraceReader::completedFunction(int retVal) {
	if((0 == eventQ->size()) && (!trace->hasReachedFinalize())) {
		trace->generateNextEvents();
	}

	if(eventQ->size() > 0) {
		zOut.verbose(__LINE__, __FILE__, "completedFunction",
			2, 1, "Returned from a call to the message API, posting the next event...\n");

		ZodiacEvent* nextEv = eventQ->front();
		eventQ->pop();
		selfLink->send(nextEv);
	} else {
		zOut.output("No more events to process.\n");

		// We have run out of events
		primaryComponentOKToEndSim();
	}
}

void ZodiacSiriusTraceReader::completedRecvFunction(int retVal) {
	free(currentRecv);

	if((0 == eventQ->size()) && (!trace->hasReachedFinalize())) {
		trace->generateNextEvents();
	}

	if(eventQ->size() > 0) {
		zOut.verbose(__LINE__, __FILE__, "completedRecvFunction",
			2, 1, "Returned from a call to the message API, posting the next event...\n");

		ZodiacEvent* nextEv = eventQ->front();
		eventQ->pop();
		selfLink->send(nextEv);
	} else {
		zOut.output("No more events to process.\n");

		// We have run out of events
		primaryComponentOKToEndSim();
	}
}

BOOST_CLASS_EXPORT(ZodiacSiriusTraceReader)
