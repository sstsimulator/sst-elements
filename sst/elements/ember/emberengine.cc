
#include "emberengine.h"

EmberEngine::EmberEngine() {

	// Send an init event to this rank, this sarts up the component
	// and then sets the correct rank for further mappings
	EmberInitEvent* initEv = new EmberInitEvent();
	selfEventLink->send(initEv);
}

EmberEngine::~EmberEngine() {

}

void EmberEngine::refillQueue() {
	generator->generate(thisRank, &evQueue);
	eventCount = evQueue->size();
}

void EmberEngine::checkQueue() {
	// Check if we have run out of events, if yes then
	// we must refill the queue to continue
	if(0 == eventCount) {
		refillQueue();
	}
}

void EmberEngine::handleFinalize() {

}

void EmberEngine::handleEvent() {
	// Refill queue if needed
	checkQueue();

	// This means we are done with processing, time to exit
	if(0 == eventCount) {
		// Exit engine
	} else {

		// Get the next event to be processed
		EmberEvent* nextEv = evQueue->front();
		evQueue->pop();

		// Process the next event
		switch(nextEv->getEventType()) {
		case SEND:

			break;
		case RECV:

			break;
		case FINALIZE:

			break;
		default:

			break;
		}

	}
}

#endif
