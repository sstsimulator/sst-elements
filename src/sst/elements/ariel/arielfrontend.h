// Copyright 2009-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2021, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _H_ARIEL_FRONTEND
#define _H_ARIEL_FRONTEND

#include <sst/core/sst_config.h>
#include <sst/core/subcomponent.h>

#include <stdint.h>
#include <unistd.h>

#include <string>
#include <map>

#include "ariel_shmem.h"

namespace SST {
namespace ArielComponent {

#define STRINGIZE(input) #input

/** ArielFrontend is a generic interface for
 * sending a dynamic trace into Ariel.
 */
class ArielFrontend : public SST::SubComponent {
public:

    /* SST ELI */
    SST_ELI_REGISTER_SUBCOMPONENT_API(SST::ArielComponent::ArielFrontend, uint32_t, uint32_t, uint32_t)

    /* ArielFrontend class */
    ArielFrontend(ComponentId_t id, Params& params, uint32_t coreCount, uint32_t maxCoreQueueLen, uint32_t defMemPool) : SubComponent(id) { }

    virtual ~ArielFrontend() { };

    virtual ArielTunnel* getTunnel() = 0;

#ifdef HAVE_CUDA
    virtual GpuDataTunnel* getDataTunnel() { return nullptr; }
    virtual GpuReturnTunnel* getReturnTunnel() { return nullptr; }
#endif

    virtual void init(unsigned int phase) = 0;
    virtual void setup() { }
    virtual void finish() { }
    virtual void emergencyShutdown() { }


};

}
}

#endif
