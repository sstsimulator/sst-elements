
#include "sst_config.h"
#include "sst/core/serialization/element.h"
#include "sst/core/element.h"

#include "nbprefetch.h"

using namespace SST;
using namespace SST::Interfaces;
using namespace SST::Cassini;

NextBlockPrefetcher::NextBlockPrefetcher(ComponentId_t id, Params_t& params) :
	Component(id) {
	
	cpuLink = configureLink( "cpuLink", "1ns",
		new Event::Handler<NextBlockPrefetcher>(this, &NextBlockPrefetcher::handleCPULinkEvent));
	memoryLink = configureLink( "memoryLink", "1ns",
		new Event::Handler<NextBlockPrefetcher>(this, &NextBlockPrefetcher::handleMemoryLinkEvent));
	cacheCPULink = configureLink( "cacheCPULink", "1ns",
		new Event::Handler<NextBlockPrefetcher>(this, &NextBlockPrefetcher::handleCacheToCPUEvent));
	cacheMemoryLink = configureLink( "cacheMemoryLink", "1ns",
		new Event::Handler<NextBlockPrefetcher>(this, &NextBlockPrefetcher::handleCacheToMemoryEvent));
}

void NextBlockPrefetcher::handleCPULinkEvent(SST::Event* event) {
	std::cout << "NextBlockPrefetcher: recv event from CPU at: " <<
		getCurrentSimTimeNano() << "ns, sending to the cache..." << std::endl;
		
	cacheCPULink->send(event);
}
		
void NextBlockPrefetcher::handleMemoryLinkEvent(SST::Event* event) {
	std::cout << "NextBlockPrefetcher: recv event from memory at: " <<
		getCurrentSimTimeNano() << "ns, sending to the cache..." << std::endl;

	vector<MemEvent*>::iterator pendingItr;
	bool found = false;

	for(pendingItr = pendingPrefetchReq.begin();
		pendingItr != pendingPrefetchReq.end(); pendingPrefetchReq++) {

		if((*pendingItr)->getID() == event->getID()) {
			found = true;
			delete (*pendingItr);
		}
	}

	if(!found) {
		cacheMemoryLink->send(event);
	}
}

void NextBlockPrefetcher::handleCacheToCPUEvent(SST::Event* event) {
	std::cout << "NextBlockPrefetcher: recv event from cache (to go to the CPU) at: " <<
		getCurrentSimTimeNano() << "ns, sending to the CPU..." << std::endl;
	cpuLink->send(event);
}

void NextBlockPrefetcher::handleCacheToMemoryEvent(SST::Event* event) {
	std::cout << "NextBlockPrefetcher: recv event from cache (to go to the memory) at: " <<
		getCurrentSimTimeNano() << "ns, sending to the memory..." << std::endl;

	// this means that this was a cache miss by this stage, we need to trap this memory
	// address and then request the next block.
	MemEvent* memEvent = dynamic_cast<MemEvent*>(event);
	
	if(memEvent) {
		if(ReadReq == memEvent->getCmd()) {
			// We have a read (miss) request from the cache which means we can prefetch
			Addr requestedAddr = memEvent->getAddr();
			Addr nextBlockAddr = (requestedAddr - (requestedAddr % 64)) + 64;
			
			MemEvent *prefReq = new MemEvent(this, nextBlockAddr, ReadReq);
			cacheCPULink->send(prefReq);

			pendingPrefetchReq.push_back(prefReq);
		}
	}
	
	memoryLink->send(event);
}

bool NextBlockPrefetcher::clockTick(Cycle_t curCycle) {
	return false;
}

void NextBlockPrefetcher::finish() {

}
