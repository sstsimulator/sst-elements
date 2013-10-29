
#include "arielcore.h"

ArielCore::ArielCore(int fd_in, SST::Link* coreToCacheLink, uint32_t thisCoreID, uint32_t maxPendTrans, Output* out, uint32_t maxIssuePerCyc, uint32_t maxQLen) {
	output = out;
	output->verbose(CALL_INFO, 2, 0, "Creating core with ID %" PRIu32 ", maximum queue length=%" PRIu32 ", max issue is: %" PRIu32 "\n", thisCoreID, maxQLen, maxIssuePerCyc);
	fd_input = fd_in;
	cacheLink = coreToCacheLink;
	coreID = thisCoreID;
	maxPendingTransactions = maxPendTrans;
	isHalted = false;
	maxIssuePerCycle = maxIssuePerCyc;
	maxQLength = maxQLen;

	coreQ = new std::queue<ArielEvent*>();
	pendingTransactions = new std::vector<MemEvent*>();
}

ArielCore::~ArielCore() {

}

void ArielCore::setCacheLink(SST::Link* newLink) {
	cacheLink = newLink;
}

void ArielCore::handleEvent(SST::Event* event) {
	
}

void ArielCore::halt() {
	isHalted = true;
}

void ArielCore::closeInput() {
	close(fd_input);
}

void ArielCore::createReadEvent(uint64_t address, uint32_t length) {
	ArielReadEvent* ev = new ArielReadEvent(address, length);
	coreQ->push(ev);
	
	output->verbose(CALL_INFO, 4, 0, "Generated a READ event, addr=%" PRIu64 ", length=%" PRIu32 "\n", address, length);
}

void ArielCore::createWriteEvent(uint64_t address, uint32_t length) {
	ArielWriteEvent* ev = new ArielWriteEvent(address, length);
	coreQ->push(ev);
	
	output->verbose(CALL_INFO, 4, 0, "Generated a WRITE event, addr=%" PRIu64 ", length=%" PRIu32 "\n", address, length);
}

bool ArielCore::isCoreHalted() {
	return isHalted;
}

bool ArielCore::refillQueue() {
	output->verbose(CALL_INFO, 16, 0, "Refilling event queue for core %" PRIu32 "...\n", coreID);

	struct pollfd poll_input;
	poll_input.fd = fd_input;
	poll_input.events = POLLIN;
	bool added_data = false;

	while(coreQ->size() < maxQLength) {
		output->verbose(CALL_INFO, 16, 0, "Attempting to fill events for core: %" PRIu32 " current queue size=%" PRIu32 ", max length=%" PRIu32 "\n",
			coreID, (uint32_t) coreQ->size(), (uint32_t) maxQLength);
			
		int poll_result = poll(&poll_input, (unsigned int) 1, (int) 20);
		if(poll_result == -1) {
			output->fatal(CALL_INFO, -2, 0, 0, "Attempt to poll failed.\n");
			break;
		}

		if(poll_input.revents & POLLIN) {
			output->verbose(CALL_INFO, 1, 0, "READ DATA ON CORE: %" PRIu32 "\n", coreID);
			// There is data on the pipe
			added_data = true;

			uint8_t command = 0;
			read(fd_input, &command, sizeof(command));

			switch(command) {
			case ARIEL_START_INSTRUCTION:
				uint64_t op_addr;
				uint32_t op_size;

				while(command != ARIEL_END_INSTRUCTION) {
					read(fd_input, &command, sizeof(command));

					switch(command) {
					case ARIEL_PERFORM_READ:
						read(fd_input, &op_addr, sizeof(op_addr));
						read(fd_input, &op_size, sizeof(op_size));

						createReadEvent(op_addr, op_size);
						break;

					case ARIEL_PERFORM_WRITE:
						read(fd_input, &op_addr, sizeof(op_addr));
						read(fd_input, &op_size, sizeof(op_size));

						createWriteEvent(op_addr, op_size);
						break;

					case ARIEL_END_INSTRUCTION:
						break;

					default:
						// Not sure what this is
						assert(0);
						break;
					}
				}
				break;

			case ARIEL_PERFORM_EXIT:
				break;
			}
		} else {
			return added_data;
		}
	}

	output->verbose(CALL_INFO, 16, 0, "Refilling event queue for core %" PRIu32 " is complete, added data? %s\n", coreID,
		(added_data ? "yes" : "no"));
	return added_data;
}

bool ArielCore::processNextEvent() {
	// Attempt to refill the queue
	if(coreQ->empty()) {
		bool addedItems = refillQueue();

		output->verbose(CALL_INFO, 16, 0, "Attempted a queue fill, %s data\n",
			(addedItems ? "added" : "did not add"));

		if(! addedItems) {
			return false;
		}
	}
	
	output->verbose(CALL_INFO, 8, 0, "Processing next event in core %" PRIu32 "...\n", coreID);

	ArielEvent* nextEvent = coreQ->front();
	bool removeEvent = false;

	switch(nextEvent->getEventType()) {
	case READ_ADDRESS:
		removeEvent = true;
		break;

	case WRITE_ADDRESS:
		removeEvent = true;
		break;

	case START_DMA_TRANSFER:
		break;

	case WAIT_ON_DMA_TRANSFER:
		break;

	case CORE_EXIT:
		isHalted = true;
		return true;

	default:
		output->fatal(CALL_INFO, -4, 0, 0, "Unknown event type has arrived on core %" PRIu32 "\n", coreID);
		break;
	}

	// If the event has actually been processed this cycle then remove it from the queue
	if(removeEvent) {
		output->verbose(CALL_INFO, 8, 0, "Removing event from pending queue, there are %" PRIu32 " events in the queue before deletion.\n", 
			(uint32_t) coreQ->size());
		coreQ->pop();
		return true;
	} else {
		return false;
	}
}

void ArielCore::tick() {
	if(! isHalted) {
		output->verbose(CALL_INFO, 16, 0, "Ticking core id %" PRIu32 "\n", coreID);
		for(uint32_t i = 0; i < maxIssuePerCycle; ++i) {
			output->verbose(CALL_INFO, 16, 0, "Issuing event %" PRIu32 " out of max issue: %" PRIu32 "...\n", i, maxIssuePerCycle);
			bool didProcess = processNextEvent();

			if(didProcess || isHalted) {
				break;
			}
		}
	}
}
