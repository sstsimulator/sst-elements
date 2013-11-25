
#ifndef _H_SST_ARIEL_CORE
#define _H_SST_ARIEL_CORE

#include <sst_config.h>
#include <sst/core/event.h>
#include <sst/core/sst_types.h>
#include <sst/core/component.h>
#include <sst/core/link.h>
#include <sst/core/timeConverter.h>
#include <sst/core/output.h>
#include <sst/core/interfaces/memEvent.h>
#include <sst/core/element.h>

#include <stdio.h>
#include <stdint.h>
#include <poll.h>

#include <string>
#include <queue>
#include <map>


#include "arielmemmgr.h"
#include "arielevent.h"
#include "arielreadev.h"
#include "arielwriteev.h"
#include "arielexitev.h"
#include "arielallocev.h"
#include "arielfreeev.h"

using namespace SST;
using namespace SST::Interfaces;
using namespace SST::ArielComponent;

namespace SST {
namespace ArielComponent {

#define ARIEL_PERFORM_EXIT 1
#define ARIEL_PERFORM_READ 2
#define ARIEL_PERFORM_WRITE 4
#define ARIEL_START_DMA 8
#define ARIEL_WAIT_DMA 16
#define ARIEL_ISSUE_TLM_MAP 80
#define ARIEL_ISSUE_TLM_FREE 100
#define ARIEL_START_INSTRUCTION 32
#define ARIEL_END_INSTRUCTION 64

class ArielCore {

	public:
		ArielCore(int fd_in, SST::Link* coreToCacheLink, uint32_t thisCoreID, uint32_t maxPendTans, 
			Output* out, uint32_t maxIssuePerCyc, uint32_t maxQLen, int pipeTimeO, 
			uint64_t cacheLineSz, SST::Component* owner,
			ArielMemoryManager* memMgr);
		~ArielCore();
		bool isCoreHalted();
		void tick();
		void closeInput();
		void halt();
		void createReadEvent(uint64_t addr, uint32_t size);
		void createWriteEvent(uint64_t addr, uint32_t size);
		void createAllocateEvent(uint64_t vAddr, uint64_t length, uint32_t level);
		void createFreeEvent(uint64_t vAddr);
		void createExitEvent();
		void setCacheLink(SST::Link* newCacheLink);
		void handleEvent(SST::Event* event);
		void handleReadRequest(ArielReadEvent* wEv);
		void handleWriteRequest(ArielWriteEvent* wEv);
		void handleAllocationEvent(ArielAllocateEvent* aEv);
		void handleFreeEvent(ArielFreeEvent* aFE);
		void printCoreStatistics();

	private:
		bool processNextEvent();
		bool refillQueue();
		uint32_t coreID;
		uint32_t maxPendingTransactions;
		Output* output;
		std::queue<ArielEvent*>* coreQ;
		bool isHalted;
		SST::Link* cacheLink;
		int fd_input;
		std::map<MemEvent::id_type, MemEvent*>* pendingTransactions;
		uint32_t maxIssuePerCycle;
		uint32_t maxQLength;
		int readPipeTimeOut;
		uint64_t cacheLineSize;
		SST::Component* owner;
		ArielMemoryManager* memmgr;
		
		uint64_t read_requests;
		uint64_t write_requests;
		uint64_t split_read_requests;
		uint64_t split_write_requests;

};

}
}

#endif
