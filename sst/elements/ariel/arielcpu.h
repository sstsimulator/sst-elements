// Copyright 2009-2014 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2014, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _H_ARIEL_CPU
#define _H_ARIEL_CPU

#include <sst/core/interfaces/simpleMem.h>
#include <sst/core/component.h>
#include <sst/core/params.h>
#include <sst/core/simulation.h>

#include <stdint.h>

#include <string>

#include "arielmemmgr.h"
#include "arielcore.h"
#include "ariel_shmem.h"

namespace SST {
namespace ArielComponent {

class ArielCPU : public SST::Component {

	public:
		ArielCPU(ComponentId_t id, Params& params);
		~ArielCPU();
        virtual void setup() {}
        virtual void finish();
        virtual bool tick( SST::Cycle_t );
        int forkPINChild(const char* app, char** args);

    private:
        SST::Output* output;
        ArielMemoryManager* memmgr;
        ArielCore** cpu_cores;
        Interfaces::SimpleMem** cpu_to_cache_links;

        uint32_t core_count;
        uint32_t memory_levels;
        uint64_t* page_sizes;
        uint64_t* page_counts;
        char* shmem_region_name;
        ArielTunnel* tunnel;
        bool stopTicking;

};

}
}

#endif
