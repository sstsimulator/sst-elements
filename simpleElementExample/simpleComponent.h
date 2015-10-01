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

#ifndef _SIMPLECOMPONENT_H
#define _SIMPLECOMPONENT_H

#include <sst/core/component.h>
#include <sst/core/link.h>
#include <sst/core/rng/marsaglia.h>

namespace SST {
namespace SimpleComponent {

class simpleComponent : public SST::Component 
{
public:
    simpleComponent(SST::ComponentId_t id, SST::Params& params);
    ~simpleComponent();
    
    void setup() { }
    void finish() {
        static int n = 0;
        n++;
        if (n == 10) {
            printf("Several Simple Components Finished\n");
        } else if (n > 10) {
            ;
        } else {
            printf("Simple Component Finished\n");
        }
//        return 0;
    }

private:
    simpleComponent();  // for serialization only
    simpleComponent(const simpleComponent&); // do not implement
    void operator=(const simpleComponent&); // do not implement
    
    void handleEvent(SST::Event *ev);
    virtual bool clockTic(SST::Cycle_t);
    
    int workPerCycle;
    int commFreq;
    int commSize;
    int neighbor;
    
    SST::RNG::MarsagliaRNG* rng;
    SST::Link* N;
    SST::Link* S;
    SST::Link* E;
    SST::Link* W;
    
    friend class boost::serialization::access;
    template<class Archive>
    void save(Archive & ar, const unsigned int version) const
    {
        ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Component);
        ar & BOOST_SERIALIZATION_NVP(workPerCycle);
        ar & BOOST_SERIALIZATION_NVP(commFreq);
        ar & BOOST_SERIALIZATION_NVP(commSize);
        ar & BOOST_SERIALIZATION_NVP(neighbor);
        ar & BOOST_SERIALIZATION_NVP(N);
        ar & BOOST_SERIALIZATION_NVP(S);
        ar & BOOST_SERIALIZATION_NVP(E);
        ar & BOOST_SERIALIZATION_NVP(W);
    }
    
    template<class Archive>
    void load(Archive & ar, const unsigned int version) 
    {
        ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Component);
        ar & BOOST_SERIALIZATION_NVP(workPerCycle);
        ar & BOOST_SERIALIZATION_NVP(commFreq);
        ar & BOOST_SERIALIZATION_NVP(commSize);
        ar & BOOST_SERIALIZATION_NVP(neighbor);
        ar & BOOST_SERIALIZATION_NVP(N);
        ar & BOOST_SERIALIZATION_NVP(S);
        ar & BOOST_SERIALIZATION_NVP(E);
        ar & BOOST_SERIALIZATION_NVP(W);
        //restore links
        N->setFunctor(new SST::Event::Handler<simpleComponent>(this,&simpleComponent::handleEvent));
        S->setFunctor(new SST::Event::Handler<simpleComponent>(this,&simpleComponent::handleEvent));
        E->setFunctor(new SST::Event::Handler<simpleComponent>(this,&simpleComponent::handleEvent));
        W->setFunctor(new SST::Event::Handler<simpleComponent>(this,&simpleComponent::handleEvent));
    }
      
    BOOST_SERIALIZATION_SPLIT_MEMBER()
};

} // namespace SimpleComponent
} // namespace SST

#endif /* _SIMPLECOMPONENT_H */
