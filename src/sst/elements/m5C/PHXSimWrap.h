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

#ifndef _PHXSimWrap_h
#define _PHXSimWrap_h

#include <sst/core/component.h>

#include <base/statistics.hh>
#include <mem/physical.hh>
#include <debug.h>
#include <deque>

#include <sst/core/timeConverter.h>

#ifndef PHXSIMC_DBG
#define PHXSIMC_DBG 1 
#endif

#include <SingleCube.h>

using namespace std;

namespace SST {
namespace M5 {

class M5;

struct PHXSimWrapParams : public PhysicalMemoryParams
{
    SST::Params exeParams;
    std::string frequency;
    std::string megsOfMemory;
    bool dumpStats;
};

class PHXSimWrap : public PhysicalMemory 
{
  protected:
    class MemPort : public Port {
      public:
        MemPort( const std::string &name, PHXSimWrap* obj ) :
            Port( name, obj )
        {
        } 
      protected:
        virtual bool recvTiming(PacketPtr pkt) {

            if (pkt->memInhibitAsserted()) {
                // snooper will supply based on copy of packet
                // still target's responsibility to delete packet
                PRINTFN("recvTiming: %s %#lx memInhibitAssert()\n",
                        pkt->cmdString().c_str(), (long)pkt->getAddr());
                delete pkt;
                return true;
            }

            return static_cast<PHXSimWrap*>(owner)->recvTiming( pkt );
        }

        virtual Tick recvAtomic(PacketPtr pkt) 
        { cerr << WHERE <<  "abort" << endl; exit(1); }

        virtual void recvFunctional(PacketPtr pkt) {
            static_cast<PHXSimWrap*>(owner)->doFunctionalAccess( pkt );
        }

        virtual void recvStatusChange(Status status) { 
            static_cast<PHXSimWrap*>(owner)->recvStatusChange( status );
        }

        virtual void recvRetry( ) { 
            static_cast<PHXSimWrap*>(owner)->recvRetry( ); 
        }

        virtual void getDeviceAddressRanges(AddrRangeList &resp, bool &snoop) {
            static_cast<PHXSimWrap*>(owner)->getAddressRanges( resp, snoop ); 
        }
    };

    class TickEvent : public ::Event
    {
        PHXSimWrap* m_obj;
        unsigned long m_delay;
      public:
        TickEvent( PHXSimWrap* obj, int delay ) : m_obj(obj), m_delay( delay ) 
        {
            m_obj->schedule( this, curTick() );  
        }
        void process() {
            m_obj->tick();
            m_obj->schedule( this, curTick() + m_delay );  
        }
        
        const char *description() const {
            return "PHXSimWrap tick";
        }
        
        NotSerializable(TickEvent)
    };

    void tick();

    TickEvent*   m_tickEvent;

    bool        m_blocked;

  public:
    typedef PHXSimWrapParams Params;
    PHXSimWrap( const Params* p );
    virtual ~PHXSimWrap( );

    virtual Port * getPort(const std::string &if_name, int idx = -1);

    void init();
    void regStats();

   protected:
    virtual bool recvTiming(PacketPtr pkt);
    MemPort*        m_port;

  private:
    const PHXSimWrap &operator=( const Params& p );

    void recvRetry();
    void sendTry();
    void finish();
	
    void recv(PHXSim::Transaction *t, uint64_t cycle);
    bool recvSignalWrapper(void *arg);

    bool clock( SST::Cycle_t );
    void readData(unsigned int id, uint64_t addr, uint64_t clockcycle);
    void writeData(unsigned int id, uint64_t addr, uint64_t clockcycle);

    std::map< uint64_t, PacketPtr >       m_rd_pktMap;
    std::multimap< uint64_t, PacketPtr >       m_wr_pktMap;
    std::deque< PacketPtr >       m_readyQ;

    Stats::Distribution m_rd_lat;
    Stats::Distribution m_wr_lat;

    PHXSim::SingleCube*           m_memorySystem;
    bool            m_waitRetry;

    SST::TimeConverter*     m_tc;

    Log< PHXSIMC_DBG >&    m_dbg;
    Log<>&                  m_log;
};

}
}

#endif
