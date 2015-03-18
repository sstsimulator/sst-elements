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

#ifndef SST_SCHEDULER_SCHEDCOMPONENT_H__
#define SST_SCHEDULER_SCHEDCOMPONENT_H__

#include <fstream>
#include <string>
#include <vector>
#include <boost/filesystem.hpp>

#include <sst/core/component.h>
#include <sst/core/link.h>
#include <sst/core/rng/sstrand.h>

#include "output.h"

namespace SST {
    class Event;
    class Link;
    class Params;
    
    namespace Scheduler {

        class Job;
        class FaultEvent;
        class CompletionEvent;
        class ArrivalEvent;
        class Machine;
        class Scheduler;
        class Allocator;
        class TaskMapper;
        class Statistics;
        class TaskMapInfo;
        class FST;
        class JobParser;

        // the maximum length of a job ID.  used primarily for job list parsing.

        struct ITMI {int i; TaskMapInfo *tmi;};

        class schedComponent : public SST::Component {
            public:

                schedComponent(SST::ComponentId_t id, SST::Params& params);
                ~schedComponent(); 
                void setup();
                void finish();
                
                //ConstraintAllocator needs this
                std::string getNodeID(int nodeIndex) const
                {
                    return nodeIDs.at(nodeIndex);
                }

            private:
                unsigned long lastfinaltime;
                schedComponent();  // for serialization only
                schedComponent(const schedComponent&); // do not implement
                void operator=(const schedComponent&); // do not implement

                void registerThis();
                void unregisterThis();
                bool isRegistered();

                bool registrationStatus;

                std::string trace;
                std::string jobListFilename;

                void handleCompletionEvent(Event *ev, int n);
                void handleJobArrivalEvent(Event *ev);

                void unregisterYourself();

                void startNextJob();
                void startJob(Job* job);

                void logJobStart(ITMI itmi);
                void logJobFinish(ITMI itmi);
                void logJobFault(ITMI itmi, FaultEvent * faultEvent );

                typedef std::vector<int> targetList_t;

                std::vector<Job*> jobs;
                std::list<CompletionEvent*> finishingcomp;
                std::list<ArrivalEvent*> finishingarr;
                Machine* machine;
                Scheduler* scheduler;
                Scheduler* FSTscheduler;
                Allocator* theAllocator;
                TaskMapper* theTaskMapper;
                Statistics* stats;
                int FSTtype;
                FST* calcFST;
                std::vector<SST::Link*> nodes;
                std::vector<std::string> nodeIDs;
                SST::Link* selfLink;
                std::map<int, ITMI> runningJobs;
                std::vector<double>* timePerDistance; //used if we want to add time to the jobs proportional to the L1 distance
                
                std::string jobLogFileName;
                std::ofstream jobLog;
                bool printJobLog;
                bool printYumYumJobLog;       // should the Job Log use the YumYum format?
                bool useYumYumTraceFormat;    // should we expect the incoming job list to use the YumYum format?
                                              // useYumYumTraceFormat is regularly used to decide if YumYum functionality should be used or not.
                JobParser* jobParser;

                bool useYumYumSimulationKill;         // should the simulation end on a special job (true), or just when the job list is exhausted (false)?
                bool YumYumSimulationKillFlag;        // this will signal the schedComponent to unregister itself iff useYumYumSimulationKill == true
                int YumYumPollWait;                   // this is the length of time in ms to wait between checks for new jobs
                
                SST::RNG::SSTRandom* rng;  //random number generator

                friend class boost::serialization::access;
                template<class Archive>
                    void save(Archive & ar, const unsigned int version) const
                    {
                        ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Component);
                        ar & BOOST_SERIALIZATION_NVP(nodes);
                        ar & BOOST_SERIALIZATION_NVP(selfLink);
                    }

                template<class Archive>
                    void load(Archive & ar, const unsigned int version) 
                    {
                        ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Component);
                        ar & BOOST_SERIALIZATION_NVP(nodes);
                        ar & BOOST_SERIALIZATION_NVP(selfLink);
                        //restore links
                        for (unsigned int i = 0; i < nodes.size(); ++i) {
                            nodes[i] -> setFunctor(new SST::Event::Handler<schedComponent,int>(this,
                                                                                             &schedComponent::handleCompletionEvent,i));
                        }

                        selfLink -> setFunctor(new SST::Event::Handler<schedComponent>(this,
                                                                                     &schedComponent::handleJobArrivalEvent));
                    }

                BOOST_SERIALIZATION_SPLIT_MEMBER()

        };

    }
}
#endif /* _SCHEDCOMPONENT_H */
