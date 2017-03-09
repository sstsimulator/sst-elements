// Copyright 2009-2016 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2016, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.
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

	enum EventType { READ_COMPLETION, WRITE_COMPLETION, DEVICE_READY};

	// Thie defines a class for events of Messier
	class MessierEvent : public SST::Event
	{

		private:
		MessierEvent() { } // For serialization

		public:

			MessierEvent(NVM_Request * x, EventType y) : SST::Event()
		{ ev = y; req = x;}

			// This indicates the event type
			EventType ev;

			// Pointer to the NVM_Request initiated this event
			NVM_Request * req;		
			void serialize_order(SST::Core::Serialization::serializer &ser) {
				Event::serialize_order(ser);
			}

			//ImplementSerializable(MemRespEvent);

		ImplementSerializable(MessierEvent);

	};


}}

#endif
