// Copyright 2009-2017 Sandia Corporation. Under the terms
// of Contract DE-NA0003525 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2017, Sandia Corporation
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

#ifndef _H_SST_OPAL_EVENT
#define _H_SST_OPAL_EVENT


#include <sst_config.h>
#include <sst/core/component.h>
#include <sst/core/timeConverter.h>
#include <sst/elements/memHierarchy/memEvent.h>
#include<map>
#include<list>


using namespace SST; 


namespace SST{ namespace OpalComponent{

	enum EventType { REQUEST, RESPONSE, UNMAP, UMAPACK, SHOOTDOWN, SDACK};

	// Thie defines a class for events of Opal
	class OpalEvent : public SST::Event
	{

		private:
		OpalEvent() { } // For serialization

			int ev;
			long long int address;
			long long int paddress;
			int size;
		public:

			OpalEvent(EventType y) : SST::Event()
		{ ev = y;}

			void setType(int ev1) { ev = static_cast<EventType>(ev1);}
			int getType() { return ev; }
			
			void setResp(long long int add, long long int padd, int sz) { address = add; paddress = padd; size = sz;}
			long long int getAddress() { return address; }
			long long int getPaddress() { return paddress; }

			int getSize() { return size; }

			void serialize_order(SST::Core::Serialization::serializer &ser) {
				Event::serialize_order(ser);
			}


		ImplementSerializable(OpalEvent);

	};


}}

#endif
