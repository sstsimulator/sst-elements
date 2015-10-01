// Copyright 2009-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2015, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

/*
 * Classes representing system events
 */

#ifndef SST_SCHEDULER_JOBSTARTEVENT_H__
#define SST_SCHEDULER_JOBSTARTEVENT_H__

namespace SST {
    namespace Scheduler {

        class JobStartEvent : public SST::Event {
            public:

                JobStartEvent(unsigned long time, int jobNum) : SST::Event() {
                    this -> time = time;
                    this -> jobNum = jobNum;
                }

                unsigned long time;   //the length of the started job

                int jobNum;
                bool emberFinished; //NetworkSim: variable that specifies if that job has run and finished on ember

            private:
                JobStartEvent() { }  // for serialization only

                friend class boost::serialization::access;
                template<class Archive>
                    void serialize(Archive & ar, const unsigned int version )
                    {
                        ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Event);
                        ar & BOOST_SERIALIZATION_NVP(time);
                        ar & BOOST_SERIALIZATION_NVP(jobNum);
                    }
        };

    }
}
#endif

