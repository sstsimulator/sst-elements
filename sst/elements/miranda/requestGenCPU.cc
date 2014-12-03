
#include <sst_config.h>
#include <sst/core/simulation.h>

#include "reqGenModule.h"
#include "requestGenCPU.h"

using namespace SST::Miranda;

RequestGenCPU::RequestGenCPU(SST::ComponentId_t id, SST::Params& params) :
	Component(id) {

	const int verbose = (int) params.find_integer("verbose", 0);
	out = new Output("RequestGenCPU[@p:@l]: ", verbose, 0, SST::Output::STDOUT);

	maxRequestsPending = (uint32_t) params.find_integer("maxmemoryreqs", 16);
	requestsPending = 0;

	out->verbose(CALL_INFO, 1, 0, "Configured CPU to allow %" PRIu32 " maximum requests to be memory to be outstanding.\n",
		maxRequestsPending);

	std::string interfaceName = params.find_string("memoryinterface", "memHierarchy.memInterface");
	out->verbose(CALL_INFO, 1, 0, "Memory interface to be loaded is: %s\n", interfaceName.c_str());

	Params interfaceParams = params.find_prefix_params("memoryinterfaceparams.");
	cache_link = dynamic_cast<SimpleMem*>( loadModuleWithComponent(interfaceName,
		this, interfaceParams) );

	if(NULL == cache_link) {
		out->fatal(CALL_INFO, -1, "Error loading memory interface module.\n");
	} else {
		out->verbose(CALL_INFO, 1, 0, "Loaded memory interface successfully.\n");
	}

	out->verbose(CALL_INFO, 1, 0, "Initializing memory interface...\n");

	bool init_success = cache_link->initialize("cache_link", new SimpleMem::Handler<RequestGenCPU>(this, &RequestGenCPU::handleEvent) );

	if(init_success) {
		out->verbose(CALL_INFO, 1, 0, "Loaded memory initialize routine returned successfully.\n");
	} else {
		out->fatal(CALL_INFO, -1, "Failed to initialize interface: %s\n", interfaceName.c_str());
	}

	if(NULL == cache_link) {
		out->fatal(CALL_INFO, -1, "Failed to load interface: %s\n", interfaceName.c_str());
	} else {
		out->verbose(CALL_INFO, 1, 0, "Loaded memory interface successfully.\n");
	}

	std::string reqGenModName = params.find_string("generator", "");
	out->verbose(CALL_INFO, 1, 0, "Request generator to be loaded is: %s\n", reqGenModName.c_str());
	Params genParams = params.find_prefix_params("generatorParams.");
	reqGen = dynamic_cast<RequestGenerator*>( loadModuleWithComponent(reqGenModName, this, genParams) );

	// Set first request to NULL to force a regenerate
	nextReq = new RequestGeneratorRequest();

	if(NULL == reqGen) {
		out->fatal(CALL_INFO, -1, "Failed to load generator: %s\n", reqGenModName.c_str());
	} else {
		out->verbose(CALL_INFO, 1, 0, "Generator loaded successfully.\n");
	}

	std::string cpuClock = params.find_string("clock", "2GHz");
	registerClock(cpuClock, new Clock::Handler<RequestGenCPU>(this, &RequestGenCPU::clockTick));

	out->verbose(CALL_INFO, 1, 0, "CPU clock configured for %s\n", cpuClock.c_str());
	registerAsPrimaryComponent();
	primaryComponentDoNotEndSim();

	out->verbose(CALL_INFO, 1, 0, "Configuration completed.\n");
}

RequestGenCPU::~RequestGenCPU() {
	delete cache_link;
	delete reqGen;
}

void RequestGenCPU::finish() {

}

void RequestGenCPU::init(unsigned int phase) {

}

void RequestGenCPU::handleEvent( Interfaces::SimpleMem::Request* ev) {
	out->verbose(CALL_INFO, 2, 0, "Recv event for processing from interface\n");

	// Decrement pending requests, we have recv'd a response
	requestsPending--;
}

void RequestGenCPU::issueRequest(RequestGeneratorRequest* req) {
	const uint64_t reqAddress = req->getAddress();
	const uint64_t reqLength  = req->getLength();
	bool isRead               = req->isRead();
	const uint64_t lineOffset = reqAddress % cacheLine;

	out->verbose(CALL_INFO, 4, 0, "Issue request: address=%" PRIu64 ", length=%" PRIu64 ", operation=%s, cache line offset=%" PRIu64 "\n",
		reqAddress, reqLength, (isRead ? "READ" : "WRITE"), lineOffset);

	if(lineOffset + reqLength > cacheLine) {
		// Request is for a split operation (i.e. split over cache lines)
		const uint64_t lowerLength = cacheLine - lineOffset;
		const uint64_t upperLength = reqLength - lowerLength;

		// Ensure that lengths are calculated correctly.
		assert(lowerLength + upperLength == reqLength);

		const uint64_t lowerAddress = reqAddress;
		const uint64_t upperAddress = (lowerAddress - lowerAddress % cacheLine) +
						cacheLine;

		out->verbose(CALL_INFO, 4, 0, "Issuing a split cache line operation:\n");
		out->verbose(CALL_INFO, 4, 0, "L -> Address: %" PRIu64 ", Length=%" PRIu64 "\n",
			lowerAddress, lowerLength);
		out->verbose(CALL_INFO, 4, 0, "U -> Address: %" PRIu64 ", Length=%" PRIu64 "\n",
			upperAddress, upperLength);

		SimpleMem::Request* reqLower = new SimpleMem::Request(
			isRead ? SimpleMem::Request::Read : SimpleMem::Request::Write,
			lowerAddress, lowerLength);

		SimpleMem::Request* reqUpper = new SimpleMem::Request(
			isRead ? SimpleMem::Request::Read : SimpleMem::Request::Write,
			upperAddress, upperLength);

		out->verbose(CALL_INFO, 4, 0, "Issuing requesting into cache link...\n");
		cache_link->sendRequest(reqLower);
		cache_link->sendRequest(reqUpper);
		out->verbose(CALL_INFO, 4, 0, "Completed issue.\n");

		requestsPending += 2;
	} else {
		// This is not a split laod, i.e. issue in a single transaction
		SimpleMem::Request* request = new SimpleMem::Request(
			isRead ? SimpleMem::Request::Read : SimpleMem::Request::Write,
			reqAddress, reqLength);

		cache_link->sendRequest(request);

		requestsPending++;
	}

	// Mark request as issued
	req->markIssued();
}

bool RequestGenCPU::clockTick(SST::Cycle_t cycle) {
	if(reqGen->isFinished()) {
		if(0 == requestsPending) {
			// Deregister here
			primaryComponentOKToEndSim();
			return true;
		} else {
			return false;
		}
	}

	if(nextReq->isIssued()) {
		reqGen->nextRequest(nextReq);
	}

	// Process the request which may require splitting into multiple
	// requests (if breaks over a cache line)
	out->verbose(CALL_INFO, 2, 0, "Requests pending %" PRIu32 ", maximum permitted %" PRIu32 ".\n",
		requestsPending, maxRequestsPending);

	if(requestsPending < maxRequestsPending) {
		out->verbose(CALL_INFO, 2, 0, "Will attempt to issue as free slots in load/store unit.\n");
		issueRequest(nextReq);
	} else {
		out->verbose(CALL_INFO, 4, 0, "Will not issue, not free slots in load/store unit.\n");
	}

	return false;
}
