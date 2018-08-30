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
 * Class that tells the scheduler there are no more events at this time
 * (otherwise the scheduler handles each event individually when they come,
 * even if they are at the same time) 
 * Has high priority value so that SST puts it at the end of its event queue
 */

#ifndef SST_SCHEDULER_FINALTIMEEVENT_H__
#define SST_SCHEDULER_FINALTIMEEVENT_H__

namespace SST {
    namespace Scheduler {

        class FinalTimeEvent : public SST::Event {
            public:
                bool forceExecute; //only one final time event is looked at at a given time.
                //however, if there is (say) a job with length 0, it will not be able to
                //complete on time as its finaltimeevent will be ignored.  This variable
                //is therefore set to force schedComponent to handle this event.

                FinalTimeEvent() : SST::Event() {
                    setPriority(FINALEVENTPRIORITY); // one less than the event that says everything is done
                    //(we want this to be after everything except that)
                    forceExecute = false;
                }

                NotSerializable(FinalTimeEvent)
        };

    }
}
#endif

