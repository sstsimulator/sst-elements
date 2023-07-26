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

#ifndef _H_VANADIS_ROCC_INTERFACE
#define _H_VANADIS_ROCC_INTERFACE

#include <sst/core/output.h>
#include <sst/core/subcomponent.h>

#include "inst/regfile.h"
#include "inst/vrocc.h"

#include <cinttypes>
#include <cstdint>
#include <vector>
#include <queue>

namespace SST {
namespace Vanadis {

class VanadisRoCCInterface : public SST::SubComponent {
public:
    SST_ELI_REGISTER_SUBCOMPONENT_API(SST::Vanadis::VanadisRoCCInterface)

    SST_ELI_DOCUMENT_PARAMS({ "verbose", "Set the verbosity of output for the RoCC Accelerator Interface", "0" }, )

    SST_ELI_DOCUMENT_STATISTICS({ "roccs_issued", "Count number of rocc instructions that are issued", "operations", 1 })

    VanadisRoCCInterface(ComponentId_t id, Params& params) : SubComponent(id) {
        const int32_t verbosity = params.find<int32_t>("verbose", 0);
        const int32_t dbg_mask = params.find<int32_t>("dbg_mask", 0);
        output = new SST::Output("[RoCC @t]: ", verbosity, dbg_mask, SST::Output::STDOUT);
        registerFiles = nullptr;

        stat_rocc_issued = registerStatistic<uint64_t>("roccs_issued", "1");
    }

    virtual ~VanadisRoCCInterface() {delete output;}

    void setRegisterFiles(std::vector<VanadisRegisterFile*>* reg_f) {
        assert(reg_f != nullptr);

        registerFiles = reg_f;
    }

    virtual bool RoCCFull() = 0;
    virtual size_t roccQueueSize() = 0;
    virtual void push(VanadisRoCCInstruction* rocc_me) = 0;
    virtual void tick(uint64_t cycle) = 0;

    virtual void init(unsigned int phase) = 0;

protected:

    class RoCCCommand
    {
        public:
        RoCCCommand(VanadisRoCCInstruction* inst, uint64_t rs1, uint64_t rs2) {
            this->inst = inst;
            this->rs1 = rs1;
            this->rs2 = rs2;
        }
        ~RoCCCommand() {}
        VanadisRoCCInstruction* inst;
        uint64_t rs1;
        uint64_t rs2;
    };

    class RoCCResponse
    {
        public:
        RoCCResponse(uint8_t rd, uint64_t rd_val) {
            this->rd = rd;
            this->rd_val = rd_val;
        }
        ~RoCCResponse() {}
        uint8_t rd;
        uint64_t rd_val;
    };
    
    std::vector<VanadisRegisterFile*>* registerFiles;

    SST::Output* output;

    Statistic<uint64_t>* stat_rocc_issued;
};

} // namespace Vanadis
} // namespace SST

#endif
