// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2018, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _VAULTSIMC_H
#define _VAULTSIMC_H

#include <sst/core/event.h>
#include <sst/core/component.h>
#include <sst/core/elementinfo.h>
#include <sst/elements/memHierarchy/memEvent.h>
#include <sst/core/output.h>

/* If we have a PHX library to link to, we use that. Otherwise, we use
   a greatly simplifed timing model */
#if HAVE_LIBPHX == 1
#include <Vault.h>
#include <BusPacket.h>
#endif  /* HAVE_LIBPHX */

#include "vaultGlobals.h"

using namespace std;

namespace SST {
namespace VaultSim {

#if HAVE_LIBPHX == 1
using namespace PHXSim; 
#endif  /* HAVE_LIBPHX */

//#define STUPID_DEBUG 

class VaultSimC : public Component {

public: // functions

    SST_ELI_REGISTER_COMPONENT(
                               VaultSimC,
                               "VaultSimC",
                               "VaultSimC",
                               SST_ELI_ELEMENT_VERSION(1,0,0),
                               "A memory vault in a stacked memory",
                               COMPONENT_CATEGORY_MEMORY)

    SST_ELI_DOCUMENT_PARAMS(
                            {"clock",  "Vault Clock Rate.", "1.0 Ghz"},
                            {"numVaults2",         "Number of bits to determine vault address (i.e. log_2(number of vaults per cube))"},
                            {"VaultID",            "Vault Unique ID (Unique to cube)."},
                            {"debug",              "0 (default): No debugging, 1: STDOUT, 2: STDERR, 3: FILE."},
#if !(HAVE_LIBPHX == 1)
                            {"delay", "Static vault delay", "40ns"}
#endif /* HAVE_LIBPHX */
                            )

    SST_ELI_DOCUMENT_PORTS(
                           {"bus", "Link to the logic layer", {"MemEvent",""}},
                            )
                           
    SST_ELI_DOCUMENT_STATISTICS(
                                { "Mem_Outstanding", "Number of memory requests outstanding each cycle", "reqs/cycle", 1}
                            )

    VaultSimC( ComponentId_t id, Params& params );
    int Finish();
    void init(unsigned int phase);
    
private: // types
    
    typedef SST::Link memChan_t;
    typedef map<unsigned, MemHierarchy::MemEvent*> t2MEMap_t;
    
private: // functions
    
    VaultSimC( const VaultSimC& c );
    
#if HAVE_LIBPHX == 1
    bool clock_phx( Cycle_t );
    
    inline PHXSim::TransactionType convertType( SST::MemHierarchy::Command type );
    
    void readData(BusPacket bp, unsigned clockcycle);
    void writeData(BusPacket bp, unsigned clockcycle);
    
    std::deque<Transaction> m_transQ;
    t2MEMap_t transactionToMemEventMap; // maps original MemEvent to a Vault transaction ID
    Vault* m_memorySystem;
#else
    // if not have PHX Lib...

    bool clock( Cycle_t );
    Link *delayLine;
    
#endif /* HAVE_LIBPHX */
    

    uint8_t *memBuffer;
    memChan_t* m_memChan;
    size_t numVaults2;  // not clear if used
    Output dbg;
    int numOutstanding; //number of mem requests outstanding (non-phx)

    unsigned vaultID;
    size_t getInternalAddress(MemHierarchy::Addr in) {
        // calculate address
        size_t lower = in & VAULT_MASK;
        size_t upper = in >> (numVaults2 + VAULT_SHIFT);
        size_t out = (upper << VAULT_SHIFT) + lower;
        return out;
    }

    // statistics
    Statistic<uint64_t>*  memOutStat;
};

#if HAVE_LIBPHX == 1
inline PHXSim::TransactionType VaultSimC::convertType( SST::MemHierarchy::Command type )
{
    /*  Needs to be updated with current MemHierarchy Commands/States
    switch( type )
      {
      case SST::MemHierarchy::ReadReq:
      case SST::MemHierarchy::RequestData:
	return PHXSim::DATA_READ;
      case SST::MemHierarchy::SupplyData:
      case SST::MemHierarchy::WriteReq:
	return PHXSim::DATA_WRITE;
      default: 
	_abort(VaultSimC,"Tried to convert unknown memEvent request type (%d) to PHXSim transaction type \n", type);
    }
    return (PHXSim::TransactionType)-1;
    */
}
#endif /* HAVE_LIBPHX */
}
}


#endif
