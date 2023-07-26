// Copyright 2009-2023 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2023, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _H_VANADIS_LSQ_BASE
#define _H_VANADIS_LSQ_BASE

#include <sst/core/output.h>
#include <sst/core/subcomponent.h>

#include "inst/regfile.h"
#include "inst/vfence.h"
#include "inst/vload.h"
#include "inst/vstore.h"

#include <cassert>
#include <cinttypes>
#include <cstdint>
#include <vector>
#include <queue>

#define VANADIS_DBG_LSQ_STORE_FLG  (1<<0)
#define VANADIS_DBG_LSQ_LOAD_FLG  (1<<1)

namespace SST {
namespace Vanadis {

class VanadisLoadStoreQueue : public SST::SubComponent {

public:
    SST_ELI_REGISTER_SUBCOMPONENT_API(SST::Vanadis::VanadisLoadStoreQueue)

    SST_ELI_DOCUMENT_PARAMS({ "verbose", "Set the verbosity of output for the LSQ", "0" }, )

    SST_ELI_DOCUMENT_STATISTICS({ "bytes_read", "Count all the bytes read for data operations", "bytes", 1 },
                                { "bytes_stored", "Count all the bytes written for data operations", "bytes", 1 },
                                { "loads_issued", "Count the number of loads issued", "operations", 1 },
                                { "stores_issued", "Count the number of stores issued", "operations", 1 })

    VanadisLoadStoreQueue(ComponentId_t id, Params& params) : SubComponent(id) {

        uint32_t verbosity = params.find<uint32_t>("verbose");
        uint32_t mask = params.find<uint32_t>("verboseMask",-1);
        output = new SST::Output("[lsq @t]: ", verbosity, 0xFFFFFFFF, SST::Output::STDOUT);

        address_mask = params.find<uint64_t>("address_mask", 0xFFFFFFFFFFFFFFFF);
        setDbgInsAddrs( params.find<std::string>("dbgInsAddrs", "") );
        setDbgAddrs( params.find<std::string>("dbgAddrs", "") );

        registerFiles = nullptr;

        stat_load_issued = registerStatistic<uint64_t>("loads_issued", "1");
        stat_store_issued = registerStatistic<uint64_t>("stores_issued", "1");
        stat_data_bytes_read = registerStatistic<uint64_t>("bytes_read", "1");
        stat_data_bytes_written = registerStatistic<uint64_t>("bytes_stored", "1");
    }

    virtual ~VanadisLoadStoreQueue() { delete output; }

    void setRegisterFiles(std::vector<VanadisRegisterFile*>* reg_f) {
        output->verbose(CALL_INFO, 8, 0, "Setting register files (%" PRIu32 " register files in set)\n",
                        (uint32_t)reg_f->size());
        assert(reg_f != nullptr);

        registerFiles = reg_f;
    }
    void setCoreId( int core ) { core_id = core; }
    int getCoreId( ) { return core_id; }

    virtual bool storeFull() = 0;
    virtual bool loadFull() = 0;
    virtual bool storeBufferFull() = 0;

    virtual size_t storeSize() = 0;
    virtual size_t loadSize() = 0;
    virtual size_t storeBufferSize() = 0;

    virtual void push(VanadisStoreInstruction* store_me) = 0;
    virtual void push(VanadisLoadInstruction* load_me) = 0;
    virtual void push(VanadisFenceInstruction* fence) = 0;

    virtual void tick(uint64_t cycle) = 0;

    virtual void clearLSQByThreadID(const uint32_t thread) = 0;

    virtual void init(unsigned int phase) = 0;
    
    virtual void printStatus(SST::Output& output) {}

protected:

    void setDbgInsAddrs( std::string addrs ) {

        while ( ! addrs.empty() ) {
            printf("%s() %s\n",__func__,addrs.c_str());
            auto pos = addrs.find(',');
            std::string addr;
            if ( pos == std::string::npos ) {
                addr = addrs;
                addrs.clear();
            } else  {
                addr = addrs.substr(0,pos);
                addrs = addrs.substr(pos+1);
            }
            m_dbgInsAddrs.push_back(  strtol( addr.c_str(), NULL , 16 ) );
        }
    }


    bool isDbgInsAddr( uint64_t addr ) {
        for ( auto& it : m_dbgInsAddrs ) {
            if ( it == addr ) return true;
        }
        return false;
    }

    void setDbgAddrs( std::string addrs ) {

        while ( ! addrs.empty() ) {
            printf("%s() %s\n",__func__,addrs.c_str());
            auto pos = addrs.find(',');
            std::string addr;
            if ( pos == std::string::npos ) {
                addr = addrs;
                addrs.clear();
            } else  {
                addr = addrs.substr(0,pos);
                addrs = addrs.substr(pos+1);
            }
            m_dbgAddrs.push_back(  strtol( addr.c_str(), NULL , 16 ) );
        }
    }


    bool isDbgAddr( uint64_t addr ) {
        for ( auto& it : m_dbgAddrs ) {
            if ( it == addr ) return true;
        }
        return false;
    }

    std::deque<uint64_t> m_dbgInsAddrs;
    std::deque<uint64_t> m_dbgAddrs;
    int core_id;
    uint64_t address_mask;
    std::vector<VanadisRegisterFile*>* registerFiles;
    SST::Output* output;

    Statistic<uint64_t>* stat_load_issued;
    Statistic<uint64_t>* stat_store_issued;
    Statistic<uint64_t>* stat_data_bytes_read;
    Statistic<uint64_t>* stat_data_bytes_written;
};

} // namespace Vanadis
} // namespace SST

#endif
