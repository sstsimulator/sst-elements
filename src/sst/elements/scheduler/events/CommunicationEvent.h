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
 * Used for communication between components, including passing instructions, requesting information, and communicating state changes.
 */

#ifndef SST_SCHEDULER_COMMUNICATIONEVENT_H__
#define SST_SCHEDULER_COMMUNICATIONEVENT_H__

namespace SST {
    namespace Scheduler {

        enum CommunicationTypes { RETRIEVE_ID, START_FAULTING, LOG_JOB_START, UNREGISTER_YOURSELF, START_FILE_WATCH, START_NEXT_JOB, SEED_FAULT, SEED_ERROR_LOG, SEED_ERROR_LATENCY, SEED_ERROR_CORRECTION, SEED_JOB_KILL, FAIL_JOBS, SET_DETAILED_NETWORK_SIM }; //NetworkSim: added SET_DETAILED_NETWORK_SIM type

        class CommunicationEvent : public SST::Event{
            public:

                CommunicationEvent( enum CommunicationTypes type ) : SST::Event() {
                    CommType = type;
                    reply = false;
                    this->payload = NULL;
                }

                CommunicationEvent(enum CommunicationTypes type, void* payload) : SST::Event() {
                    CommType = type;
                    reply = false;
                    this -> payload = payload;
                }

                enum CommunicationTypes CommType;
                bool reply;
                void * payload;
                
                NotSerializable(CommunicationEvent)
        };

    }
}
#endif

