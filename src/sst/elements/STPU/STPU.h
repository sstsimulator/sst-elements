// Copyright 2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2018, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _STPU_H
#define _STPU_H

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif
#include <inttypes.h>

#include <sst/core/event.h>
#include <sst/core/sst_types.h>
#include <sst/core/component.h>
#include <sst/core/link.h>
#include <sst/core/timeConverter.h>
#include <sst/core/output.h>
#include <sst/core/elementinfo.h>

#include <sst/core/interfaces/simpleMem.h>
#include "../memHierarchy/memEvent.h"
#include "stpu_lib.h"
#include "neuron.h"

namespace SST {
namespace STPUComponent {

class STPU : public SST::Component {
public:
/* Element Library Info */
    SST_ELI_REGISTER_COMPONENT(STPU, "STPU", "STPU", SST_ELI_ELEMENT_VERSION(1,0,0),
            "Spiking Temportal Processing Unit", COMPONENT_CATEGORY_PROCESSOR)

    SST_ELI_DOCUMENT_PARAMS(
            {"verbose",                 "(uint) Determine how verbose the output from the CPU is", "0"},
            {"clock",                   "(string) Clock frequency", "1GHz"},
            {"neurons",                  "(uint) number of neurons", "32"}
                            )

    SST_ELI_DOCUMENT_PORTS( {"mem_link", "Connection to memory", { "memHierarchy.MemEventBase" } } )

    /* Begin class definiton */
    STPU(SST::ComponentId_t id, SST::Params& params);
    void init();
    void finish() {;}

private:
    STPU();  // for serialization only
    STPU(const STPU&); // do not implement
    void operator=(const STPU&); // do not implement
    void init(unsigned int phase);
    
    void handleEvent( SST::Interfaces::SimpleMem::Request * req );
    virtual bool clockTic( SST::Cycle_t );

    Output out;
    Interfaces::SimpleMem * memory;
    uint numNeurons;

    neuron *neurons;

    std::map<uint64_t, SimTime_t> requests;

    TimeConverter *clockTC;
    Clock::HandlerBase *clockHandler;

};

}
}
#endif /* _STPU_H */
