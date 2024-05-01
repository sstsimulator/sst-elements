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

#include <sst/core/sst_config.h>
#include <sst/core/component.h>
#include <sst/core/params.h>
#include <sst/core/interprocess/mmapparent.h>

#include <stdint.h>
#include <unistd.h>

#include <string>
#include <map>

#include "arielfrontendcommon.h"
#include "ariel_shmem.h"

namespace SST {
namespace ArielComponent {

#define STRINGIZE(input) #input

class Pin3Frontend : public ArielFrontendCommon {
    public:

    /* SST ELI */
    SST_ELI_REGISTER_SUBCOMPONENT(Pin3Frontend, "ariel", "frontend.pin", SST_ELI_ELEMENT_VERSION(1,0,0), "Ariel frontend for dynamic tracing using Pin3", SST::ArielComponent::ArielFrontend)

    SST_ELI_DOCUMENT_PARAMS(
        {"verbose", "Verbosity for debugging. Increased numbers for increased verbosity.", "0"},
        {"corecount", "Number of CPU cores to emulate", "1"},
        {"maxcorequeue", "Maximum queue depth per core", "64"},
        {"arielmode", "Tool interception mode, set to 1 to trace entire program (default), set to 0 to delay tracing until ariel_enable() call., set to 2 to attempt auto-detect", "2"},
        {"executable", "Executable to trace", ""},
        {"appstdin", "Specify a file to use for the program's stdin", ""},
        {"appstdout", "Specify a file to use for the program's stdout", ""},
        {"appstdoutappend", "If appstdout is set, set this to 1 to append the file intead of overwriting", "0"},
        {"appstderr", "Specify a file to use for the program's stderr", ""},
        {"appstderrappend", "If appstderr is set, set this to 1 to append the file intead of overwriting", "0"},
        {"mpimode", "Whether to use <mpilauncher> to to launch <launcher> in order to trace MPI-enabled applications.", "0"},
        {"mpilauncher", "Specify a launcher to be used for MPI executables in conjuction with <launcher>", STRINGIZE(MPILAUNCHER_EXECUTABLE)},
        {"mpiranks", "Number of ranks to be launched by <mpilauncher>. Only <mpitracerank> will be traced by <launcher>.", "1" },
        {"mpitracerank", "Rank to be traced by <launcher>.", "0" },
        {"appargcount", "Number of arguments to the traced executable", "0"},
        {"apparg%(appargcount)d", "Arguments for the traced executable", ""},
        {"envparamcount", "Number of environment parameters to supply to the Ariel executable, default=-1 (use SST environment)", "-1"},
        {"envparamname%(envparamcount)d", "Sets the environment parameter name", ""},
        {"envparamval%(envparamcount)d", "Sets the environment parameter value", ""},
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
