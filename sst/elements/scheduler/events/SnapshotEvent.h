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

#ifndef SST_SCHEDULER_SNAPSHOTEVENT_H__
#define SST_SCHEDULER_SNAPSHOTEVENT_H__

#include "sst/core/sst_types.h"

namespace SST {
    namespace Scheduler {

        class SnapshotEvent : public SST::Event {
            public:

                SnapshotEvent(SimTime_t time, int jobNum) : SST::Event() {
                    this -> time = time;
                    this -> jobNum = jobNum;
                }

                SimTime_t time;   //current time of the snapshot
                int jobNum;
                ITMI itmi;

            private:
                SnapshotEvent();

                friend class boost::serialization::access;
                template<class Archive>
                    void serialize(Archive & ar, const unsigned int version )
                    {
                        ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Event);
                        ar & BOOST_SERIALIZATION_NVP(jobNum);
                        ar & BOOST_SERIALIZATION_NVP(time);
                        ar & BOOST_SERIALIZATION_NVP(itmi);
                    }

         };

    }
}
#endif

