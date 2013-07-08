
#include "sst_config.h"
#include "sst/core/serialization/element.h"
#include <assert.h>

#include "sst/core/element.h"

#include "zsirius.h"

using namespace std;
using namespace SST;
using namespace SST::Zodiac;

ZodiacSiriusTraceReader::ZodiacSiriusTraceReader(ComponentId_t id, Params_t& params) :
  Component(id),
  retFunctor(DerivedFunctor(this, &ZodiacSiriusTraceReader::completedFunction)),
  recvFunctor(DerivedFunctor(this, &ZodiacSiriusTraceReader::completedRecvFunction))
{

    string msgiface = params.find_string("msgapi");

    if ( msgiface == "" ) {
        msgapi = new MessageInterface();
    } else {
	// Took code from Mike, Scott is going to fix.
	std::ostringstream ownerName;
    	ownerName << this;
    	Params_t hermesParams = params.find_prefix_params("hermesParams." );
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
}

void ZodiacSiriusTraceReader::setup() {
    msgapi->_componentSetup();

    rank = msgapi->myWorldRank();
    eventQ = new std::queue<ZodiacEvent*>();

    char trace_name[trace_file.length() + 20];
    sprintf(trace_name, "%s.%d", trace_file.c_str(), rank);

    printf("Opening trace file: %s\n", trace_name);
    trace = new SiriusReader(trace_name, rank, 64, eventQ, verbosityLevel);
    int count = trace->generateNextEvents();
    std::cout << "Obtained: " << count << " events" << std::endl;

    if(eventQ->size() > 0) {
	selfLink->send(eventQ->front());
	eventQ->pop();
    }

    char logPrefix[512];
    sprintf(logPrefix, "ZSirius::SimulatedRank[%d]: ", rank);
    string logPrefixStr = logPrefix;

    zOut.init(logPrefixStr, (uint32_t) 0, (uint32_t) 1, Output::STDOUT);

}

void ZodiacSiriusTraceReader::init(unsigned int phase) {
	msgapi->_componentInit(phase);
}

void ZodiacSiriusTraceReader::finish() {
	zOut.output("Completed at %"PRIu64"ns", getCurrentSimTimeNano());
	trace->close();
}

ZodiacSiriusTraceReader::~ZodiacSiriusTraceReader() {

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
		case COMPUTE:
			handleComputeEvent(zEv);
			break;

		case SEND:
			handleSendEvent(zEv);
			break;

		case RECV:
			handleRecvEvent(zEv);
			break;

		case IRECV:
			handleIRecvEvent(zEv);
			break;

		case WAIT:
			handleWaitEvent(zEv);
			break;

		case BARRIER:
			break;

		case INIT:
			handleInitEvent(zEv);
			break;

		case SKIP:
			break;
		}
	} else {
		zOut.verbose(__LINE__, __FILE__, "handleSelfEvent",
			0, 1, "Attempted to process an event which is not supported by the Zodiac engine.\n");
		exit(-1);
	}

	delete zEv;
}

void ZodiacSiriusTraceReader::handleInitEvent(ZodiacEvent* zEv) {
	// Just initialize the library nothing fancy to do here
	msgapi->init(&retFunctor);
}

void ZodiacSiriusTraceReader::handleSendEvent(ZodiacEvent* zEv) {
	ZodiacSendEvent* zSEv = static_cast<ZodiacSendEvent*>(zEv);
	assert(zSEv);
	assert(zSEv->getLength() < emptyBufferSize);

	zOut.verbose(__LINE__, __FILE__, "handleSendEvent",
		2, 1, "Processing a Send event (length=%"PRIu32", tag=%d, dest=%"PRIu32")\n",
		zSEv->getLength(), zSEv->getMessageTag(), zSEv->getDestination());

	msgapi->send((Addr) emptyBuffer, zSEv->getLength(),
		zSEv->getDataType(), (RankID) zSEv->getDestination(),
		zSEv->getMessageTag(), zSEv->getCommunicatorGroup(), &retFunctor);
}

void ZodiacSiriusTraceReader::handleRecvEvent(ZodiacEvent* zEv) {
	ZodiacRecvEvent* zREv = static_cast<ZodiacRecvEvent*>(zEv);
	assert(zREv);
	assert(zREv->getLength() < emptyBufferSize);

	currentRecv = (MessageResponse*) malloc(sizeof(MessageResponse));
	memset(currentRecv, 1, sizeof(MessageResponse));

	zOut.verbose(__LINE__, __FILE__, "handleRecvEvent",
		2, 1, "Processing a Recv event (length=%"PRIu32", tag=%d, source=%"PRIu32")\n",
		zREv->getLength(), zREv->getMessageTag(), zREv->getSource());

	msgapi->recv((Addr) emptyBuffer, zREv->getLength(),
		zREv->getDataType(), (RankID) zREv->getSource(),
		zREv->getMessageTag(), zREv->getCommunicatorGroup(),
		currentRecv, &recvFunctor);
}

void ZodiacSiriusTraceReader::handleWaitEvent(ZodiacEvent* zEv) {
	ZodiacWaitEvent* zWEv = static_cast<ZodiacWaitEvent*>(zEv);
	assert(zWEv);

	MessageRequest* msgReq = reqMap[zWEv->getRequestID()];
	currentRecv = (MessageResponse*) malloc(sizeof(MessageResponse));
	memset(currentRecv, 1, sizeof(MessageResponse));

	zOut.verbose(__LINE__, __FILE__, "handleWaitEvent",
		2, 1, "Processing a Wait event.\n");

	msgapi->wait(msgReq, currentRecv, &recvFunctor);
}

void ZodiacSiriusTraceReader::handleIRecvEvent(ZodiacEvent* zEv) {
	ZodiacIRecvEvent* zREv = static_cast<ZodiacIRecvEvent*>(zEv);
	assert(zREv);
	assert(zREv->getLength() < emptyBufferSize);

	MessageRequest* msgReq = (MessageRequest*) malloc(sizeof(MessageRequest));
	reqMap.insert(std::pair<uint64_t, MessageRequest*>(zREv->getRequestID(), msgReq));

	zOut.verbose(__LINE__, __FILE__, "handleIrecvEvent",
		2, 1, "Processing a Irecv event (length=%"PRIu32", tag=%d, source=%"PRIu32")\n",
		zREv->getLength(), zREv->getMessageTag(), zREv->getSource());

	msgapi->irecv((Addr) emptyBuffer, zREv->getLength(),
		zREv->getDataType(), (RankID) zREv->getSource(),
		zREv->getMessageTag(), zREv->getCommunicatorGroup(),
		msgReq, &recvFunctor);
}

void ZodiacSiriusTraceReader::handleComputeEvent(ZodiacEvent* zEv) {
	ZodiacComputeEvent* zCEv = static_cast<ZodiacComputeEvent*>(zEv);
	assert(zCEv);

	zOut.verbose(__LINE__, __FILE__, "handleComputeEvent",
		2, 1, "Processing a compute event (duration=%d seconds)\n",
		zCEv->getComputeDuration());

	if((0 == eventQ->size()) && (!trace->hasReachedFinalize())) {
		trace->generateNextEvents();
	}

	if(eventQ->size() > 0) {
		ZodiacEvent* nextEv = eventQ->front();
		zOut.verbose(__LINE__, __FILE__, "handleComputeEvent",
			2, 1, "Enqueuing next event at a delay of %d seconds)\n",
			zCEv->getComputeDuration());
		eventQ->pop();
		selfLink->send(zCEv->getComputeDurationNano(), tConv, nextEv);
	} else {
		std::cout << "ZSirius: Has no more events to process" << std::endl;

		// We have run out of events
		primaryComponentOKToEndSim();
	}
}

bool ZodiacSiriusTraceReader::clockTic( Cycle_t ) {
  std::cout << "Generated: " << eventQ->size() << " events." << std::endl;

  // return false so we keep going
  return false;
}

void ZodiacSiriusTraceReader::completedFunction(int retVal) {
	if((0 == eventQ->size()) && (!trace->hasReachedFinalize())) {
		trace->generateNextEvents();
	}

	if(eventQ->size() > 0) {
		std::cout << "ZSirius: Posting the next event..." << std::endl;
		ZodiacEvent* nextEv = eventQ->front();
		eventQ->pop();
		selfLink->send(nextEv);
	} else {
		std::cout << "ZSirius: Has no more events to process" << std::endl;

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
		std::cout << "ZSirius: Posting the next event..." << std::endl;
		ZodiacEvent* nextEv = eventQ->front();
		eventQ->pop();
		selfLink->send(nextEv);
	} else {
		std::cout << "ZSirius: Has no more events to process" << std::endl;

		// We have run out of events
		primaryComponentOKToEndSim();
	}
}

BOOST_CLASS_EXPORT(ZodiacSiriusTraceReader)
