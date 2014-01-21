
#ifndef _H_SST_ARIEL_CORE
#define _H_SST_ARIEL_CORE

#include <sst/core/event.h>
#include <sst/core/sst_types.h>
#include <sst/core/component.h>
#include <sst/core/link.h>
#include <sst/core/output.h>
#include <sst/core/interfaces/memEvent.h>
#include <sst/core/element.h>

#include <sst/core/simulation.h>
#include <sst/core/timeConverter.h>
#include <sst/core/timeLord.h>

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
#include "arielnoop.h"
#include "arielswitchpool.h"

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
#define ARIEL_NOOP 128
#define ARIEL_SWITCH_POOL 110

class ArielCore {

	public:
		ArielCore(int fd_in, SST::Link* coreToCacheLink, uint32_t thisCoreID, uint32_t maxPendTans,
			Output* out, uint32_t maxIssuePerCyc, uint32_t maxQLen, int pipeTimeO,
			uint64_t cacheLineSz, SST::Component* owner,
			ArielMemoryManager* memMgr, const uint32_t perform_address_checks, const std::string tracePrefix);
		~ArielCore();
		bool isCoreHalted();
		void tick();
		void closeInput();
		void halt();
		void finishCore();
		void createReadEvent(uint64_t addr, uint32_t size);
		void createWriteEvent(uint64_t addr, uint32_t size);
		void createAllocateEvent(uint64_t vAddr, uint64_t length, uint32_t level);
		void createNoOpEvent();
		void createFreeEvent(uint64_t vAddr);
		void createExitEvent();
		void createSwitchPoolEvent(uint32_t pool);

		void setCacheLink(SST::Link* newCacheLink);
		void handleEvent(SST::Event* event);
		void handleReadRequest(ArielReadEvent* wEv);
		void handleWriteRequest(ArielWriteEvent* wEv);
		void handleAllocationEvent(ArielAllocateEvent* aEv);
		void handleFreeEvent(ArielFreeEvent* aFE);
		void handleSwitchPoolEvent(ArielSwitchPoolEvent* aSPE);

		void commitReadEvent(const uint64_t address, const uint32_t length);
		void commitWriteEvent(const uint64_t address, const uint32_t length);

		void printCoreStatistics();
		void printTraceEntry(const bool isRead, const uint64_t address, const uint32_t length);

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
		uint32_t verbosity;
		const uint32_t perform_checks;
		const bool enableTracing;
		FILE* traceFile;
		TimeConverter* picoTimeConv;

		uint64_t pending_transaction_count;
		uint64_t read_requests;
		uint64_t write_requests;
		uint64_t split_read_requests;
		uint64_t split_write_requests;
		uint64_t noop_count;

};

}
}

#endif
