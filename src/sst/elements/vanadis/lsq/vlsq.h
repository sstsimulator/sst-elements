// Copyright 2009-2022 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2022, NTESS
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
        output = new SST::Output("[lsq]: ", verbosity, 0, SST::Output::STDOUT);

        address_mask = params.find<uint64_t>("address_mask", 0xFFFFFFFFFFFFFFFF);

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
