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

#include <sst_config.h>

#include "arielfrontendcommon.h"

#include <signal.h>
#if !defined(SST_COMPILE_MACOSX)
#include <sys/prctl.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <fcntl.h>

#include <time.h>

#include <string.h>

#define ARIEL_INNER_STRINGIZE(input) #input
#define ARIEL_STRINGIZE(input) ARIEL_INNER_STRINGIZE(input)

using namespace SST::ArielComponent;

ArielFrontendCommon::ArielFrontendCommon(ComponentId_t id, Params& params,
    uint32_t cores, uint32_t maxCoreQueueLen, uint32_t defMemPool) :
    ArielFrontend(id, params, cores, maxCoreQueueLen, defMemPool),
    core_count(cores), max_core_queue_len(maxCoreQueueLen),
    def_mem_pool(defMemPool) {
}

void ArielFrontendCommon::finish() {
    // If the simulation ended early, e.g. by using --stop-at, the child
    // may still be executing. It will become a zombie if we do not
    // kill it.
    if (child_pid != 0) {
        kill(child_pid, SIGTERM);
    }
}

ArielTunnel* ArielFrontendCommon::getTunnel() {
    return tunnel;
}

void ArielFrontendCommon::init(unsigned int phase)
{
    if ( phase == 0 ) {
        output->verbose(CALL_INFO, 1, 0, "Launching...\n");
        // Init the child_pid = 0, this prevents problems in emergencyShutdown()
        // if forkPINChild() calls fatal (i.e. the child_pid would not be set)
        child_pid = 0;
        child_pid = forkChildProcess(app_name.c_str(), execute_args,
            execute_env, redirect_info);
        output->verbose(CALL_INFO, 1, 0, "Waiting for child to attach.\n");
        tunnel->waitForChild();
        output->verbose(CALL_INFO, 1, 0, "Child has attached!\n");
    }
}

void ArielFrontendCommon::emergencyShutdown() {
    // Note: derived classes must clean up allocated memory

    // If child_pid = 0, dont kill (this would kill all processes of the group)
    if (child_pid != 0) {
        kill(child_pid, SIGKILL);
    }
}

void ArielFrontendCommon::parseCommonSubComponentParams(Params& params) {
    // Parse ariel options
    startup_mode = (uint32_t) params.find<uint32_t>("arielmode", 2);

    // Parse executable name
    executable = params.find<std::string>("executable", "");
    if("" == executable) {
        output->fatal(CALL_INFO, -1, "The input deck did not specify an executable to be run\n");
    }

    // Parse redirect info
    redirect_info.stdin_file = params.find<std::string>("appstdin", "");
    redirect_info.stdout_file = params.find<std::string>("appstdout", "");
    redirect_info.stderr_file = params.find<std::string>("appstderr", "");
    redirect_info.stdoutappend = params.find<std::uint32_t>("appstdoutappend", "0");
    redirect_info.stderrappend = params.find<std::uint32_t>("appstderrappend", "0");

    // MPI Launcher options
    mpimode = params.find<int>("mpimode", 0);
    mpilauncher = params.find<std::string>("mpilauncher",  ARIEL_STRINGIZE(MPILAUNCHER_EXECUTABLE));
    mpiranks = params.find<int>("mpiranks", 1);
    mpitracerank = params.find<int>("mpitracerank", 0);

    // MPI Launcher error checking
    if (mpimode == 1) {
        if (mpilauncher.compare("") == 0) {
            output->fatal(CALL_INFO, -1, "mpimode=1 was specified but parameter `mpilauncher` is an empty string");
        }

        if (redirect_info.stdin_file.compare("") != 0 || redirect_info.stdout_file.compare("") != 0 || redirect_info.stderr_file.compare("") != 0)  {
            output->fatal(CALL_INFO, -1, "Using an MPI launcher and redirected I/O is not supported.\n");
        }

        if (mpiranks < 1) {
            output->fatal(CALL_INFO, -1, "You must specify a positive number for `mpiranks` when using an MPI launhcer. Got %d.\n", mpiranks);
        }

        if (mpitracerank < 0 || mpitracerank >= mpiranks) {
            output->fatal(CALL_INFO, -1, "The value of `mpitracerank` must be in [0,mpiranks) Got %d.\n", mpitracerank);
        }

    }

    if (mpimode == 1) {
        output->verbose(CALL_INFO, 1, 0, "Ariel-MPI: MPI launcher: %s\n", mpilauncher.c_str());
        output->verbose(CALL_INFO, 1, 0, "Ariel-MPI: MPI ranks: %d\n", mpiranks);
        output->verbose(CALL_INFO, 1, 0, "Ariel-MPI: MPI trace rank: %d\n", mpitracerank);
    }

    // Parse application information
    appargcount = (uint32_t) params.find<uint32_t>("appargcount", 0);
    output->verbose(CALL_INFO, 1, 0, "Model specifies that there are %" PRIu32 " application arguments\n", appargcount);

    char* argv_buffer = (char*) malloc(sizeof(char) * 256);
    for(uint32_t aa = 0; aa < appargcount ; ++aa) {
        snprintf(argv_buffer, sizeof(char)*256, "apparg%" PRIu32, aa);
        std::string argv_i = params.find<std::string>(argv_buffer, "");

        output->verbose(CALL_INFO, 1, 0, "Found application argument %" PRIu32 " (%s) = %s\n",
                aa, argv_buffer, argv_i.c_str());
        app_arguments.push_back(argv_i);
    }
    free(argv_buffer);

    // Parse environment variables
    const int32_t env_count = params.find<int32_t>("envparamcount", -1);
    char* env_name_buffer = (char*) malloc(sizeof(char) * 256);
    for(int32_t env_param_num = 0; env_param_num < env_count; env_param_num++) {
        snprintf(env_name_buffer, sizeof(char)*256, "envparamname%" PRId32 , env_param_num);

        std::string env_name = params.find<std::string>(env_name_buffer, "");

        if("" == env_name) {
            output->fatal(CALL_INFO, -1, "Parameter: %s environment variable name is empty",
                    env_name_buffer);
        }

        snprintf(env_name_buffer, sizeof(char)*256, "envparamval%" PRId32, env_param_num);

        std::string env_value = params.find<std::string>(env_name_buffer, "");
        execute_env.insert(std::pair<std::string, std::string>(
            env_name, env_value));
    }
    free(env_name_buffer);

}

