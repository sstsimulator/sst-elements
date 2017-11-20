// Copyright 2009-2017 Sandia Corporation. Under the terms
// of Contract DE-NA0003525 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2017, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _H_SST_MEMH_ADDR_MAPPER
#define _H_SST_MEMH_ADDR_MAPPER

#include <sst/core/module.h>
#include "sst/elements/memHierarchy/util.h"

namespace SST {
namespace MemHierarchy {
namespace TimingDRAM_NS {

class AddrMapper : public SST::Module {
  public:
    AddrMapper( ) : m_numChannels(1), m_numRanks(1), m_numBanks(8)
    { }

    virtual void setNumChannels( unsigned int num  ) { 
        m_numChannels = num;
    }

    virtual void setNumRanks( unsigned int num ) {
        m_numRanks = num;
    } 
    
    virtual void setNumBanks( unsigned int num ) {
        m_numBanks = num;
    }

    virtual int getChannel( Addr ) = 0;
    virtual int getRank( Addr ) = 0;
    virtual int getBank( Addr ) = 0;
    virtual int getRow( Addr ) = 0;

  protected:
    unsigned m_numChannels;
    unsigned m_numRanks;
    unsigned m_numBanks;
};

class SimpleAddrMapper : public AddrMapper {
  public:
/* Element Library Info */
    SST_ELI_REGISTER_MODULE(SimpleAddrMapper, "memHierarchy", "simpleAddrMapper", SST_ELI_ELEMENT_VERSION(1,0,0),
            "Simple address mapper", "SST::MemHierarchy::AddrMapper")
    
/* Begin class definition */
    SimpleAddrMapper( Params &params ) : 
        AddrMapper(), 
        m_baseShift( 6 ),
        m_columnShift( 8 )
    { }

    int channelShift() { return m_baseShift; } 
    int channelMask()  { return m_numChannels - 1; }
    int channelWidth() { return log2Of( m_numChannels ); }

    int rankShift() { return m_columnShift + channelShift() + channelWidth(); }
    int rankMask() { return m_numRanks - 1; }
    int rankWidth() { return log2Of( m_numRanks ); }

    int bankShift() { return rankShift() + rankWidth(); }
    int bankMask() { return m_numBanks - 1; }
    int bankWidth() { return log2Of( m_numBanks ); }

    int rowShift() { return bankShift() + bankWidth(); }

    int getChannel( Addr addr ) { 
        return (addr >> channelShift() ) & channelMask();
    }

    int getRank( Addr addr ) {
        return ( addr >> rankShift() ) & rankMask();
    }

    int getBank( Addr addr ) {
        return ( addr >> bankShift() ) & bankMask();
    }

    int getRow( Addr addr ) {
        return addr >> rowShift();
    }
    
  private:
    unsigned m_baseShift;
    unsigned m_columnShift;
};

class SandyBridgeAddrMapper : public AddrMapper {
  public:
/* Element Library Info */
    SST_ELI_REGISTER_MODULE(SandyBridgeAddrMapper, "memHierarchy", "sandyBridgeAddrMapper", SST_ELI_ELEMENT_VERSION(1,0,0),
            "Sandy Bridge address mapper", "SST::MemHierarchy::AddrMapper")
    
/* Begin class definition */
    SandyBridgeAddrMapper( Params &params ) : AddrMapper() 
    { }

    int getChannel( Addr addr ) { return 0; }
    int getRank( Addr addr ) {

        if ( m_numRanks == 1 ) {
            return 0;
        } else {
            return (addr >> 17) & 1;
        }
    }
    int getBank( Addr addr ) {
        return ( (addr >> 14 ) & 7 ) ^ ( getRow(addr) & 7 );
    }
    int getRow( Addr addr ) {
        if ( m_numRanks == 1 ) {
            return (addr >> 17) & ( ( 1<<15 ) - 1 );
        } else {
            return (addr >> 18) & ( ( 1<<15 ) - 1 );
        }
    }

  private:
};

}
}
}

#endif
