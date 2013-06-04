
#include <vector>

#include "sst_config.h"
#include "sst/core/serialization/element.h"
#include "sst/core/element.h"

#include "strideprefetch.h"

using namespace SST;
using namespace SST::Interfaces;
using namespace SST::Cassini;

StridePrefetcher::StridePrefetcher(ComponentId_t id, Params_t& params) :
	Component(id) {

	cpuLink = configureLink( "cpuLink", "1ns",
		new Event::Handler<StridePrefetcher>(this, &StridePrefetcher::handleCPULinkEvent));
	memoryLink = configureLink( "memoryLink", "1ns",
		new Event::Handler<StridePrefetcher>(this, &StridePrefetcher::handleMemoryLinkEvent));
	cacheCPULink = configureLink( "cacheCPULink", "1ns",
		new Event::Handler<StridePrefetcher>(this, &StridePrefetcher::handleCacheToCPUEvent));
	cacheMemoryLink = configureLink( "cacheMemoryLink", "1ns",
		new Event::Handler<StridePrefetcher>(this, &StridePrefetcher::handleCacheToMemoryEvent));

	maximumPending = 4;
	if ( params.find("pending") != params.end() ) {
		maximumPending = strtol( params[ "pending" ].c_str(), NULL, 0 );
        }
}

void StridePrefetcher::handleCPULinkEvent(SST::Event* event) {
	//std::cout << "StridePrefetcher: recv event from CPU at: " <<
	//	getCurrentSimTimeNano() << "ns, sending to the cache..." << std::endl;

	cacheCPULink->send(event);
}

void StridePrefetcher::handleMemoryLinkEvent(SST::Event* event) {
	//std::cout << "StridePrefetcher: recv event from memory at: " <<
	//	getCurrentSimTimeNano() << "ns, sending to the cache..." << std::endl;

	cacheMemoryLink->send(event);
}

void StridePrefetcher::handleCacheToCPUEvent(SST::Event* event) {
	//std::cout << "StridePrefetcher: recv event from cache (to go to the CPU) at: " <<
	//	getCurrentSimTimeNano() << "ns, sending to the CPU..." << std::endl;

	cpuLink->send(event);
}

void StridePrefetcher::handleCacheToMemoryEvent(SST::Event* event) {
	//std::cout << "StridePrefetcher: recv event from cache (to go to the memory) at: " <<
	//	getCurrentSimTimeNano() << "ns, sending to the memory..." << std::endl;

	memoryLink->send(event);
}

bool StridePrefetcher::clockTick(Cycle_t curCycle) {
	return false;
}

void StridePrefetcher::finish() {

}
