
#include "sst_config.h"
#include "sst/core/serialization.h"

#include "emberengine.h"

using namespace std;
using namespace SST::Statistics;
using namespace SST::Ember;

EmberEngine::EmberEngine(SST::ComponentId_t id, SST::Params& params) :
    Component( id ),
	finalizeFunctor(HermesAPIFunctor(this, &EmberEngine::completedFinalize)),
	initFunctor(HermesAPIFunctor(this, &EmberEngine::completedInit)),
	recvFunctor(HermesAPIFunctor(this, &EmberEngine::completedRecv)),
	sendFunctor(HermesAPIFunctor(this, &EmberEngine::completedSend))
{
	// Get the level of verbosity the user is asking to print out, default is 1
	// which means don't print much.
	uint32_t verbosity = (uint32_t) params.find_integer("verbose", 1);
	output.init("EmberEngine", verbosity, (uint32_t) 0, Output::STDOUT);

	// See if the user requested that we print statistics for this run
	printStats = ((uint32_t) (params.find_integer("printStats", 0))) != ((uint32_t) 0);

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

	// Add the init event to the queue since all ranks must eventually
	// initialize themselves
	EmberInitEvent* initEv = new EmberInitEvent();
	evQueue.push(initEv);

	// Make sure we don't stop the simulation until we are ready
    	registerAsPrimaryComponent();
    	primaryComponentDoNotEndSim();

	// Create a time converter for our compute events
	nanoTimeConverter = Simulation::getSimulation()->getTimeLord()->getTimeConverter("1ns");

	histoCompute = new Histogram<uint64_t>(20);
	histoSend = new Histogram<uint64_t>(5);
	histoRecv = new Histogram<uint64_t>(5);
	histoInit = new Histogram<uint64_t>(5);
	histoFinalize = new Histogram<uint64_t>(5);
	histoStart = new Histogram<uint64_t>(5);

	// Set the accumulation to be the start
	accumulateTime = histoStart;
}

EmberEngine::~EmberEngine() {
	// Free the big buffer we have been using
	free(emptyBuffer);
}

void EmberEngine::init(unsigned int phase) {
	// Pass the init phases through to the communication layer
	msgapi->_componentInit(phase);
}

void EmberEngine::finish() {
	if(printStats) {
		output.output("Ember Statistics for Rank %" PRIu32 "\n", thisRank);
/*		output.output("- Time spent in compute:         %" PRIu64 " ns\n", nanoCompute);
		output.output("- Time spent in init:            %" PRIu64 " ns\n", nanoInit);
		output.output("- Time spent in finalize:        %" PRIu64 " ns\n", nanoFinalize);
		output.output("- Time spent in send:            %" PRIu64 " ns\n", nanoSend);
		output.output("- Time spent in recv:            %" PRIu64 " ns\n", nanoRecv);
*/

		output.output("- Histogram of compute times:\n");
		for(uint32_t i = 0; i < histoCompute->getBinCount(); ++i) {
			printHistoBin(histoCompute->getBinStart() + i * histoCompute->getBinWidth(),
				histoCompute->getBinWidth(),
				histoCompute->getBinByIndex(i));
		}

	}
}

void EmberEngine::printHistoBin(uint64_t binStart, uint64_t width, uint64_t* bin) {
	output.output("-   [%" PRIu64 " to %" PRIu64 "] : %" PRIu64 "\n",
		binStart, binStart + width, *bin);
}

void EmberEngine::setup() {
	// Notify communication layer we are done with init phase
	// and are now in final bring up state
	msgapi->_componentSetup();

	// Get my rank from the communication layer, we will
	// need to pass this to the generator
	thisRank = (uint32_t) msgapi->myWorldRank();
	uint32_t worldSize = (uint32_t) msgapi->myWorldSize();

	generator->configureEnvironment(thisRank, worldSize);

	char outputPrefix[256];
	sprintf(outputPrefix, "@t:%d:ZSirius::@p:@l: ", (int) thisRank);
	string outputPrefixStr = outputPrefix;
	output.setPrefix(outputPrefixStr);

	// Update event count to ensure we are not correctly sync'd
	eventCount = (uint32_t) evQueue.size();

	// Send an start event to this rank, this starts up the component
	EmberStartEvent* startEv = new EmberStartEvent();
	selfEventLink->send(startEv);
}

void EmberEngine::processInitEvent(EmberInitEvent* ev) {
	output.verbose(CALL_INFO, 2, 0, "Processing an Init Event\n");
	msgapi->init(&initFunctor);

	accumulateTime = histoInit;
}

void EmberEngine::processSendEvent(EmberSendEvent* ev) {
	output.verbose(CALL_INFO, 2, 0, "Processing a Send Event (%s)\n", ev->getPrintableString().c_str());
	msgapi->send((Addr) emptyBuffer, ev->getMessageSize(),
		CHAR, (RankID) ev->getSendToRank(),
		ev->getTag(), ev->getCommunicator(),
		&sendFunctor);

	accumulateTime = histoSend;
}

void EmberEngine::processRecvEvent(EmberRecvEvent* ev) {
	output.verbose(CALL_INFO, 2, 0, "Processing a Recv Event (%s)\n", ev->getPrintableString().c_str());

	memset(&currentRecv, 0, sizeof(MessageResponse));
	msgapi->recv((Addr) emptyBuffer, ev->getMessageSize(),
		CHAR, (RankID) ev->getRecvFromRank(),
		ev->getTag(), ev->getCommunicator(),
		&currentRecv, &recvFunctor);

	accumulateTime = histoRecv;
}

void EmberEngine::processFinalizeEvent(EmberFinalizeEvent* ev) {
	output.verbose(CALL_INFO, 2, 0, "Processing a Finalize Event\n");
	msgapi->fini(&finalizeFunctor);

	accumulateTime = histoFinalize;

	// Tell the simulator core we are finished and do not need any further
	// processing to continue
	primaryComponentOKToEndSim();
}

void EmberEngine::processComputeEvent(EmberComputeEvent* ev) {
	output.verbose(CALL_INFO, 2, 0, "Processing a Compute Event (%s)\n", ev->getPrintableString().c_str());

	// Issue the next event with a delay (essentially the time we computed something)
	issueNextEvent(ev->getNanoSecondDelay());
	accumulateTime = histoCompute;
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
	generator->generate(&output, generationPhase++, &evQueue);
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
	// Accumulate the time processing the last event into a counter
	// we track these by event type
	const uint64_t sim_time_now = (uint64_t) getCurrentSimTimeNano();
	accumulateTime->add( sim_time_now - nextEventStartTimeNanoSec );
	nextEventStartTimeNanoSec = sim_time_now;

	// Cast out the event we are processing and then hand off to whatever
	// handlers we have created
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
	case COMPUTE:
		processComputeEvent(dynamic_cast<EmberComputeEvent*>(eEv));
		break;
	default:

		break;
	}

}

EmberEngine::EmberEngine() :
    Component(-1)
{
    // for serialization only
}

BOOST_CLASS_EXPORT(EmberEngine)
