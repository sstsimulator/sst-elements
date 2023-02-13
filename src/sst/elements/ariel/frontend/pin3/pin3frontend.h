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

#include "arielfrontend.h"
#include "ariel_shmem.h"

namespace SST {
namespace ArielComponent {

#define STRINGIZE(input) #input

struct redirect_info_t {
    std::string stdin_file;
    std::string stdout_file;
    std::string stderr_file;
    int stdoutappend = 0;
    int stderrappend = 0;
} redirect_info;

class Pin3Frontend : public ArielFrontend {
    public:

    /* SST ELI */
    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(Pin3Frontend, "ariel", "frontend.pin", SST_ELI_ELEMENT_VERSION(1,0,0), "Ariel frontend for dynamic tracing using Pin3", SST::ArielComponent::ArielFrontend)

    SST_ELI_DOCUMENT_PARAMS(
        {"verbose", "Verbosity for debugging. Increased numbers for increased verbosity.", "0"},
        {"profilefunctions", "Profile functions for Ariel execution, 0 = none, >0 = enable", "0" },
        {"corecount", "Number of CPU cores to emulate", "1"},
        {"maxcorequeue", "Maximum queue depth per core", "64"},
        {"arieltool", "Path to the Ariel PIN-tool shared library", ""},
        {"launcher", "Specify the launcher to be used for instrumentation, default is path to PIN", STRINGIZE(PINTOOL_EXECUTABLE)},
        {"executable", "Executable to trace", ""},
        {"appstdin", "Specify a file to use for the program's stdin", ""},
        {"appstdout", "Specify a file to use for the program's stdout", ""},
        {"appstdoutappend", "If appstdout is set, set this to 1 to append the file intead of overwriting", "0"},
        {"appstderr", "Specify a file to use for the program's stderr", ""},
        {"appstderrappend", "If appstderr is set, set this to 1 to append the file intead of overwriting", "0"},
        {"launchparamcount", "Number of parameters supplied for the launch tool", "0" },
        {"launchparam%(launchparamcount)d", "Set the parameter to the launcher", "" },
        {"envparamcount", "Number of environment parameters to supply to the Ariel executable, default=-1 (use SST environment)", "-1"},
        {"envparamname%(envparamcount)d", "Sets the environment parameter name", ""},
        {"envparamval%(envparamcount)d", "Sets the environment parameter value", ""},
        {"appargcount", "Number of arguments to the traced executable", "0"},
        {"apparg%(appargcount)d", "Arguments for the traced executable", ""},
        {"arielmode", "Tool interception mode, set to 1 to trace entire program (default), set to 0 to delay tracing until ariel_enable() call., set to 2 to attempt auto-detect", "2"},
        {"arielinterceptcalls", "Toggle intercepting library calls", "0"},
        {"arielstack", "Dump stack on malloc calls (also requires enabling arielinterceptcalls). May increase overhead due to keeping a shadow stack.", "0"},
        {"mallocmapfile", "File with valid 'ariel_malloc_flag' ids", ""},
        {"tracePrefix", "Prefix when tracing is enable", ""},
        {"writepayloadtrace", "Trace write payloads and put real memory contents into the memory system", "0"},
        {"instrument_instructions", "turn on or off instruction instrumentation in fesimple", "1"})

        /* Ariel class */
        Pin3Frontend(ComponentId_t id, Params& params, uint32_t cores, uint32_t qSize, uint32_t memPool);
        ~Pin3Frontend();
        virtual void emergencyShutdown();
        virtual void init(unsigned int phase);
        virtual void setup() {}
        virtual void finish();
        virtual ArielTunnel* getTunnel();

#ifdef HAVE_CUDA
        virtual GpuReturnTunnel* getReturnTunnel();
        virtual GpuDataTunnel* getDataTunnel();
#endif

    private:

        int forkPINChild(const char* app, char** args, std::map<std::string, std::string>& app_env, redirect_info_t redirect_info);

        SST::Output* output;

        pid_t child_pid;

        uint32_t core_count;
        SST::Core::Interprocess::MMAPParent<ArielTunnel>* tunnelmgr;

        ArielTunnel* tunnel;

#ifdef HAVE_CUDA
        SST::Core::Interprocess::MMAPParent<GpuReturnTunnel>* tunnelRmgr;
        SST::Core::Interprocess::MMAPParent<GpuDataTunnel>* tunnelDmgr;

        GpuReturnTunnel* tunnelR;
        GpuDataTunnel* tunnelD;
#endif

        std::string appLauncher;
        redirect_info_t redirect_info;


        char **execute_args;
        std::map<std::string, std::string> execute_env;

};

}
}

#endif
