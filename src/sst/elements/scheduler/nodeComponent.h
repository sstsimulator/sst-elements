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

#ifndef SST_SCHEDULER_NODECOMPONENT_H
#define SST_SCHEDULER_NODECOMPONENT_H

#include <sst/core/element.h>
#include <sst/core/event.h>
#include <sst/core/sst_types.h>
#include <sst/core/component.h>
#include <sst/core/link.h>
#include <sst/core/timeConverter.h>
#include "events/FaultEvent.h"
#include "events/JobKillEvent.h"

#include "linkBuilder.h"

#include <vector>
#include <fstream>

namespace SST {
    namespace Scheduler {

        class nodeComponent : public SST::Component, public virtual linkChanger {
            friend class linkBuilder;

            public:

            nodeComponent(SST::ComponentId_t id, SST::Params& params);
            void setup();
            void finish() {}

            virtual void addLink(SST::Link * link, enum linkTypes type);
            virtual void rmLink(SST::Link * link, enum linkTypes type);
            virtual void disconnectYourself();
            virtual std::string getType();
            virtual std::string getID();

            bool doDetailedNetworkSim; //NetworkSim: variable that protects the original functionality without detailed network sim 

            private:
		unsigned short int * yumyumFaultRand48State;
		unsigned short int * yumyumErrorLogRand48State;
		unsigned short int * yumyumErrorLatencyRand48State;
		unsigned short int * yumyumErrorCorrectionRand48State;
		unsigned short int * yumyumJobKillRand48State;

		bool faultsActivated;

            nodeComponent();  // for serialization only
            nodeComponent(const nodeComponent&); // do not implement
            void operator=(const nodeComponent&); // do not implement

            void handleEvent(SST::Event* ev);
            void handleSelfEvent(SST::Event* ev);
            void handleFaultEvent(SST::Event* ev);
            bool canCorrectError(FaultEvent* error);
            unsigned int genFaultLatency( std::string faultName );

            void handleJobKillEvent(JobKillEvent * killEvent);

            void sendNextFault(std::string faultType);
            void logError(FaultEvent* faultEvent);
            void logFault(FaultEvent* faultEvent);

            int jobNum;
            int nodeNum;

            SST::Link * Scheduler;
            SST::Link * failureInjector;
            SST::Link * SelfLink;
            SST::Link * FaultLink;

            std::vector<SST::Link*> ParentFaultLinks;
            std::vector<SST::Link*> ChildFaultLinks;
            SST::Link* Builder;

            std::map<std::string, float> Faults;
            std::map<std::string, std::pair<unsigned int, unsigned int> > FaultLatencyBounds;
            std::map<std::string, float> errorCorrectionProbability;
            std::map<std::string, float> errorLogProbability;
            std::map<std::string, float> jobKillProbability;
            std::map<int, int> killedJobs;        // jobs that had to be killed, but haven't finished yet.  used to keep track of this node's state
            std::string faultLogFileName;
            std::string errorLogFileName;

            std::string ID;
            std::string nodeType;

//            friend class boost::serialization::access;
//            template<class Archive>
//                void save(Archive & ar, const unsigned int version) const
//                {
//                    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Component);
//                    ar & BOOST_SERIALIZATION_NVP(jobNum);
//                    ar & BOOST_SERIALIZATION_NVP(nodeNum);
//                    ar & BOOST_SERIALIZATION_NVP(Scheduler);
//                    ar & BOOST_SERIALIZATION_NVP(SelfLink);
//
//                    ar & BOOST_SERIALIZATION_NVP(ParentFaultLinks);
//                    ar & BOOST_SERIALIZATION_NVP(ChildFaultLinks);
//
//                    ar & BOOST_SERIALIZATION_NVP(Faults);
//                    ar & BOOST_SERIALIZATION_NVP(errorLogProbability);
//                    ar & BOOST_SERIALIZATION_NVP(faultLogFileName);
//                    ar & BOOST_SERIALIZATION_NVP(errorLogFileName);
//                }
//
//            template<class Archive>
//                void load(Archive & ar, const unsigned int version) 
//                {
//                    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Component);
//                    ar & BOOST_SERIALIZATION_NVP(jobNum);
//                    ar & BOOST_SERIALIZATION_NVP(nodeNum);
//                    ar & BOOST_SERIALIZATION_NVP(Scheduler);
//                    ar & BOOST_SERIALIZATION_NVP(SelfLink);
//
//                    ar & BOOST_SERIALIZATION_NVP(ParentFaultLinks);
//                    ar & BOOST_SERIALIZATION_NVP(ChildFaultLinks);
//
//                    ar & BOOST_SERIALIZATION_NVP(Faults);
//                    ar & BOOST_SERIALIZATION_NVP(errorLogProbability);
//                    ar & BOOST_SERIALIZATION_NVP(faultLogFileName);
//                    ar & BOOST_SERIALIZATION_NVP(errorLogFileName);
//
//                    //restore links
//                    Scheduler -> setFunctor(new SST::Event::Handler<nodeComponent>(this,
//                                                                                   &nodeComponent::handleEvent));
//                    SelfLink -> setFunctor(new SST::Event::Handler<nodeComponent>(this,
//                                                                                  &nodeComponent::handleSelfEvent));
//
//                    for (unsigned int counter = 0; counter < ParentFaultLinks.size(); counter++) {
//                        ParentFaultLinks.at( counter ) -> setFunctor(new SST::Event::Handler<nodeComponent>(this, 
//                                                                                                            &nodeComponent::handleEvent));
//                    }
//
//
//                    for (unsigned int counter = 0; counter < ChildFaultLinks.size(); counter ++) {
//                        ChildFaultLinks.at(counter) -> setFunctor(new SST::Event::Handler<nodeComponent>(this, 
//                                                                                                         &nodeComponent::handleEvent));
//                    }
//                }
//            BOOST_SERIALIZATION_SPLIT_MEMBER()
        };

    }
}
#endif /* _NODECOMPONENT_H */
