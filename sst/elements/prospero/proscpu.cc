
#include "sst_config.h"
#include "proscpu.h"

using namespace SST;
using namespace SST::Prospero;

ProsperoComponent::ProsperoComponent(ComponentId_t id, Params& params) :
	Component(id) {

	const uint32_t output_level = (uint32_t) params.find_integer("verbose", 0);
	output = new SST::Output("Prospero[@p:@l]: ", output_level, 0, SST::Output::STDOUT);

	std::string traceModule = params.find_string("reader", "prospero.TextTraceReader");
	output->verbose(CALL_INFO, 1, 0, "Reader module is: %s\n", traceModule.c_str());

	Params readerParams = params.find_prefix_params("readerParams.");
	reader = dynamic_cast<ProsperoTraceReader*>( loadModuleWithComponent(traceModule, this, readerParams) );

	if(NULL == reader) {
		output->fatal(CALL_INFO, -1, "Failed to load reader module: %s\n", traceModule.c_str());
	}

	pageSize = (uint64_t) params.find_integer("pagesize", 4096);
	output->verbose(CALL_INFO, 1, 0, "Configured Prospero page size for %" PRIu64 " bytes.\n", pageSize);

        cacheLineSize = (uint64_t) params.find_integer("cache_line_size", 64);
	output->verbose(CALL_INFO, 1, 0, "Configured Prospero cache line size for %" PRIu64 " bytes.\n", cacheLineSize);

	std::string prosClock = params.find_string("clock", "2GHz");
	// Register the clock
	registerClock(prosClock, new Clock::Handler<ProsperoComponent>(this, &ProsperoComponent::tick));

	output->verbose(CALL_INFO, 1, 0, "Configured Prospero clock for %s\n", prosClock.c_str());

	maxOutstanding = (uint32_t) params.find_integer("max_outstanding", 16);
	output->verbose(CALL_INFO, 1, 0, "Configured maximum outstanding transactions for %" PRIu32 "\n", maxOutstanding);

	maxIssuePerCycle = (uint32_t) params.find_integer("max_issue_per_cycle", 2);
	output->verbose(CALL_INFO, 1, 0, "Configured maximum transaction issue per cycle %" PRIu32 "\n", maxIssuePerCycle);

	// tell the simulator not to end without us
  	registerAsPrimaryComponent();
  	primaryComponentDoNotEndSim();

	output->verbose(CALL_INFO, 1, 0, "Configuring Prospero cache connection...\n");
	cache_link = dynamic_cast<SimpleMem*>(loadModuleWithComponent("memHierarchy.memInterface", this, params));
  	cache_link->initialize("cache_link", new SimpleMem::Handler<ProsperoComponent>(this,
		&ProsperoComponent::handleResponse) );
	output->verbose(CALL_INFO, 1, 0, "Configuration of memory interface completed.\n");

	output->verbose(CALL_INFO, 1, 0, "Reading first entry from the trace reader...\n");
	currentEntry = reader->readNextEntry();
	output->verbose(CALL_INFO, 1, 0, "Read of first entry complete.\n");

	// We start by telling the system to continue to process as long as the first entry
	// is not NULL
	traceEnded = currentEntry != NULL;

	readsIssued = 0;
	writesIssued = 0;
	splitReadsIssued = 0;
	splitWritesIssued = 0;

	output->verbose(CALL_INFO, 1, 0, "Prospero configuration completed successfully.\n");
}

ProsperoComponent::~ProsperoComponent() {

}

void ProsperoComponent::finish() {
	output->output("Prospero Component Statistics:\n");
	output->output("- Reads issued:             %" PRIu64 "\n", readsIssued);
	output->output("- Writes issued:            %" PRIu64 "\n", writesIssued);
	output->output("- Split reads issued:       %" PRIu64 "\n", splitReadsIssued);
	output->output("- Split writes issued:      %" PRIu64 "\n", splitWritesIssued);
}

void ProsperoComponent::handleResponse(SST::Event *ev) {
	output->verbose(CALL_INFO, 4, 0, "Handle response from memory subsystem.\n");

	// Our responsibility to delete incoming event
	delete ev;
}

bool ProsperoComponent::tick(SST::Cycle_t currentCycle) {
	output->verbose(CALL_INFO, 8, 0, "Prospero execute on cycle %" PRIu64 "\n", (uint64_t) currentCycle);

	// If we have finished reading the trace we need to let the events in flight
	// drain and the system come to a rest
	if(traceEnded) {
		if(currentOutstanding == 0) {
			primaryComponentOKToEndSim();
                        return true;
		}

		return false;
	}

	// Wait to see if the current operation can be issued, if yes then
	// go ahead and issue it, otherwise we will stall
	for(uint32_t i = 0; i < maxIssuePerCycle; ++i) {
		if(currentCycle >= currentEntry->getIssueAtCycle()) {
			if(currentOutstanding < maxOutstanding) {
				// Issue the pending request into the memory subsystem
				issueRequest(currentEntry);

				// Obtain the next newest request
				currentEntry = reader->readNextEntry();

				// Trace reader has read all entries, time to begin draining
				// the system, caches etc
				if(NULL == currentEntry) {
					traceEnded = true;
					break;
				}
			} else {
				// Cannot issue any more items this cycle, load/stores are full
				break;
			}
		} else {
			// Have reached a point in the trace which is too far ahead in time
			// so stall until we find that point
			break;
		}
	}

	// Keep simulation ticking, we have more work to do if we reach here
	return false;
}

void ProsperoComponent::issueRequest(ProsperoTraceEntry* entry) {
	const uint64_t entryAddress = entry->getAddress();
	const uint64_t entryLength  = (uint64_t) entry->getLength();

	const uint64_t lineOffset   = entryAddress % cacheLineSize;
	bool  isRead                = entry->isRead();

	if(lineOffset + entryLength > cacheLineSize) {
		// Perform a split cache line load
		const uint64_t lowerLength = cacheLineSize - lineOffset;
		const uint64_t upperLength = entryLength - lineOffset;

		assert(lowerLength + upperLength == entryLength);

		// Start split requests at the original requested address and then
		// also the the next cache line along
		const uint64_t lowerAddress = entryAddress;
		const uint64_t upperAddress = (lowerAddress - (lowerAddress % cacheLineSize)) + cacheLineSize;

		SimpleMem::Request* reqLower = new SimpleMem::Request(
			isRead ? SimpleMem::Request::Read : SimpleMem::Request::Write,
			lowerAddress, lowerLength);

		SimpleMem::Request* reqUpper = new SimpleMem::Request(
			isRead ? SimpleMem::Request::Read : SimpleMem::Request::Write,
			upperAddress, upperLength);

		cache_link->sendRequest(reqLower);
		cache_link->sendRequest(reqUpper);

		if(isRead) {
			readsIssued += 2;
			splitReadsIssued++;
		} else {
			writesIssued += 2;
			splitWritesIssued++;
		}
	} else {
		// Perform a single load
		SimpleMem::Request* request = new SimpleMem::Request(
			isRead ? SimpleMem::Request::Read : SimpleMem::Request::Write,
			entryAddress, entryLength);
		cache_link->sendRequest(request);

		if(isRead) {
			readsIssued++;
		} else {
			writesIssued++;
		}
	}
}
