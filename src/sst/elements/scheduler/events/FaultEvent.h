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

#ifndef __JOBFAULTEVENT_H__
#define __JOBFAULTEVENT_H__

#include <string>

namespace SST {
    namespace Scheduler {

#define FAULT_EVENT_SHOULDKILL 2 /* Scheduled nodes should kill the job */
#define FAULT_EVENT_DONTKILL 1 /* Scheduled nodes should _NOT_ kill the job */
#define FAULT_EVENT_UNDECIDED 0 /* Scheduled nodes should determine if the job should be killed.  Default. */

        /* This is a fault that can travel through the graph.
         *
         * Any and All attributes of this class should be added to FaultEvent.copy()
         * unless there is a Really Good Reason not to do so.
         */
        class FaultEvent : public SST::Event{
            public:
                FaultEvent(std::string faultType) : SST::Event()
                {
                    this -> faultType = faultType;

                    shouldKillJob = FAULT_EVENT_UNDECIDED;
                    jobNum = -1;
                    nodeNumber = -1;
                    /* jobNumber and nodeNumber are assigned by the leaf node */
                }


                /* Creates and returns an identical copy of this object. */
                FaultEvent * copy()
                {
                    FaultEvent* newFault = new FaultEvent(faultType);
                    newFault -> shouldKillJob = shouldKillJob;
                    newFault -> jobNum = jobNum;
                    newFault -> nodeNumber = nodeNumber;

                    return newFault;
                }

                int shouldKillJob;  // dictates if this event should cause jobs to die
                int jobNum;         // the job number of the scheduled node that recieves the fault
                int nodeNumber;     // the node number of the scheduled node that recieves the fault
                std::string faultType;

            private:
                FaultEvent();

                NotSerializable(FaultEvent)
        };

    }
}
#endif

