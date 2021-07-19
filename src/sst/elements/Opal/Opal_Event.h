// Copyright 2009-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2021, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.
//


//
/* Author: Amro Awad
 * E-mail: aawad@sandia.gov
 */
/* Author: Vamsee Reddy Kommareddy
 * E-mail: vamseereddy@knights.ucf.edu
 */

#ifndef _H_SST_OPAL_EVENT
#define _H_SST_OPAL_EVENT

#include <sst/core/component.h>
#include <sst/core/timeConverter.h>
#include <../memHierarchy/memEvent.h>

#include <map>
#include <list>
#include <string>

using namespace SST;


namespace SST{ namespace OpalComponent{

	enum EventType { HINT, MMAP, REQUEST, RESPONSE, UNMAP, UMAPACK, SHOOTDOWN, REMAP, SDACK, ARIEL_ENABLED, HALT, PAGE_REFERENCE, PAGE_REFERENCE_END, IPC_INFO };
	enum MemType { LOCAL, SHARED };
	enum MemTech { DRAM, NVM, HBM, HMC, SCRATCHPAD, BURSTBUFFER};

// **************** Important *****************
//	Levels hints are: 0 for DRAM
//	1: NVM
//	2: HBM
//	3: HMC
//	4: SCRATCHPAD
//	5: BURSTBUFFER

	// Thie defines a class for events of Opal
	class OpalEvent : public SST::Event
	{

		private:
		OpalEvent() {} // For serialization

			EventType ev;
			uint64_t address;
			uint64_t paddress;
			int faultLevel;
			int size; // to redcue packet size, size valiable we used size variable for multiple pusrposes. 1. size of the page fault. 2. shoowdownID. 3. local and global page reference
			uint32_t nodeId;
			uint32_t coreId;
			MemType memType;
			int hint;
			int fileId;
			int memContrlId;
			bool invalidate;

		public:

			OpalEvent(EventType y) : SST::Event()
		{ ev = y; memType = SST::OpalComponent::MemType::LOCAL; invalidate = false;}

			OpalEvent(EventType y, const uint32_t level, const uint64_t virtualAddress, const uint64_t size_, const uint32_t thread) : SST::Event()
			{
				ev = y;
				memType = SST::OpalComponent::MemType::LOCAL;
				invalidate = false;
				size =size_;
				faultLevel = level;
				address = virtualAddress;
				coreId = thread;
			}

			void setType(int ev1) { ev = static_cast<EventType>(ev1);}
			int getType() { return ev; }

			void setMemType(int mtype) { memType = static_cast<MemType>(mtype);}
			MemType getMemType() { return memType; }

			void setNodeId(uint32_t id) { nodeId = id; }
			uint32_t getNodeId() { return nodeId; }

			void setCoreId(uint32_t id) { coreId = id; }
			uint32_t getCoreId() { return coreId; }

			void setResp(uint64_t add, uint64_t padd, int sz) { address = add; paddress = padd; size = sz;}

			void setAddress(uint64_t add) { address = add; }
			uint64_t getAddress() { return address; }

			void setPAddress(uint64_t add) { paddress = add; }
			uint64_t getPaddress() { return paddress; }

			void setSize(int size_) { size = size_; }
			int getSize() { return size; }

			void setFaultLevel(int level) { faultLevel = level; }
			int getFaultLevel() { return faultLevel; }

			void setInvalidate() { invalidate = true; }
			bool getInvalidate() { return invalidate; }

			void setFileId(int id) { fileId = id; }
			int getFileId() { return fileId; }

			void setHint(int x) { hint = x; }
			int getHint() { return hint; }

			void setMemContrlId(int id) { memContrlId = id; }
			int getMemContrlId() { return memContrlId; }

			void serialize_order(SST::Core::Serialization::serializer &ser) override {
				Event::serialize_order(ser);
				ser & ev;
				ser & address;
				ser & paddress;
                ser & faultLevel;
				ser & size;
				ser & nodeId;
				ser & coreId;
				ser & memType;
				ser & hint;
				ser & fileId;
				ser & memContrlId;
                ser & invalidate;
			}


		ImplementSerializable(OpalEvent);

	};


}}

#endif

