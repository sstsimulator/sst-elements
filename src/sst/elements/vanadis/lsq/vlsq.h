// Copyright 2009-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2025, NTESS
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

#define VANADIS_DBG_LSQ_STORE_FLG 0 // (1<<0)
#define VANADIS_DBG_LSQ_LOAD_FLG  0 //(1<<1)

namespace SST {
namespace Vanadis {

class VanadisLoadStoreQueue : public SST::SubComponent {

public:
    SST_ELI_REGISTER_SUBCOMPONENT_API(SST::Vanadis::VanadisLoadStoreQueue, int, int)

    SST_ELI_DOCUMENT_PARAMS({ "verbose", "Set the verbosity of output for the LSQ", "0" },
                            { "verbose_mask", "Mask bits for masking output", "-1" },
                            { "debug_ins_addrs", "Comma-separated list of instruction addresses to debug", ""},
                            { "debug_addrs", "Comma-separated list of memory addresses to debug", ""},
                            { "verboseMask", "DEPRECATED - use 'verbose_mask' instead. Mask bits for masking output", "-1" },
                            { "dbgInsAddrs", "DEPRECATED - use 'debug_ins_addrs' instead. Comma-separated list of instruction addresses to debug", ""},
                            { "dbgAddrs", "DEPRECATED - use 'debug_addrs' instead. Comma-separated list of addresses to debug", ""},
            )

    /*
     * Constructor takes two additional parameters
     * coreid - an integer specifying the core ID for the core that loaded this LSQ
     *          - Each vanadis core should have a unique and sequential ID. E.g., for one core the ID=0, for two cores, one has ID=0 and one has ID=1
     * hwthreads - the total number of hardware threads supported by the core. The LSQ will use this to appropriately partition queues and detect
     *              ordering violations
     */
    VanadisLoadStoreQueue(ComponentId_t id, Params& params, uint32_t coreid, uint32_t hwthreads) : SubComponent(id) {

        uint32_t verbosity = params.find<uint32_t>("verbose");
        uint32_t mask = params.find<uint32_t>("verboseMask",-1);
        std::string prefix = "[lsq " + getName() + " !t]: ";
        output_ = new SST::Output(prefix, verbosity, mask, SST::Output::STDOUT);

        #ifdef VANADIS_BUILD_DEBUG
        setDbgInsAddrs( params.find<std::string>("dbgInsAddrs", "") );
        setDbgAddrs( params.find<std::string>("dbgAddrs", "") );
        #endif

        core_id_ = coreid;
        hw_threads_ = hwthreads;
        register_files_ = nullptr;
    }

    virtual ~VanadisLoadStoreQueue() { delete output_; }

    void setRegisterFiles(std::vector<VanadisRegisterFile*>* reg_f) {
        #ifdef VANADIS_BUILD_DEBUG
        output_->verbose(CALL_INFO, 8, 0, "Setting register files (%" PRIu32 " register files in set)\n",
                        (uint32_t)reg_f->size());
        #endif
        assert(reg_f != nullptr);

        register_files_ = reg_f;
    }
    void setCoreId( int core ) { core_id_ = core; }
    int getCoreId( ) { return core_id_; }

    void setHWThreads( int threads ) { hw_threads_ = threads; }
    int getHWThreads( ) { return hw_threads_; }

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
            dbg_ins_addrs_.push_back(  strtol( addr.c_str(), NULL , 16 ) );
        }
    }


    bool isDbgInsAddr( uint64_t addr ) {
        for ( auto& it : dbg_ins_addrs_ ) {
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
            dbg_addrs_.push_back(  strtol( addr.c_str(), NULL , 16 ) );
        }
    }


    bool isDbgAddr( uint64_t addr ) {
        for ( auto& it : dbg_addrs_ ) {
            if ( it == addr ) return true;
        }
        return false;
    }

    std::deque<uint64_t> dbg_ins_addrs_;
    std::deque<uint64_t> dbg_addrs_;
    uint32_t core_id_;
    uint32_t hw_threads_;
    std::vector<VanadisRegisterFile*>* register_files_;
    SST::Output* output_;
};

} // namespace Vanadis
} // namespace SST

#endif
