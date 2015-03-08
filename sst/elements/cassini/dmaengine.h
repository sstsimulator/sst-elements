
#ifndef _H_SST_CASSINI_DMA_ENGINE
#define _H_SST_CASSINI_DMA_ENGINE

#include <sst/core/component.h>
#include <sst/core/output.h>
#include <deque>

#include <dmacmd.h>
#include <dmastate.h>
#include <dmamemop.h>

using namespace SST;
using namespace SST::Interfaces;
using namespace SST::Statistics;

namespace SST {
namespace Cassini {

class DMAEngine : public SST::Component {

public:
	DMAEngine(SST::ComponentId_t id, SST::Params& params);
	~DMAEngine();

	void init(unsigned int phase);
	void finish();
	void handleDMACommandIssue(SST::Event* ev);
	void handleMemorySystemEvent(Interfaces::SimpleMem::Request* ev);

private:
	DMAEngine(); 			// Serialization only, no implement
	DMAEngine(const DMAEngine&);	// Serialization only, no implement
	void operator=(const DMAEngine&); // Serialization only, no implement

	void issueNextCommand();

	uint64_t cacheLineSize;
	uint32_t maxInFlight;
	uint32_t opsInFlight;
	uint32_t cpuLinkCount;

	SST::Link* cpuSideLinks;
	SimpleMem* cache_link;

	std::vector<SimpleMem::Request::id_t> pendingWriteReqs;
	std::map<SimpleMem::Request::id_t, DMAMemoryOperation*> pendingReadReqs;

	std::deque<DMACommand*> cmdQ;
	DMAEngineState* currentState;
	Output* output;

};

}
}

#endif
