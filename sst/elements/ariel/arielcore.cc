
#include "arielcore.h"

ArielCore::ArielCore(int fd_in, SST::Link* coreToCacheLink, uint32_t thisCoreID, 
	uint32_t maxPendTrans, Output* out, uint32_t maxIssuePerCyc, 
	uint32_t maxQLen, int pipeTO, uint64_t cacheLineSz, SST::Component* own,
			ArielMemoryManager* memMgr) {
	
	output = out;
	output->verbose(CALL_INFO, 2, 0, "Creating core with ID %" PRIu32 ", maximum queue length=%" PRIu32 ", max issue is: %" PRIu32 "\n", thisCoreID, maxQLen, maxIssuePerCyc);
	fd_input = fd_in;
	cacheLink = coreToCacheLink;
	coreID = thisCoreID;
	maxPendingTransactions = maxPendTrans;
	isHalted = false;
	maxIssuePerCycle = maxIssuePerCyc;
	maxQLength = maxQLen;
	readPipeTimeOut = pipeTO;
	cacheLineSize = cacheLineSz;
	owner = own;
	memmgr = memMgr;

	coreQ = new std::queue<ArielEvent*>();
	pendingTransactions = new std::map<MemEvent::id_type, MemEvent*>();
}

ArielCore::~ArielCore() {

}

void ArielCore::setCacheLink(SST::Link* newLink) {
	cacheLink = newLink;
}

void ArielCore::handleEvent(SST::Event* event) {
	output->verbose(CALL_INFO, 4, 0, "Core %" PRIu32 " handling an event...\n", coreID);
	MemEvent* memEv = dynamic_cast<MemEvent*>(event);
	
	if(memEv) {
		output->verbose(CALL_INFO, 4, 0, "Mapped successfully to a memory event.\n");
		
		// Ignore invalidation requests at the core
		if( memEv->getCmd() == Invalidate ) {
			delete event;
			return;
		}
		
		MemEvent::id_type mev_id = memEv->getResponseToID();
		std::map<MemEvent::id_type, MemEvent*>::iterator find_entry = pendingTransactions->find(mev_id);
		
		if(find_entry != pendingTransactions->end()) {
			output->verbose(CALL_INFO, 4, 0, "Correctly identified event in pending transactions, removing from list leaving: %" PRIu32 " transactions\n",
				(uint32_t) pendingTransactions->size());
				
			pendingTransactions->erase(find_entry);
			delete memEv;
		} else {
			output->fatal(CALL_INFO, -4, 0, 0, "Memory event response to core: %" PRIu32 " was not found in pending list.\n", coreID);
		}
	}
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

void ArielCore::createExitEvent() {
	ArielExitEvent* xEv = new ArielExitEvent();
	coreQ->push(xEv);
	
	output->verbose(CALL_INFO, 4, 0, "Generated an EXIT event.\n");
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
			
		int poll_result = poll(&poll_input, (unsigned int) 1, (int) readPipeTimeOut);
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
				createExitEvent();
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

void ArielCore::handleReadRequest(ArielReadEvent* rEv) {
	output->verbose(CALL_INFO, 4, 0, "Core %" PRIu32 " processing a read event...\n", coreID);
	
	const uint64_t readAddress = rEv->getAddress();
	const uint32_t readLength  = rEv->getLength();
	
	const uint64_t addr_offset  = readAddress % cacheLineSize;
	
	if(addr_offset + readLength < cacheLineSize) {
		output->verbose(CALL_INFO, 4, 0, "Core %" PRIu32 " generating a non-split read request: Addr=%" PRIu64 " Length=%" PRIu32 "\n",
			coreID, readAddress, readLength);
	
		// We do not need to perform a split operation
		const uint64_t physAddr = memmgr->translateAddress(readAddress);
		
		output->verbose(CALL_INFO, 4, 0, "Core %" PRIu32 " issuing read, VAddr=%" PRIu64 ", Size=%" PRIu32 ", PhysAddr=%" PRIu64 "\n", 
			coreID, readAddress, readLength, physAddr);
			
		MemEvent* memEvent = new MemEvent(owner, (Addr) physAddr, ReadReq);
		memEvent->setSize(readLength);

		pendingTransactions->insert( std::pair<MemEvent::id_type, MemEvent*>(memEvent->getID(), memEvent) );
				
		// Actually send the event to the cache
		cacheLink->send(memEvent);
	} else {
		output->verbose(CALL_INFO, 4, 0, "Core %" PRIu32 " generating a split read request: Addr=%" PRIu64 " Length=%" PRIu32 "\n",
			coreID, readAddress, readLength);
	
		// We need to perform a split operation
		const uint64_t leftAddr = readAddress;
		const uint32_t leftSize = cacheLineSize - addr_offset;
		
		const uint64_t rightAddr = (readAddress - addr_offset) + cacheLineSize;
		const uint32_t rightSize = (readAddress + readLength) % cacheLineSize;
		
		const uint64_t physLeftAddr = memmgr->translateAddress(leftAddr);
		const uint64_t physRightAddr = memmgr->translateAddress(rightAddr);
		
		output->verbose(CALL_INFO, 4, 0, "Core %" PRIu32 " issuing split-address read, LeftVAddr=%" PRIu64 ", RightVAddr=%" PRIu64 ", LeftSize=%" PRIu32 ", RightSize=%" PRIu32 ", LeftPhysAddr=%" PRIu64 ", RightPhysAddr=%" PRIu64 "\n", 
			coreID, leftAddr, rightAddr, leftSize, rightSize, physLeftAddr, physRightAddr);
				
		MemEvent* leftEvent  = new MemEvent(owner, (Addr) physLeftAddr, ReadReq);
		MemEvent* rightEvent = new MemEvent(owner, (Addr) physRightAddr, ReadReq);
		
		pendingTransactions->insert( std::pair<MemEvent::id_type, MemEvent*>(leftEvent->getID(), leftEvent) );
		pendingTransactions->insert( std::pair<MemEvent::id_type, MemEvent*>(rightEvent->getID(), rightEvent) );
		
		cacheLink->send(leftEvent);
		cacheLink->send(rightEvent);			
	}
}

void ArielCore::handleWriteRequest(ArielWriteEvent* wEv) {
	output->verbose(CALL_INFO, 4, 0, "Core %" PRIu32 " processing a write event...\n", coreID);
	
	const uint64_t writeAddress = wEv->getAddress();
	const uint32_t writeLength  = wEv->getLength();
	
	const uint64_t addr_offset  = writeAddress % cacheLineSize;
	
	if(addr_offset + writeLength < cacheLineSize) {
		output->verbose(CALL_INFO, 4, 0, "Core %" PRIu32 " generating a non-split write request: Addr=%" PRIu64 " Length=%" PRIu32 "\n",
			coreID, writeAddress, writeLength);
	
		// We do not need to perform a split operation
		const uint64_t physAddr = (Addr) memmgr->translateAddress(writeAddress);
		
		output->verbose(CALL_INFO, 4, 0, "Core %" PRIu32 " issuing write, VAddr=%" PRIu64 ", Size=%" PRIu32 ", PhysAddr=%" PRIu64 "\n", 
			coreID, writeAddress, writeLength, physAddr);
			
		MemEvent* memEvent = new MemEvent(owner, (Addr) physAddr, WriteReq);
		memEvent->setSize(writeLength);

		pendingTransactions->insert( std::pair<MemEvent::id_type, MemEvent*>(memEvent->getID(), memEvent) );
				
		// Actually send the event to the cache
		cacheLink->send(memEvent);
	} else {
		output->verbose(CALL_INFO, 4, 0, "Core %" PRIu32 " generating a split write request: Addr=%" PRIu64 " Length=%" PRIu32 "\n",
			coreID, writeAddress, writeLength);
	
		// We need to perform a split operation
		const uint64_t leftAddr = writeAddress;
		const uint32_t leftSize = cacheLineSize - addr_offset;
		
		const uint64_t rightAddr = (writeAddress - addr_offset) + cacheLineSize;
		const uint32_t rightSize = (writeAddress + writeLength) % cacheLineSize;
		
		const uint64_t physLeftAddr = memmgr->translateAddress(leftAddr);
		const uint64_t physRightAddr = memmgr->translateAddress(rightAddr);
		
		output->verbose(CALL_INFO, 4, 0, "Core %" PRIu32 " issuing split-address write, LeftVAddr=%" PRIu64 ", RightVAddr=%" PRIu64 ", LeftSize=%" PRIu32 ", RightSize=%" PRIu32 ", LeftPhysAddr=%" PRIu64 ", RightPhysAddr=%" PRIu64 "\n", 
			coreID, leftAddr, rightAddr, leftSize, rightSize, physLeftAddr, physRightAddr);
				
		MemEvent* leftEvent  = new MemEvent(owner, (Addr) physLeftAddr, WriteReq);
		MemEvent* rightEvent = new MemEvent(owner, (Addr) physRightAddr, WriteReq);
		
		pendingTransactions->insert( std::pair<MemEvent::id_type, MemEvent*>(leftEvent->getID(), leftEvent) );
		pendingTransactions->insert( std::pair<MemEvent::id_type, MemEvent*>(rightEvent->getID(), rightEvent) );
		
		cacheLink->send(leftEvent);
		cacheLink->send(rightEvent);			
	}
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
		if(pendingTransactions->size() < maxPendingTransactions) {
			output->verbose(CALL_INFO, 16, 0, "Found a read event, fewer pending transactions than permitted so will process...\n");
			removeEvent = true;
			handleReadRequest(dynamic_cast<ArielReadEvent*>(nextEvent));
		} else {
			output->verbose(CALL_INFO, 16, 0, "Pending transaction queue is currently full for core %" PRIu32 ", core will stall for new events\n", coreID);
			break;
		}
		break;

	case WRITE_ADDRESS:
		if(pendingTransactions->size() < maxPendingTransactions) {
			output->verbose(CALL_INFO, 16, 0, "Found a write event, fewer pending transactions than permitted so will process...\n");
			removeEvent = true;
			handleWriteRequest(dynamic_cast<ArielWriteEvent*>(nextEvent));
		} else {
			output->verbose(CALL_INFO, 16, 0, "Pending transaction queue is currently full for core %" PRIu32 ", core will stall for new events\n", coreID);
			break;
		}
		break;

	case START_DMA_TRANSFER:
		removeEvent = true;
		break;

	case WAIT_ON_DMA_TRANSFER:
		removeEvent = true;
		break;

	case CORE_EXIT:
		isHalted = true;
		std::cout << "CORE ID: " << coreID << " PROCESSED AN EXIT EVENT" << std::endl;
		output->verbose(CALL_INFO, 2, 0, "Core %" PRIu32 " has called exit.\n", coreID);
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
		output->verbose(CALL_INFO, 8, 0, "Event removal was not requested, pending transaction queue length=%" PRIu32 ", maximum transactions: %" PRIu32 "\n",
			(uint32_t)pendingTransactions->size(), maxPendingTransactions);
		return false;
	}
}

void ArielCore::tick() {
	if(! isHalted) {
		output->verbose(CALL_INFO, 16, 0, "Ticking core id %" PRIu32 "\n", coreID);
		for(uint32_t i = 0; i < maxIssuePerCycle; ++i) {
			output->verbose(CALL_INFO, 16, 0, "Issuing event %" PRIu32 " out of max issue: %" PRIu32 "...\n", i, maxIssuePerCycle);
			bool didProcess = processNextEvent();

			// If we didnt process anything in the call or we have halted then 
			// we stop the ticking and return
			if( (!didProcess) || isHalted) {
				break;
			}
		}
	}
}
