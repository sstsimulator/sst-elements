
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


#include "arielevent.h"
#include "arielreadev.h"
#include "arielwriteev.h"

using namespace SST;
using namespace SST::Interfaces;
using namespace SST::ArielComponent;

namespace SST {
namespace ArielComponent {

#define ARIEL_PERFORM_EXIT 1
#define ARIEL_PERFORM_READ 2
#define ARIEL_PERFORM_WRITE 4
#define ARIEL_START_DMA 8  // Format: Destination:64 Source:64 Size:32 Tag:32
#define ARIEL_WAIT_DMA 16  // Format: Tag:32
#define ARIEL_ISSUE_TLM_MAP 80 // issue a Two level memory allocation
#define ARIEL_ISSUE_TLM_FREE 100
#define ARIEL_START_INSTRUCTION 32
#define ARIEL_END_INSTRUCTION 64

class ArielCore {

	public:
		ArielCore(int fd_in, SST::Link* coreToCacheLink, uint32_t thisCoreID, uint32_t maxPendTans, Output* out, uint32_t maxIssuePerCyc, uint32_t maxQLen);
		~ArielCore();
		bool isCoreHalted();
		void tick();
		void closeInput();
		void halt();
		void createReadEvent(uint64_t addr, uint32_t size);
		void createWriteEvent(uint64_t addr, uint32_t size);
		void setCacheLink(SST::Link* newCacheLink);
		void handleEvent(SST::Event* event);

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
		std::vector<MemEvent*>* pendingTransactions;
		uint32_t maxIssuePerCycle;
		uint32_t maxQLength;

};

}
}

#endif
