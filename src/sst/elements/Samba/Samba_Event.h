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

#ifndef _H_SST_MESSIER_EVENT
#define _H_SST_MESSIER_EVENT


#include <sst_config.h>
#include <sst/core/component.h>
#include <sst/core/timeConverter.h>
#include <sst/elements/memHierarchy/memEvent.h>
#include<map>
#include<list>


using namespace SST;


namespace SST{ namespace SambaComponent{

	enum EventType { PAGE_FAULT, PAGE_FAULT_RESPONSE, PAGE_FAULT_SERVED, SHOOTDOWN, PAGE_REFERENCE, SDACK };

	// Thie defines a class for events of Samba
	class SambaEvent : public SST::Event
	{

		private:
		SambaEvent() { } // For serialization

			int ev;
			uint64_t address;
			uint64_t paddress;
			uint64_t size;
		public:

			SambaEvent(EventType y) : SST::Event()
		{ ev = y;}

			void setType(int ev1) { ev = static_cast<EventType>(ev1);}
			int getType() { return ev; }

			void setResp(uint64_t add, uint64_t padd, uint64_t sz) { address = add; paddress = padd; size = sz;}
			uint64_t getAddress() { return address; }
			uint64_t getPaddress() { return paddress; }
			uint64_t getSize() { return size; }

			void serialize_order(SST::Core::Serialization::serializer &ser) override {
				Event::serialize_order(ser);
			}


		ImplementSerializable(SambaEvent);

	};


}}

#endif
