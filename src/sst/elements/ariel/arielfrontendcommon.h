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

#ifndef _H_ARIEL_FRONTEND_COMMON
#define _H_ARIEL_FRONTEND_COMMON

#include <sst/core/sst_config.h>
#include <sst/core/component.h>
#include <sst/core/params.h>

#include <stdint.h>
#include <unistd.h>

#include <string>
#include <map>

#include "arielfrontend.h"
#include "ariel_shmem.h"

namespace SST {
namespace ArielComponent {

// Common struct for I/O redirection that can be used by all frontends
typedef struct ariel_redirect_info_t {
    std::string stdin_file;
    std::string stdout_file;
    std::string stderr_file;
    int stdoutappend = 0;
    int stderrappend = 0;
} ariel_redirect_info;

// Common base implementation for Ariel frontends
class ArielFrontendCommon : public ArielFrontend {
    public:

    SST_ELI_REGISTER_SUBCOMPONENT_API(SST::ArielComponent::ArielFrontendCommon)
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
        {"envparamval%(envparamcount)d", "Sets the environment parameter value", ""})

        ArielFrontendCommon(ComponentId_t id, Params& params, uint32_t cores,
            uint32_t qSize, uint32_t memPool);
        ~ArielFrontendCommon() {}

        // Common Functions
        virtual void finish();
        virtual ArielTunnel* getTunnel();
        virtual void init(unsigned int phase);

        // Functions that should be implemented by derived classes
        virtual void emergencyShutdown();

    protected:
        // Common data members
        pid_t child_pid;
        uint32_t core_count;
        uint32_t max_core_queue_len;
        uint32_t def_mem_pool;
        SST::Output* output;
        ArielTunnel* tunnel;

        // SubComponenent parameters
        int verbosemode; // "verbosity"
        uint32_t startup_mode;  // "arielmode"
        std::string executable;
        ariel_redirect_info_t redirect_info;  // "appstd*"
        int mpimode;
        std::string mpilauncher;
        int mpiranks;
        int mpitracerank;
        uint32_t appargcount;
        std::vector<std::string> app_arguments; // "apparg*"

        // arguments and environment for fork
        std::string app_name; // application name (e.g. mpilauncher, pin)
        char **execute_args;
        std::map<std::string, std::string> execute_env;

        // Common Functions
        virtual void parseCommonSubComponentParams(Params& params);

        // Functions that should be impemented by derived classes
        virtual int forkChildProcess(const char* app, char** args,
            std::map<std::string, std::string>& app_env,
            ariel_redirect_info_t redirect_info) = 0;
        virtual void setForkArguments() = 0; // set execute_args

};

} // namespace ArielComponent
} // namespace SST

#endif // _H_ARIEL_FRONTEND_COMMON
