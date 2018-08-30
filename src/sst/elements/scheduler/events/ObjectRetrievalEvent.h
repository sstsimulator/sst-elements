// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2018, NTESS
// All rights reserved.
// 
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

/*
 * Used to request an object via an event.  Useful if more direct communication
 * is required between two Components to organize some meta-simulation event.
 */

#ifndef SST_SCHEDULER_OBJECTRETRIEVALEVENT_H__
#define SST_SCHEDULER_OBJECTRETRIEVALEVENT_H__

namespace SST {
    namespace Scheduler {
        
            class ObjectRetrievalEvent : public SST::Event{
                public:
                       ObjectRetrievalEvent() : SST::Event(){}
                           ObjectRetrievalEvent * copy(){
                               ObjectRetrievalEvent * newEvent = new ObjectRetrievalEvent();
                                   newEvent -> payload = payload;
                                   
                                   return newEvent;
                           }
                       
                           SST::Component * payload;
                           
                      NotSerializable(ObjectRetrievalEvent)
            };
        
    }
}
#endif

