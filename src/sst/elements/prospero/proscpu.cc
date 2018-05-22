// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2018, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#include "sst_config.h"
#include <sst/core/unitAlgebra.h>

#include "proscpu.h"
#include <algorithm>

using namespace SST;
using namespace SST::Prospero;

#define PROSPERO_MAX(a, b) ((a) < (b) ? (b) : (a))

ProsperoComponent::ProsperoComponent(ComponentId_t id, Params& params) :
	Component(id) {

	const uint32_t output_level = (uint32_t) params.find<uint32_t>("verbose", 0);
	output = new SST::Output("Prospero[@p:@l]: ", output_level, 0, SST::Output::STDOUT);

	std::string traceModule = params.find<std::string>("reader", "prospero.ProsperoTextTraceReader");
	output->verbose(CALL_INFO, 1, 0, "Reader module is: %s\n", traceModule.c_str());

	Params readerParams = params.find_prefix_params("readerParams.");
	reader = dynamic_cast<ProsperoTraceReader*>( loadSubComponent(traceModule, this, readerParams) );

	if(NULL == reader) {
		output->fatal(CALL_INFO, -1, "Failed to load reader module: %s\n", traceModule.c_str());
	}

	reader->setOutput(output);

	pageSize = (uint64_t) params.find<uint64_t>("pagesize", 4096);
	output->verbose(CALL_INFO, 1, 0, "Configured Prospero page size for %" PRIu64 " bytes.\n", pageSize);

        cacheLineSize = (uint64_t) params.find<uint64_t>("cache_line_size", 64);
	output->verbose(CALL_INFO, 1, 0, "Configured Prospero cache line size for %" PRIu64 " bytes.\n", cacheLineSize);

	std::string prosClock = params.find<std::string>("clock", "2GHz");
	// Register the clock
	registerClock(prosClock, new Clock::Handler<ProsperoComponent>(this, &ProsperoComponent::tick));

	output->verbose(CALL_INFO, 1, 0, "Configured Prospero clock for %s\n", prosClock.c_str());

	maxOutstanding = (uint32_t) params.find<uint32_t>("max_outstanding", 16);
	output->verbose(CALL_INFO, 1, 0, "Configured maximum outstanding transactions for %" PRIu32 "\n", maxOutstanding);

	maxIssuePerCycle = (uint32_t) params.find<uint32_t>("max_issue_per_cycle", 2);
	output->verbose(CALL_INFO, 1, 0, "Configured maximum transaction issue per cycle %" PRIu32 "\n", maxIssuePerCycle);

	// tell the simulator not to end without us
  	registerAsPrimaryComponent();
  	primaryComponentDoNotEndSim();

	output->verbose(CALL_INFO, 1, 0, "Configuring Prospero cache connection...\n");
	cache_link = dynamic_cast<SimpleMem*>(loadSubComponent("memHierarchy.memInterface", this, params));
  	cache_link->initialize("cache_link", new SimpleMem::Handler<ProsperoComponent>(this,
		&ProsperoComponent::handleResponse) );
	output->verbose(CALL_INFO, 1, 0, "Configuration of memory interface completed.\n");

	output->verbose(CALL_INFO, 1, 0, "Reading first entry from the trace reader...\n");
	currentEntry = reader->readNextEntry();
	output->verbose(CALL_INFO, 1, 0, "Read of first entry complete.\n");

	output->verbose(CALL_INFO, 1, 0, "Creating memory manager with page size %" PRIu64 "...\n", pageSize);
	memMgr = new ProsperoMemoryManager(pageSize, output);
	output->verbose(CALL_INFO, 1, 0, "Created memory manager successfully.\n");

	// We start by telling the system to continue to process as long as the first entry
	// is not NULL
	traceEnded = currentEntry == NULL;

	readsIssued = 0;
	writesIssued = 0;
	splitReadsIssued = 0;
	splitWritesIssued = 0;
	totalBytesRead = 0;
	totalBytesWritten = 0;

	currentOutstanding = 0;
	cyclesWithNoIssue = 0;
	cyclesWithIssue = 0;

	output->verbose(CALL_INFO, 1, 0, "Prospero configuration completed successfully.\n");
}

ProsperoComponent::~ProsperoComponent() {
	delete memMgr;
	delete output;
}

void ProsperoComponent::init(unsigned int phase) {
    cache_link->init(phase);
}

void ProsperoComponent::finish() {
	const uint64_t nanoSeconds = getCurrentSimTimeNano();

	output->output("\n");
	output->output("Prospero Component Statistics:\n");

	output->output("------------------------------------------------------------------------\n");
	output->output("- Completed at:                          %" PRIu64 " ns\n", nanoSeconds);
	output->output("- Cycles with ops issued:                %" PRIu64 " cycles\n", cyclesWithIssue);
	output->output("- Cycles with no ops issued (LS full):   %" PRIu64 " cycles\n", cyclesWithNoIssue);

	output->output("------------------------------------------------------------------------\n");
	output->output("- Reads issued:                          %" PRIu64 "\n", readsIssued);
	output->output("- Writes issued:                         %" PRIu64 "\n", writesIssued);
	output->output("- Split reads issued:                    %" PRIu64 "\n", splitReadsIssued);
	output->output("- Split writes issued:                   %" PRIu64 "\n", splitWritesIssued);
	output->output("- Bytes read:                            %" PRIu64 "\n", totalBytesRead);
	output->output("- Bytes written:                         %" PRIu64 "\n", totalBytesWritten);

	output->output("------------------------------------------------------------------------\n");

	const double totalBytesReadDbl = (double) totalBytesRead;
	const double totalBytesWrittenDbl = (double) totalBytesWritten;
	const double secondsDbl = ((double) nanoSeconds) / 1000000000.0;

	char buffBWRead[32];
	sprintf(buffBWRead, "%f B/s", ((double) totalBytesReadDbl / secondsDbl));

	UnitAlgebra baBWRead(buffBWRead);

	output->output("- Bandwidth (read):                      %s\n",
		baBWRead.toStringBestSI().c_str());

	char buffBWWrite[32];
	sprintf(buffBWWrite, "%f B/s", ((double) totalBytesWrittenDbl / secondsDbl));

	UnitAlgebra uaBWWrite(buffBWWrite);

	output->output("- Bandwidth (written):                   %s\n",
		uaBWWrite.toStringBestSI().c_str());

	char buffBWCombined[32];
	sprintf(buffBWCombined, "%f B/s", ((double) (totalBytesReadDbl + totalBytesWrittenDbl) / secondsDbl));

	UnitAlgebra uaBWCombined(buffBWCombined);

	output->output("- Bandwidth (combined):                  %s\n", uaBWCombined.toStringBestSI().c_str());

	output->output("- Avr. Read request size:                %20.2f bytes\n",
		((double) PROSPERO_MAX(totalBytesRead, 1)) /
		((double) PROSPERO_MAX(readsIssued, 1)));
	output->output("- Avr. Write request size:               %20.2f bytes\n",
		((double) PROSPERO_MAX(totalBytesWritten, 1)) /
		((double) PROSPERO_MAX(writesIssued, 1)));
	output->output("- Avr. Request size:                     %20.2f bytes\n",
		((double) PROSPERO_MAX(totalBytesRead + totalBytesWritten, 1)) /
		((double) PROSPERO_MAX(readsIssued + writesIssued, 1)));
	output->output("\n");
}

void ProsperoComponent::handleResponse(SimpleMem::Request *ev) {
	output->verbose(CALL_INFO, 4, 0, "Handle response from memory subsystem.\n");

	currentOutstanding--;

	// Our responsibility to delete incoming event
	delete ev;
}

bool ProsperoComponent::tick(SST::Cycle_t currentCycle) {
	if(NULL == currentEntry) {
		output->verbose(CALL_INFO, 16, 0, "Prospero execute on cycle %" PRIu64 ", current entry is NULL, outstanding=%" PRIu32 ", maxOut=%" PRIu32 "\n",
			(uint64_t) currentCycle, currentOutstanding, maxOutstanding);
	} else {
		output->verbose(CALL_INFO, 16, 0, "Prospero execute on cycle %" PRIu64 ", current entry time: %" PRIu64 ", outstanding=%" PRIu32 ", maxOut=%" PRIu32 "\n",
			(uint64_t) currentCycle, (uint64_t) currentEntry->getIssueAtCycle(),
			currentOutstanding, maxOutstanding);
	}

	// If we have finished reading the trace we need to let the events in flight
	// drain and the system come to a rest
	if(traceEnded) {
		if(0 == currentOutstanding) {
			primaryComponentOKToEndSim();
                        return true;
		}

		return false;
	}

	const uint64_t outstandingBeforeIssue = currentOutstanding;

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
			output->verbose(CALL_INFO, 8, 0, "Not issuing on cycle %" PRIu64 ", waiting for cycle: %" PRIu64 "\n",
				(uint64_t) currentCycle, currentEntry->getIssueAtCycle());
			// Have reached a point in the trace which is too far ahead in time
			// so stall until we find that point
			break;
		}
	}

	const uint64_t outstandingAfterIssue = currentOutstanding;
	const uint64_t issuedThisCycle = outstandingAfterIssue - outstandingBeforeIssue;

	if(0 == issuedThisCycle) {
		cyclesWithNoIssue++;
	} else {
		cyclesWithIssue++;
	}

	// Keep simulation ticking, we have more work to do if we reach here
	return false;
}

void ProsperoComponent::issueRequest(const ProsperoTraceEntry* entry) {
	const uint64_t entryAddress = entry->getAddress();
	const uint64_t entryLength  = (uint64_t) entry->getLength();

	const uint64_t lineOffset   = entryAddress % cacheLineSize;
	bool  isRead                = entry->isRead();

	if(isRead) {
		totalBytesRead += entryLength;
	} else {
		totalBytesWritten += entryLength;
	}

	if(lineOffset + entryLength > cacheLineSize) {
		// Perform a split cache line load
		const uint64_t lowerLength = cacheLineSize - lineOffset;
		const uint64_t upperLength = entryLength - lowerLength;

		if(lowerLength + upperLength != entryLength) {
			output->fatal(CALL_INFO, -1, "Error: split cache line, lower size=%" PRIu64 ", upper size=%" PRIu64 " != request length: %" PRIu64 " (cache line %" PRIu64 ")\n",
				lowerLength, upperLength, entryLength, cacheLineSize);
		}
		assert(lowerLength + upperLength == entryLength);

		// Start split requests at the original requested address and then
		// also the the next cache line along
		const uint64_t lowerAddress = memMgr->translate(entryAddress);
		const uint64_t upperAddress = memMgr->translate((lowerAddress - (lowerAddress % cacheLineSize)) + cacheLineSize);

		SimpleMem::Request* reqLower = new SimpleMem::Request(
			isRead ? SimpleMem::Request::Read : SimpleMem::Request::Write,
			lowerAddress, lowerLength);
        reqLower->setVirtualAddress(entryAddress);

		SimpleMem::Request* reqUpper = new SimpleMem::Request(
			isRead ? SimpleMem::Request::Read : SimpleMem::Request::Write,
			upperAddress, upperLength);
        reqUpper->setVirtualAddress((entryAddress - (entryAddress % cacheLineSize)) + cacheLineSize);

		cache_link->sendRequest(reqLower);
		cache_link->sendRequest(reqUpper);

		if(isRead) {
			readsIssued += 2;
			splitReadsIssued++;
		} else {
			writesIssued += 2;
			splitWritesIssued++;
		}

		currentOutstanding++;
		currentOutstanding++;
	} else {
		// Perform a single load
		SimpleMem::Request* request = new SimpleMem::Request(
			isRead ? SimpleMem::Request::Read : SimpleMem::Request::Write,
			memMgr->translate(entryAddress), entryLength);
        request->setVirtualAddress(entryAddress);
		cache_link->sendRequest(request);

		if(isRead) {
			readsIssued++;
		} else {
			writesIssued++;
		}

		currentOutstanding++;
	}

	// Delete this entry, we are done converting it into a request
	delete entry;
}
