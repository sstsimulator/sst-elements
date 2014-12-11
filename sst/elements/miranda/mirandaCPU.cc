
#include <sst_config.h>
#include <sst/core/simulation.h>
#include <sst/core/unitAlgebra.h>

#include "mirandaGenerator.h"
#include "mirandaCPU.h"

using namespace SST::Miranda;

RequestGenCPU::RequestGenCPU(SST::ComponentId_t id, SST::Params& params) :
	Component(id) {

	const int verbose = (int) params.find_integer("verbose", 0);
	out = new Output("RequestGenCPU[@p:@l]: ", verbose, 0, SST::Output::STDOUT);

	maxRequestsPending = (uint32_t) params.find_integer("maxmemreqpending", 16);
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

	printStats = (params.find_integer("printStats", 0) == 0 ) ? true : false;

	std::string reqGenModName = params.find_string("generator", "");
	out->verbose(CALL_INFO, 1, 0, "Request generator to be loaded is: %s\n", reqGenModName.c_str());
	Params genParams = params.find_prefix_params("generatorParams.");
	reqGen = dynamic_cast<RequestGenerator*>( loadModuleWithComponent(reqGenModName, this, genParams) );

	// Set first request to NULL to force a regenerate
	nextReq = new RequestGeneratorRequest();
	nextReq->markIssued();

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

	cacheLine = (uint64_t) params.find_integer("cache_line_size", 64);

	readRequestsIssued = 0;
	splitReadRequestsIssued = 0;
	writeRequestsIssued = 0;
	splitWriteRequestsIssued = 0;
	cyclesWithIssue = 0;
	cyclesWithoutIssue = 0;
	bytesRead = 0;
	bytesWritten = 0;
	reqLatency = 0;

	out->verbose(CALL_INFO, 1, 0, "Configuration completed.\n");
}

RequestGenCPU::~RequestGenCPU() {
	delete cache_link;
	delete reqGen;
}

void RequestGenCPU::finish() {
	if(printStats) return;

	out->output("------------------------------------------------------------------------\n");
	out->output("Miranda CPU Statistics (%s):\n", getName().c_str());
	out->output("\n");
	out->output("Total requests issued:                 %" PRIu64 "\n",
		(readRequestsIssued + (splitReadRequestsIssued * 2)+
		writeRequestsIssued + (splitWriteRequestsIssued * 2)));
	out->output("Total split Requests issued:           %" PRIu64 "\n",
		(splitReadRequestsIssued + splitWriteRequestsIssued));
	out->output("Total read (split + non-split):        %" PRIu64 "\n",
		(readRequestsIssued + splitReadRequestsIssued ));
	out->output("Total write (split + non-split):       %" PRIu64 "\n",
		(writeRequestsIssued + splitWriteRequestsIssued ));
	out->output("Non-split read requests:               %" PRIu64 "\n", readRequestsIssued);
	out->output("Non-split write requests:              %" PRIu64 "\n", writeRequestsIssued);
	out->output("Split read requests:                   %" PRIu64 "\n", splitReadRequestsIssued);
	out->output("Split write requests:                  %" PRIu64 "\n", splitWriteRequestsIssued);
	out->output("\n");
	out->output("Total bytes read:                      %" PRIu64 "\n", bytesRead);
	out->output("Total bytes written:                   %" PRIu64 "\n", bytesWritten);

	const uint64_t nanoSeconds = getCurrentSimTimeNano();
	out->output("Nanoseconds:                           %" PRIu64 "\n", nanoSeconds);

	const double seconds = ((double) nanoSeconds) / (1.0e9);
	out->output("Seconds:                               %f\n", seconds);

	char bufferRead[32];
	sprintf(bufferRead, "%f B/s", ((double) bytesRead / seconds));

	char bufferWrite[32];
	sprintf(bufferWrite, "%f B/s", ((double) bytesWritten / seconds));

	char bufferCombined[32];
	sprintf(bufferCombined, "%f B/s", ((double)(bytesRead + bytesWritten) / seconds));

	UnitAlgebra readBandwidth(bufferRead);
	UnitAlgebra writeBandwidth(bufferWrite);
	UnitAlgebra combinedBandwidth(bufferCombined);

	out->output("CPU Bandwidth (read):                  %s\n", readBandwidth.toStringBestSI().c_str());
	out->output("CPU Bandwidth (write):                 %s\n", writeBandwidth.toStringBestSI().c_str());
	out->output("CPU Bandwidth (combined):              %s\n", combinedBandwidth.toStringBestSI().c_str());

	char bufferLatency[32];
	sprintf(bufferLatency, "%f ns", ((double) reqLatency) / (readRequestsIssued + (splitReadRequestsIssued * 2)+
                writeRequestsIssued + (splitWriteRequestsIssued * 2)));

	UnitAlgebra avrLatency(bufferLatency);

	out->output("\n");
	out->output("Total request latency:                 %" PRIu64 " ns\n", reqLatency);
	out->output("Average request latency:               %s\n", avrLatency.toStringBestSI().c_str());

	out->output("\n");
	out->output("Cycles with request issues:            %" PRIu64 "\n", cyclesWithIssue);
	out->output("Cycles without request issue:          %" PRIu64 "\n", cyclesWithoutIssue);
	out->output("------------------------------------------------------------------------\n");
	out->output("\n");
}

void RequestGenCPU::init(unsigned int phase) {

}

void RequestGenCPU::handleEvent( Interfaces::SimpleMem::Request* ev) {
	out->verbose(CALL_INFO, 2, 0, "Recv event for processing from interface\n");

	SimpleMem::Request::id_t reqID = ev->id;
	std::map<SimpleMem::Request::id_t, uint64_t>::iterator reqFind = requests.find(reqID);

	if(reqFind == requests.end()) {
		out->fatal(CALL_INFO, -1, "Unable to find request %" PRIu64 " in request map.\n", reqID);
	} else{
		reqLatency += (getCurrentSimTimeNano() - reqFind->second);
		requests.erase(reqID);

		// Decrement pending requests, we have recv'd a response
		requestsPending--;

		delete ev;
	}
}

void RequestGenCPU::issueRequest(RequestGeneratorRequest* req) {
	const uint64_t reqAddress = req->getAddress();
	const uint64_t reqLength  = req->getLength();
	bool isRead               = req->isRead();
	const uint64_t lineOffset = reqAddress % cacheLine;

	out->verbose(CALL_INFO, 4, 0, "Issue request: address=%" PRIu64 ", length=%" PRIu64 ", operation=%s, cache line offset=%" PRIu64 "\n",
		reqAddress, reqLength, (isRead ? "READ" : "WRITE"), lineOffset);

	if(isRead) {
		bytesRead += reqLength;
	} else {
		bytesWritten += reqLength;
	}

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

		requests.insert( std::pair<SimpleMem::Request::id_t, uint64_t>(reqLower->id, getCurrentSimTimeNano()) );
		requests.insert( std::pair<SimpleMem::Request::id_t, uint64_t>(reqUpper->id, getCurrentSimTimeNano()) );

		out->verbose(CALL_INFO, 4, 0, "Issuing requesting into cache link...\n");
		cache_link->sendRequest(reqLower);
		cache_link->sendRequest(reqUpper);
		out->verbose(CALL_INFO, 4, 0, "Completed issue.\n");

		requestsPending += 2;

		// Keep track of split requests
		if(isRead) {
			splitReadRequestsIssued++;
		} else {
			splitWriteRequestsIssued++;
		}
	} else {
		// This is not a split laod, i.e. issue in a single transaction
		SimpleMem::Request* request = new SimpleMem::Request(
			isRead ? SimpleMem::Request::Read : SimpleMem::Request::Write,
			reqAddress, reqLength);

		requests.insert( std::pair<SimpleMem::Request::id_t, uint64_t>(request->id, getCurrentSimTimeNano()) );
		cache_link->sendRequest(request);

		requestsPending++;

		if(isRead) {
			readRequestsIssued++;
		} else {
			writeRequestsIssued++;
		}
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

		cyclesWithIssue++;
	} else {
		out->verbose(CALL_INFO, 4, 0, "Will not issue, not free slots in load/store unit.\n");

		cyclesWithoutIssue++;
	}

	return false;
}
