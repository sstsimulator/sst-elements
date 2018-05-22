// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2018, NTESS
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

    virtual void setNumChannels( unsigned int num ) {
        if (!isPowerOfTwo(num)) {
            Output output("", 1, 0, Output::STDOUT);
            output.fatal(CALL_INFO, -1, "SimpleAddrMapper, Error: memHierarchy.simpleAddrMapper does not support non-power-of-2 channels...use memHierarchy.roundRobinAddrMapper instead\n");
        }
        m_numChannels = num; 
    }

    virtual void setNumRanks( unsigned int num ) {
        if (!isPowerOfTwo(num)) {
            Output output("", 1, 0, Output::STDOUT);
            output.fatal(CALL_INFO, -1, "SimpleAddrMapper, Error: memHierarchy.simpleAddrMapper does not support non-power-of-2 ranks...use memHierarchy.roundRobinAddrMapper instead\n");
        }
        m_numRanks = num;
    }

    virtual void setNumBanks( unsigned int num ) {
        if (!isPowerOfTwo(num)) {
            Output output("", 1, 0, Output::STDOUT);
            output.fatal(CALL_INFO, -1, "SimpleAddrMapper, Error: memHierarchy.simpleAddrMapper does not support non-power-of-2 banks...use memHierarchy.roundRobinAddrMapper instead\n");
        }
        m_numBanks = num;
    }

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

class RoundRobinAddrMapper: public AddrMapper {
public:
    /* Element Library Info */
    SST_ELI_REGISTER_MODULE(RoundRobinAddrMapper, "memHierarchy", "roundRobinAddrMapper", SST_ELI_ELEMENT_VERSION(1,0,0),
            "Round-robin address mapper", "SST::MemHierarchy::AddrMapper")

    SST_ELI_DOCUMENT_PARAMS(
            {"interleave_size", "(string) Granularity of interleaving in bytes (B). SI ok.", "64B"},
            {"row_size",        "(string) Size of each row in bytes (B). SI ok.", "1KiB"})

    /* Begin class definition */
    /* Interleave blocks across channel, then bank, then rank, pack into rows */
    /* Generic for any #ranks, channels, banks. interleave_size must be power of 2, same as row/interleave_size */
    RoundRobinAddrMapper(Params &params) : AddrMapper() {
        UnitAlgebra ilSize = UnitAlgebra(params.find<std::string>("interleave_size", "64B")); /* Default = 64B line interleaving */
        UnitAlgebra rowSize = UnitAlgebra(params.find<std::string>("row_size", "1KiB"));
        Output output("", 1, 0, Output::STDOUT);
        
        if (!ilSize.hasUnits("B")) {
            output.fatal(CALL_INFO, -1, "RoundRobinAddrMapper, Invalid param - interleave_size: must have units of 'B'. SI ok.\n");
        }
        if (!rowSize.hasUnits("B")) {
            output.fatal(CALL_INFO, -1, "RoundRobinAddrMapper, Invalid param - row_size: must have units of 'B'. SI ok.\n");
        }
        if (!isPowerOfTwo(ilSize.getRoundedValue())) {
            output.fatal(CALL_INFO, -1, "RoundRobinAddrMapper, Invalid param - interleave_size: must be a power-of-2.\n");
        }
        if (!isPowerOfTwo(rowSize.getRoundedValue())) {
            output.fatal(CALL_INFO, -1, "RoundRobinAddrMapper, Invalid param - row_size: must be a power-of-2.\n");
        }
        m_baseShift = log2Of( ilSize.getRoundedValue() );
        m_rowDivider = m_numChannels * m_numRanks * m_numBanks * (rowSize.getRoundedValue() / ilSize.getRoundedValue());
        m_rankDivider = m_numChannels * m_numBanks;
    }
    
    virtual void setNumChannels( unsigned int num ) {
        m_rowDivider = m_rowDivider * num / m_numChannels; /* Swap num for m_numChannels */
        m_numChannels = num; 
        m_rankDivider = m_numChannels * m_numBanks;
    }

    virtual void setNumRanks( unsigned int num ) {
        m_rowDivider = m_rowDivider * num / m_numRanks;
        m_numRanks = num;
    }

    virtual void setNumBanks( unsigned int num ) {
        m_rowDivider = m_rowDivider * num / m_numBanks;
        m_numBanks = num;
        m_rankDivider = m_numChannels * m_numBanks;
    }

    int getChannel( Addr addr ) {
        return (addr >> m_baseShift) % m_numChannels;
    }
    
    int getRank( Addr addr ) {
        return ((addr >> m_baseShift) / m_rankDivider) % m_numRanks;
    }
    
    int getBank( Addr addr ) {
        return ((addr >> m_baseShift) / m_numChannels) % m_numBanks;
    }
    
    int getRow( Addr addr ) {
        return (addr >> m_baseShift) / m_rowDivider;
    }

private:
    unsigned int m_baseShift;
    unsigned int m_rankDivider;
    unsigned int m_rowDivider;
    unsigned int m_rowSize;
};


class SandyBridgeAddrMapper : public AddrMapper {
  public:
/* Element Library Info */
    SST_ELI_REGISTER_MODULE(SandyBridgeAddrMapper, "memHierarchy", "sandyBridgeAddrMapper", SST_ELI_ELEMENT_VERSION(1,0,0),
            "Sandy Bridge address mapper", "SST::MemHierarchy::AddrMapper")
    
/* Begin class definition */
    SandyBridgeAddrMapper( Params &params ) : AddrMapper() 
    { }
    
    virtual void setNumChannels( unsigned int num  ) { 
        if (num != 1) {
            Output output("", 1, 0, Output::STDOUT);
            output.fatal(CALL_INFO, -1, "SandyBridgeAddrMapper, Error: memHierarchy.sandyBridgeAddrMapper does not support multiple channels\n");
        }
        m_numChannels = num;
    }


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
