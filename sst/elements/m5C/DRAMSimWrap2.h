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


#ifndef _DRAMSimWrap2_h
#define _DRAMSimWrap2_h

#include <DRAMSimWrap.h>

namespace SST {
namespace M5 {

struct DRAMSimWrap2Params : public DRAMSimWrapParams 
{
    Range< Addr > backdoorRange;
    int rank;
};

class DRAMSimWrap2 : public DRAMSimWrap
{
    class backdoorPort : public Port {
      public:
        backdoorPort( const std::string &name, DRAMSimWrap2* obj ) :
            Port( name, obj )
        {
        }
      protected:
        virtual bool recvTiming(PacketPtr pkt ) {
            PRINTFN("%s\n",__func__);
            return static_cast<DRAMSimWrap2*>(owner)->recvTimingBackdoor( pkt);
        }

        virtual Tick recvAtomic(PacketPtr pkt)
        { cerr << WHERE <<  "abort" << endl; exit(1); }

        virtual void recvFunctional(PacketPtr pkt)
        { cerr << WHERE <<  "abort" << endl; exit(1); }

        virtual void recvStatusChange(Status status) {}
    };

  public:
    typedef DRAMSimWrap2Params Params;
    DRAMSimWrap2( const Params*p );
    Port * getPort(const std::string &if_name, int idx );
    bool recvTiming(PacketPtr); 
    bool recvTimingBackdoor( PacketPtr );

  private:
    int             m_rank;
    Range< Addr >   m_sharedRange;
    Range< Addr >   m_myRange;
    Port*           m_backdoor;
};

}
}
#endif
