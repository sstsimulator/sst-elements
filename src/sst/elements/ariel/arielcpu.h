// Copyright 2009-2017 Sandia Corporation. Under the terms
// of Contract DE-NA0003525 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2017, Sandia Corporation
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
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
#include <unistd.h>

#include <string>
#include <map>

#include "arielmemmgr.h"
#include "arielcore.h"
#include "ariel_shmem.h"

namespace SST {
namespace ArielComponent {

class ArielCPU : public SST::Component {

	public:
		ArielCPU(ComponentId_t id, Params& params);
		~ArielCPU();
        virtual void emergencyShutdown();
        virtual void init(unsigned int phase);
        virtual void setup() {}
        virtual void finish();
        virtual bool tick( SST::Cycle_t );
        int forkPINChild(const char* app, char** args, std::map<std::string, std::string>& app_env);

    private:
        SST::Output* output;
        ArielMemoryManager* memmgr;
        ArielCore** cpu_cores;
        Interfaces::SimpleMem** cpu_to_cache_links;
        SST::Link **cpu_to_alloc_tracker_links;
        pid_t child_pid;

        uint32_t core_count;
        ArielTunnel* tunnel;
        bool stopTicking;
	std::string appLauncher;
        bool useAllocTracker;

        char **execute_args;
	std::map<std::string, std::string> execute_env;

};

}
}

#endif
