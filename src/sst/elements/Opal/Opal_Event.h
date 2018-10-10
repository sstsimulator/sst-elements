// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2018, NTESS
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


#include <sst_config.h>
#include <sst/core/component.h>
#include <sst/core/timeConverter.h>
#include <../memHierarchy/memEvent.h>
#include<map>
#include<list>
#include<string>


using namespace SST; 


namespace SST{ namespace OpalComponent{

	enum EventType { HINT, MMAP, REQUEST, RESPONSE, UNMAP, UMAPACK, SHOOTDOWN, INVALIDADDR, SDACK};
	enum MemType { LOCAL, SHARED };
	enum MemTech { DRAM, NVM, HBM, HMC, SCRATCHPAD, BURSTBUFFER};
//	enum HintType { DRAM, NVM, HBM, HMC, SCRATCHPAD, BURSTBUFFER, };

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
			int size;
			int nodeId;
			int coreId;
			MemType memType;
			int shootdownId;
			int hint;
			int fileId;

		public:

			OpalEvent(EventType y) : SST::Event()
		{ ev = y; memType = SST::OpalComponent::MemType::LOCAL;}

			void setType(int ev1) { ev = static_cast<EventType>(ev1);}
			int getType() { return ev; }
			
			void setMemType(int mtype) { memType = static_cast<MemType>(mtype);}
			MemType getMemType() { return memType; }

			void setNodeId(int id) { nodeId = id; }
			int getNodeId() { return nodeId; }

			void setCoreId(int id) { coreId = id; }
			int getCoreId() { return coreId; }
			
			void setResp(uint64_t add, uint64_t padd, int sz) { address = add; paddress = padd; size = sz;}

			void setAddress(uint64_t add) { address = add; }
			uint64_t getAddress() { return address; }

			uint64_t getPaddress() { return paddress; }

			int getSize() { return size; }

			void setFaultLevel(int level) { faultLevel = level; }
			int getFaultLevel() { return faultLevel; }

			void setShootdownId(int id) { shootdownId = id; }
			int getShootdownId() { return shootdownId; }

			void setFileId(int id) { fileId = id; }
			int getFileId() { return fileId; }

			void setHint(int x) { hint = x; }
			int getHint() { return hint; }

			void serialize_order(SST::Core::Serialization::serializer &ser) override{
				Event::serialize_order(ser);
				ser & ev;
				ser & address;
				ser & paddress;
				ser & size;
				ser & nodeId;
				ser & coreId;
				ser & memType;
				ser & shootdownId;
				ser & hint;
				ser & fileId;
			}


		ImplementSerializable(OpalEvent);

	};


}}

#endif

