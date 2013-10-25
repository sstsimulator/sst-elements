#include "arielcore.h"

ArielCore::ArielCore(int fd_in, SST::Link* coreToCacheLink, uint32_t thisCoreID, uint32_t maxPendTrans, Output* out, uint32_t maxIssuePerCyc, uint32_t maxQLen) {
	output = out;
	fd_input = fd_in;
	cacheLink = coreToCacheLink;
	coreID = thisCoreID;
	maxPendingTransactions = maxPendTrans;
	isHalted = false;

	coreQ = new std::queue<ArielEvent*>();
	pendingTransactions = new std::vector<MemEvent*>();
}

ArielCore::~ArielCore() {

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
}

void ArielCore::createWriteEvent(uint64_t address, uint32_t length) {
	ArielWriteEvent* ev = new ArielWriteEvent(address, length);
	coreQ->push(ev);
}

bool ArielCore::refillQueue() {
	output->verbose(CALL_INFO, 16, 0, "Refilling event queue for core %" PRIu32 "...\n", coreID);

	struct pollfd poll_input;
	poll_input.fd = fd_input;
	poll_input.events = POLLIN;
	bool added_data = false;

	while(coreQ->size() < maxQLength) {
		poll(&poll_input, (unsigned int) 1, 10);

		if(poll_input.revents & POLLIN) {
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

		if(! addedItems) {
			return false;
		}
	}

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
		coreQ->pop();
		return true;
	} else {
		return false;
	}
}

void ArielCore::tick() {
	if(! isHalted) {
		for(int i = 0; i < maxIssuePerCycle; ++i) {
			bool didProcess = processNextEvent();

			if(didProcess || isHalted) {
				break;
			}
		}
	}
}
