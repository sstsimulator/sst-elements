// Copyright 2009-2010 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2010,2013, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _VAULTSIMC_H
#define _VAULTSIMC_H

#include <sst/core/event.h>
#include <sst/core/introspectedComponent.h>
#include <sst/core/interfaces/memEvent.h>
#include <Vault.h>
#include <BusPacket.h>
#include <sst/core/output.h>
#include "vaultGlobals.h"


using namespace std;
using namespace SST;
using namespace PHXSim; 

//#define STUPID_DEBUG 

class VaultSimC : public IntrospectedComponent {

    public: // functions

        VaultSimC( ComponentId_t id, Params_t& params );
        int Finish();
	void init(unsigned int phase);

    private: // types

        typedef SST::Link memChan_t;
	typedef map<unsigned, Interfaces::MemEvent*> t2MEMap_t;

    private: // functions

        VaultSimC( const VaultSimC& c );
        bool clock( Cycle_t );

        inline PHXSim::TransactionType convertType( SST::Interfaces::Command type );

        void readData(BusPacket bp, unsigned clockcycle);
        void writeData(BusPacket bp, unsigned clockcycle);

        std::deque<Transaction> m_transQ;
	t2MEMap_t transactionToMemEventMap; // maps original MemEvent to a Vault transaction ID
        uint8_t *memBuffer;
	Vault* m_memorySystem;
        memChan_t* m_memChan;
	size_t numVaults2;
        Output dbg;

	unsigned vaultID;
	size_t getInternalAddress(Interfaces::Addr in) {
	  // calculate address
	  size_t lower = in & VAULT_MASK;
	  size_t upper = in >> (numVaults2 + VAULT_SHIFT);
	  size_t out = (upper << VAULT_SHIFT) + lower;
	  return out;
	}
};

inline PHXSim::TransactionType VaultSimC::convertType( SST::Interfaces::Command type )
{
    switch( type ) 
      {
      case SST::Interfaces::ReadReq:
      case SST::Interfaces::RequestData:
	return PHXSim::DATA_READ;
      case SST::Interfaces::SupplyData:
      case SST::Interfaces::WriteReq:
	return PHXSim::DATA_WRITE;
      default: 
	_abort(VaultSimC,"Tried to convert unknown memEvent request type (%d) to PHXSim transaction type \n", type);
    }
    return (PHXSim::TransactionType)-1;
}

#endif
