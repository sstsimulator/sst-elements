// Copyright 2009-2013 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2013, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef SST_SCHEDULER_JOB_H__
#define SST_SCHEDULER_JOB_H__

#include "sst/core/serialization/element.h"

#include <string>
#include <sstream>

#include "Statistics.h"  //needed for friend declaration



namespace SST {
    namespace Scheduler {

        class AllocInfo;
        class Machine;

        class Job {
            public:
                Job(std::istream& input, bool accurateEsts);
                Job(unsigned long arrivalTime, int procsNeeded, unsigned long actualRunningTime,
                    unsigned long estRunningTime);
                Job(long arrivalTime, int procsNeeded, long actualRunningTime,
                    long estRunningTime, std::string ID );

                /*  ~Job(){
                    std::cout << "destructing job " << ID << " ";
                    if( hasRun ){
                    std::cout << "which has run" << std::endl;
                    }else{
                    std::cout << "which has not run" << std::endl;
                    }
                    print_stacktrace();
                    }*/

                ~Job(){}

                unsigned long getArrivalTime() { return arrivalTime; }
                unsigned long getStartTime();
                int getProcsNeeded() { return procsNeeded; }
                long getJobNum() { return jobNum; }

                std::string * getID() {
                    if (0 == ID.length()) {
                        std::ostringstream jobNumStr;
                        jobNumStr << jobNum;
                        ID = std::string( jobNumStr.str() );
                    }
                    return & ID;
                }

                //two versions depending on whether allocation is considered:
                unsigned long getEstimatedRunningTime() 
                { 
                    return estRunningTime; 
                }
                unsigned long getEstimatedRunningTime(AllocInfo* allocInfo);

                std::string toString();

                void start(unsigned long time, Machine* machine, AllocInfo* allocInfo,
                           Statistics* stats);

            private:
                unsigned long arrivalTime;        //when the job arrived
                int procsNeeded;         //how many processors it uses
                unsigned long actualRunningTime;  //how long it runs
                unsigned long estRunningTime;     //user estimated running time
                unsigned long startTime;	     //when the job started (-1 if not running)
                bool hasRun;

                long jobNum;             //ID number unique to this job

                std::string ID;       // alternate ID not used by SST

                unsigned long getActualTime() 
                { 
                    return actualRunningTime; 
                }

                //helper for constructors:
                void initialize(unsigned long arrivalTime, int procsNeeded,
                                unsigned long actualRunningTime, unsigned long estRunningTime);

                friend class Statistics;
                friend class schedComponent;
                //so statistics and schedComponent can use actual job times

                friend class boost::serialization::access;
                template<class Archive>
                    void serialize(Archive & ar, const unsigned int version )
                    {
                        ar & BOOST_SERIALIZATION_NVP(arrivalTime);
                        ar & BOOST_SERIALIZATION_NVP(procsNeeded);
                        ar & BOOST_SERIALIZATION_NVP(actualRunningTime);
                        ar & BOOST_SERIALIZATION_NVP(estRunningTime);
                        ar & BOOST_SERIALIZATION_NVP(startTime);
                        ar & BOOST_SERIALIZATION_NVP(jobNum);
                        ar & BOOST_SERIALIZATION_NVP(ID);
                    }

        };


    }
}
#endif
