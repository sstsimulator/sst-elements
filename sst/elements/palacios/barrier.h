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

#ifndef _barrier_h
#define _barrier_h

#include <sst/core/action.h>
#include <sst/core/simulation.h>
#include <sst/core/timeLord.h>


#if 0
#define BA_DBG(fmt,args...) \
  fprintf(stderr,"%d:BarrierAction::%s() "fmt, world.rank(), __func__, ##args)
#else
#define BA_DBG(fmt,args...)
#endif

namespace SST {
namespace Palacios {


class BarrierAction : public SST::Action 
{
     boost::mpi::communicator world; 

  public:

    class HandlerBase {
      public:
        virtual void operator()() = 0;
        virtual ~HandlerBase() {}
    };

    template <typename classT>
    class Handler : public HandlerBase {
      private:
        typedef void (classT::*PtrMember)();
        const PtrMember member;
        classT* object;

      public:
        Handler( classT* const object, PtrMember member ) :
          member(member),
          object(object)
        {}

        void operator()() {
            (object->*member)();
        }
    };


    BarrierAction() :
        m_numReporting(0)
    {
        BA_DBG("\n");
        setPriority(75);
        m_nRanks = SST::Simulation::getSimulation()->getNumRanks();

        SST::Simulation::getSimulation()->insertActivity(
            SST::Simulation::getSimulation()->getTimeLord()->
                getTimeConverter("1us")->getFactor(),this);
    }

    void execute() {
        SST::Simulation *sim = SST::Simulation::getSimulation();

        int value = m_localQ.size() - m_numReporting ? 0 : 1 ;
        int out;
        
        all_reduce( world, &value, 1, &out, std::plus<int>() );
        if ( out == m_nRanks ) {
            BA_DBG("everyone is here\n");  
            std::deque<HandlerBase*>::iterator it = m_localQ.begin();

            while( it != m_localQ.end() ) {
                (**it)();
                ++it;
            }

            m_numReporting = 0;
        }  

        SST::SimTime_t next = sim->getCurrentSimCycle() +
            sim->getTimeLord()->getTimeConverter("1us")->getFactor();
        sim->insertActivity( next, this );
    }

    void add(HandlerBase* handler) {
        m_localQ.push_back( handler );
        BA_DBG("local=%d\n",m_localQ.size());
    }

    void enter() {
        BA_DBG("numReporting=%d\n",m_numReporting);
        ++m_numReporting;
    }

  private:
    int m_nRanks;
    int m_numReporting;
    std::deque< HandlerBase* > m_localQ;
};
}}
#endif
