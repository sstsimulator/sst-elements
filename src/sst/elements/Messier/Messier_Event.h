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

#ifndef _H_SST_MESSIER_EVENT
#define _H_SST_MESSIER_EVENT


#include <sst_config.h>
#include <sst/core/component.h>
#include <sst/core/timeConverter.h>
#include <sst/elements/memHierarchy/memEvent.h>
#include "NVM_Request.h"
#include<map>
#include<list>


using namespace SST; 


namespace SST{ namespace MessierComponent{

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Winconsistent-missing-override"
#endif
    
	enum EventType { READ_COMPLETION, WRITE_COMPLETION, DEVICE_READY, HIT_MISS, INVALIDATE_WRITE};

	// Thie defines a class for events of Messier
	class MessierEvent : public SST::Event
	{

		private:
		MessierEvent() { } // For serialization

			int ev;
			NVM_Request * req;		
		public:

			MessierEvent(NVM_Request * x, EventType y) : SST::Event()
		{ ev = y; req = x;}

			void setType(int ev1) { ev = static_cast<EventType>(ev1);}
			int getType() { return ev; }
			
			void setReq(NVM_Request * tmp) { req = tmp;}
			NVM_Request * getReq() { return req; }

			// Pointer to the NVM_Request initiated this event

			void serialize_order(SST::Core::Serialization::serializer &ser) {
				Event::serialize_order(ser);
			}

				// This indicates the event type


		//ImplementSerializable(MemRespEvent);

		ImplementSerializable(MessierEvent);

	};

#if defined(__clang__)
#pragma clang diagnostic pop
#endif

}}

#endif
