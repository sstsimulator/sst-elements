
#include <vector>

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

	MemEvent* memEvent = dynamic_cast<MemEvent*>(event);

	if(memEvent) {
		RequestEntry newEntry;
		newEntry.memAddr = memEvent->getAddr();
		newEntry.size    = memEvent->getSize();

		pendingReq.push_back(newEntry);
	}

	cacheCPULink->send(event);
}

void NextBlockPrefetcher::handleMemoryLinkEvent(SST::Event* event) {
	std::cout << "NextBlockPrefetcher: recv event from memory at: " <<
		getCurrentSimTimeNano() << "ns, sending to the cache..." << std::endl;

	cacheMemoryLink->send(event);
}

void NextBlockPrefetcher::handleCacheToCPUEvent(SST::Event* event) {
	std::cout << "NextBlockPrefetcher: recv event from cache (to go to the CPU) at: " <<
		getCurrentSimTimeNano() << "ns, sending to the CPU..." << std::endl;

	bool send_to_cpu = false;

	MemEvent* memEvent = dynamic_cast<MemEvent*>(event);

	if(memEvent) {
		vector<RequestEntry>::iterator req_itr;
		for(req_itr = pendingReq.begin(); req_itr != pendingReq.end(); req_itr++) {
			if(req_itr->memAddr == memEvent->getAddr() &&
				req_itr->size == memEvent->getSize()) {

				// Found a request directly from the CPU, so lets
				// return it back to them
				send_to_cpu = true;
				pendingReq.erase(req_itr);
			}
		}
	}

	cpuLink->send(event);
}

void NextBlockPrefetcher::handleCacheToMemoryEvent(SST::Event* event) {
	std::cout << "NextBlockPrefetcher: recv event from cache (to go to the memory) at: " <<
		getCurrentSimTimeNano() << "ns, sending to the memory..." << std::endl;

	memoryLink->send(event);

	// this means that this was a cache miss by this stage, we need to trap this memory
	// address and then request the next block.
	MemEvent* memEvent = dynamic_cast<MemEvent*>(event);

	if(memEvent) {
		std::cout << "NextBlockPrefetcher: considering prefetch evIDRes(2)=" <<
			memEvent->getResponseToID().second << " myID=" <<
			this->getId() << " evID(2)=" << memEvent->getID().second << 
			", getAddr=" << memEvent->getAddr() << 
			", ReqData: " << (memEvent->getCmd() == RequestData ? "true" : "false") << 
			", Src=" << memEvent->getSrc() <<
			", Dst=" << memEvent->getDst() <<
			", Flags=" << memEvent->getFlags() << std::endl;
		/*if(memEvent->getResponseToID().second != this->getId() &&
			memEvent->getID().second != this->getId()) {

			if(memEvent->getCmd() == RequestData) {
				//if(memEvent->getSize() == 0) {
				// We have a read (miss) request from the cache which means we can prefetch
				Addr requestedAddr = memEvent->getAddr();
				Addr nextBlockAddr = (requestedAddr - (requestedAddr % 64)) + 64;
				MemEvent *prefReq = new MemEvent(this, nextBlockAddr, ReadReq);
				prefReq->setSrc("PREFETCH" + this->getId());
				prefReq->setFlags(11);

				std::cout << "NextBlockPrefetcher: created prefetch address: " <<
					nextBlockAddr << " (orig-miss: " << requestedAddr << 
					", prefReqID=" << prefReq->getID().second << std::endl;
				cacheCPULink->send(prefReq);
				//}
			}
		}*/

		bool createReq = memEvent->getCmd() == RequestData;
		vector<RequestEntry>::iterator req_itr;

		for(req_itr = pendingReq.begin(); req_itr != pendingReq.end(); req_itr++) {
			if(memEvent->getAddr() == req_itr->memAddr &&
				memEvent->getSize() == req_itr->size) {
				createReq = false;
			}
		}

		if(createReq) {
		Addr requestedAddr = memEvent->getAddr();
		Addr nextBlockAddr = (requestedAddr - (requestedAddr % 64)) + 64;
		MemEvent *prefReq = new MemEvent(this, nextBlockAddr, ReadReq);

		std::cout << "NextBlockPrefetcher: created prefetch address: " <<
			nextBlockAddr << " (orig-miss: " << requestedAddr << 
			", prefReqID=" << prefReq->getID().second << std::endl;
		cacheCPULink->send(prefReq);
		}
	}
}

bool NextBlockPrefetcher::clockTick(Cycle_t curCycle) {
	return false;
}

void NextBlockPrefetcher::finish() {

}
