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

#ifndef _H_PIN3_FRONTEND
#define _H_PIN3_FRONTEND

#include <cstdint>
#include <vector>
#include <sst/core/sst_config.h>
#include <sst/core/component.h>
#include <sst/core/params.h>
#include <sst/core/interprocess/mmapparent.h>

#include <stdint.h>
#include <unistd.h>

#include <map>
#include <string>

#include "arielfrontendcommon.h"
#include "ariel_shmem.h"

namespace SST {
namespace ArielComponent {

#define STRINGIZE(input) #input

class Pin3Frontend : public ArielFrontendCommon {
    public:

    /* SST ELI */
    //SST_ELI_REGISTER_SUBCOMPONENT_DERIVED_API(SST::ArielComponent::Pin3Frontend, SST::ArielComponent::ArielFrontendCommon)
    SST_ELI_REGISTER_SUBCOMPONENT(Pin3Frontend, "ariel", "frontend.pin", SST_ELI_ELEMENT_VERSION(1,0,0), "Ariel frontend for dynamic tracing using Pin3", SST::ArielComponent::ArielFrontend)

    SST_ELI_DOCUMENT_PARAMS(
        {"launcher", "Specify the launcher to be used for instrumentation, default is path to PIN", STRINGIZE(PINTOOL_EXECUTABLE)},
        {"launchparamcount", "Number of parameters supplied for the launch tool", "0" },
        {"launchparam%(launchparamcount)d", "Set the parameter to the launcher", "" },
        {"arieltool", "Path to the Ariel PIN-tool shared library", ""},
        {"writepayloadtrace", "Trace write payloads and put real memory contents into the memory system", "0"},
        {"instrument_instructions", "turn on or off instruction instrumentation in fesimple", "1"},
        {"profilefunctions", "Profile functions for Ariel execution, 0 = none, >0 = enable", "0" },
        {"arielinterceptcalls", "Toggle intercepting library calls", "0"},
        {"arielstack", "Dump stack on malloc calls (also requires enabling arielinterceptcalls). May increase overhead due to keeping a shadow stack.", "0"},
        {"mallocmapfile", "File with valid 'ariel_malloc_flag' ids", ""})

        /* Ariel class */
        Pin3Frontend(ComponentId_t id, Params& params, uint32_t cores,
            uint32_t qSize, uint32_t memPool);
        ~Pin3Frontend();

        virtual void emergencyShutdown();

    private:

        SST::Core::Interprocess::MMAPParent<ArielTunnel>* tunnelmgr;

        // SubComponent parameters
        // - pin
        std::string applauncher;  // "launcher"
        uint32_t launch_param_count;
        std::vector<std::string> launch_params;
        // - pintool
        std::string arieltool;
        // - pintool arguments
        int writepayloadtrace;
        int instrument_instructions;
        uint32_t profilefunctions;
        uint32_t intercept_mem_allocations;  // "arielinterceptcalls"
        uint32_t keep_malloc_stack_trace; // "arielstack"
        std::string malloc_map_filename; // "mallocmapfile"

        // Functions that need to be implemented for ArielFrontendCommon
        int forkChildProcess(const char* app, char** args,
            std::map<std::string, std::string>& app_env,
            ariel_redirect_info_t redirect_info);
        virtual void setForkArguments();  // set execute_args

        // Other functions
        virtual void parsePin3Params(Params& params);


};

} // namespace ArielComponent
} // namespace SST

#endif // _H_PIN3_FRONTEND
