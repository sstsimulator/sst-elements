
#include "emberengine.h"

using namespace std;
using namespace SST::Ember;

EmberEngine::EmberEngine(SST::ComponentId_t id, SST::Params& params) :
	finalizeFunctor(HermesAPIFunctor(this, &EmberEngine::completedFinalize)),
	initFunctor(HermesAPIFunctor(this, &EmberEngine::completedInit)),
	recvFunctor(HermesAPIFunctor(this, &EmberEngine::completedRecv)),
	sendFunctor(HermesAPIFunctor(this, &EmberEngine::completedSend))
{
	uint32_t verbosity = (uint32_t) params.find_integer("verbose", 1);
	output.init("EmberEngine", verbosity, (uint32_t) 0, Output::STDOUT);

	// Configure the empty buffer ready for use by MPI routines.
	emptyBufferSize = (uint32_t) params.find_integer("buffersize", 8192);
	emptyBuffer = (char*) malloc(sizeof(char) * emptyBufferSize);

	// Create the messaging interface we are going to use
	string msgiface = params.find_string("msgapi");
	if ( msgiface == "" ) {
        	msgapi = new MessageInterface();
    	} else {
        	Params hermesParams = params.find_prefix_params("hermesParams." );

        	msgapi = dynamic_cast<MessageInterface*>(loadModuleWithComponent(
                            msgiface, this, hermesParams));

        	if(NULL == msgapi) {
                	std::cerr << "Message API: " << msgiface << " could not be loaded." << std::endl;
                	exit(-1);
        	}
    	}

	// Create the generator
	string gentype = params.find_string("generator");
	if( gentype == "" ) {
		output.fatal(CALL_INFO, -1, "Error: You did not specify a generator for Ember to use (parameter is called \'generator\')\n");
	} else {
		Params generatorParams = params.find_prefix_params("generatorParams.");

		generator = dynamic_cast<EmberGenerator*>( loadModuleWithComponent(gentype, this, generatorParams ) );

		if(NULL == generator) {
			output.fatal(CALL_INFO, -1, "Error: Could not load the generator %s for Ember\n", gentype.c_str());
		}
	}

	// Configure self link to handle event timing
	selfEventLink = configureSelfLink("self", "1ps",
		new Event::Handler<EmberEngine>(this, &EmberEngine::handleEvent));

	// Send an start event to this rank, this starts up the component
	EmberStartEvent* startEv = new EmberStartEvent();
	selfEventLink->send(startEv);

	// Add the init event to the queue since all ranks must eventually
	// initialize themselves
	EmberInitEvent* initEv = new EmberInitEvent();
	evQueue.push(initEv);

	// Make sure we don't stop the simulation until we are ready
    	registerAsPrimaryComponent();
    	primaryComponentDoNotEndSim();

	// Create a time converter for our compute events
	nanoTimeConverter = Simulation::getSimulation()->getTimeLord()->getTimeConverter("1ns");
}

EmberEngine::~EmberEngine() {
	// Free the big buffer we have been using
	free(emptyBuffer);
}

void EmberEngine::init(unsigned int phase) {
	// Pass the init phases through to the communication layer
	msgapi->_componentInit(phase);
}

void EmberEngine::setup() {
	// Notify communication layer we are done with init phase
	// and are now in final bring up state
	msgapi->_componentSetup();

	// Get my rank from the communication layer, we will
	// need to pass this to the generator
	thisRank = (int) msgapi->myWorldRank();

	char outputPrefix[256];
	sprintf(outputPrefix, "@t:%d:ZSirius::@p:@l: ", thisRank);
	string outputPrefixStr = outputPrefix;
	output.setPrefix(outputPrefixStr);

	// Update event count to ensure we are not correctly sync'd
	eventCount = (uint32_t) evQueue.size();
}

void EmberEngine::processInitEvent(EmberInitEvent* ev) {
	output.verbose(CALL_INFO, 2, 0, "Processing an Init Event\n");
	msgapi->init(&initFunctor);
}

void EmberEngine::processSendEvent(EmberSendEvent* ev) {
	output.verbose(CALL_INFO, 2, 0, "Processing a Send Event (%s)\n", ev->getPrintableString().c_str());
	msgapi->send((Addr) emptyBuffer, ev->getMessageSize(),
		CHAR, (RankID) ev->getSendToRank(),
		ev->getTag(), ev->getCommunicator(),
		&sendFunctor);
}

void EmberEngine::processRecvEvent(EmberRecvEvent* ev) {
	output.verbose(CALL_INFO, 2, 0, "Processing a Recv Event (%s)\n", ev->getPrintableString().c_str());

	memset(&currentRecv, 0, sizeof(MessageResponse));
	msgapi->recv((Addr) emptyBuffer, ev->getMessageSize(),
		CHAR, (RankID) ev->getRecvFromRank(),
		ev->getTag(), ev->getCommunicator(),
		&currentRecv, &recvFunctor);
}

void EmberEngine::processFinalizeEvent(EmberFinalizeEvent* ev) {
	output.verbose(CALL_INFO, 2, 0, "Processing a Finalize Event\n");
	msgapi->fini(&finalizeFunctor);
}

void EmberEngine::completedInit(int val) {
	output.verbose(CALL_INFO, 2, 0, "Completed Init, result = %d\n", val);
	issueNextEvent(0);
}

void EmberEngine::completedFinalize(int val) {
	output.verbose(CALL_INFO, 2, 0, "Completed Finalize, result = %d\n", val);
	issueNextEvent(0);
}

void EmberEngine::completedSend(int val) {
	output.verbose(CALL_INFO, 2, 0, "Completed Send, result = %d\n", val);
	issueNextEvent(0);
}

void EmberEngine::completedRecv(int val) {
	output.verbose(CALL_INFO, 2, 0, "Completed Recv, result = %d\n", val);
	issueNextEvent(0);
}

void EmberEngine::refillQueue() {
	generator->generate(&output, generationPhase++, thisRank, &evQueue);
	eventCount = evQueue.size();
}

void EmberEngine::checkQueue() {
	// Check if we have run out of events, if yes then
	// we must refill the queue to continue
	if(0 == eventCount) {
		refillQueue();
	}
}

void EmberEngine::issueNextEvent(uint32_t nanoDelay) {
	// This issues the next event on the self link
	// Check the queue, may need refilling
	checkQueue();

	if(0 == eventCount) {
		// We are completed so we can now exit
	} else {
		EmberEvent* nextEv = evQueue.front();
		evQueue.pop();

		// issue the next event to the engine for deliver later
		selfEventLink->send(nanoDelay, nanoTimeConverter, nextEv);
	}
}

void EmberEngine::handleEvent(Event* ev) {
	EmberEvent* eEv = static_cast<EmberEvent*>(ev);

	// Process the next event
	switch(eEv->getEventType()) {
	case SEND:
		processSendEvent(dynamic_cast<EmberSendEvent*>(eEv));
		break;
	case RECV:
		processRecvEvent(dynamic_cast<EmberRecvEvent*>(eEv));
		break;
	case FINALIZE:
		processFinalizeEvent(dynamic_cast<EmberFinalizeEvent*>(eEv));
		break;
	case INIT:
		processInitEvent(dynamic_cast<EmberInitEvent*>(eEv));
		break;
	default:

		break;
	}

}

BOOST_CLASS_EXPORT(EmberEngine)
