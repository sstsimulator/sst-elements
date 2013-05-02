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


#ifndef _DRAMSIMC_H
#define _DRAMSIMC_H

#include <sst/core/log.h>
#include <sst/core/event.h>
#include <sst/core/introspectedComponent.h>
#include <memoryChannel.h>
#include <MultiChannelMemorySystem.h>


using namespace std;
using namespace SST;

#ifndef DRAMSIMC_DBG
#define DRAMSIMC_DBG 0
#endif

class DRAMSimC : public IntrospectedComponent {

    public: // functions

        DRAMSimC( ComponentId_t id, Params_t& params );
        void finish(); 

    private: // types

        typedef MemoryChannel<uint64_t> memChan_t;

    private: // functions

        DRAMSimC( const DRAMSimC& c );
        bool clock( Cycle_t  );

        inline DRAMSim::TransactionType 
                        convertType( memChan_t::event_t::reqType_t type );

        void readData(uint id, uint64_t addr, uint64_t clockcycle);
        void writeData(uint id, uint64_t addr, uint64_t clockcycle);
	///uint64_t getIntData(int dataID, int index);

        std::deque<Transaction> m_transQ;
        MultiChannelMemorySystem*           m_memorySystem;
        memChan_t*              m_memChan;
        std::string             m_printStats;
        Log< DRAMSIMC_DBG >&    m_dbg;
        Log<>&                  m_log;

};

inline DRAMSim::TransactionType 
            DRAMSimC::convertType( memChan_t::event_t::reqType_t type )
{
    switch( type ) {
    case memChan_t::event_t::READ:
      return DRAMSim::DATA_READ;
    case memChan_t::event_t::WRITE:
      return DRAMSim::DATA_WRITE;
    default: ;
    }
    return (DRAMSim::TransactionType)-1;
}

#endif
