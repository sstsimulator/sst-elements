// Copyright 2025 EP Analytics
// SPDX-License-Identifier: BSD-3-Clause
//
// Authors: Allyson Cauble-Chantrenne

#include <sst_config.h>

#include "epafrontend.h"

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

EPAFrontend::EPAFrontend(ComponentId_t id, Params& params, uint32_t cores,
    uint32_t maxCoreQueueLen, uint32_t defMemPool) :
    ArielFrontendCommon(id, params, cores, maxCoreQueueLen, defMemPool) {

    // Create output
    verbosemode = params.find<int>("verbose", 0);
    output = new SST::Output("EPAFrontend[@f:@l:@p] ", verbosemode, 0, SST::Output::STDERR);

    // Parse parameters that all frontends have
    parseCommonSubComponentParams(params);

    output->verbose(CALL_INFO, 1, 0, "Completed processing application arguments.\n");

    // Create Tunnel Manager and set the tunnel
    tunnelmgr = new SST::Core::Interprocess::SHMParent<ArielTunnel>(id,
        core_count, maxCoreQueueLen);
    std::string shmem_region_name = tunnelmgr->getRegionName();
    tunnel = tunnelmgr->getTunnel();
    output->verbose(CALL_INFO, 1, 0, "Base pipe name: %s\n", shmem_region_name.c_str());

    // Put together execute_args for fork
    setForkArguments();
    // If mpi, use mpi launcher. Otherwise launch instrumented app
    app_name = (mpimode == 1) ? mpilauncher : executable;

//    output->verbose(CALL_INFO, 1, 0, "Processing application arguments...\n");


    // Remember that the list of arguments must be NULL terminated for execution
    //execute_args[arg] = NULL;

    output->verbose(CALL_INFO, 1, 0, "Completed initialization of the Ariel CPU.\n");
    fflush(stdout);
}

int EPAFrontend::forkChildProcess(const char* app, char** args, std::map<std::string, std::string>& app_env, ariel_redirect_info_t redirect_info) {
    // If user only wants to init the simulation then we do NOT fork the binary
    if(isSimulationRunModeInit())
        return 0;

    int next_arg_index = 0;
    int next_line_index = 0;

    char* full_execute_line = (char*) malloc(sizeof(char) * 16384);

    memset(full_execute_line, 0, sizeof(char) * 16384);
// TODO Why is no command appearing?
    while (NULL != args[next_arg_index]) {
        int copy_char_index = 0;

        if (0 != next_line_index) {
            full_execute_line[next_line_index++] = ' ';
        }

        while ('\0' != args[next_arg_index][copy_char_index]) {
            full_execute_line[next_line_index++] = args[next_arg_index][copy_char_index++];
        }

        next_arg_index++;
    }

    full_execute_line[next_line_index] = '\0';

    output->verbose(CALL_INFO, 1, 0, "Executing command: %s\n", full_execute_line);
    free(full_execute_line);


    // Fork this binary, then exec to get around waiting for
    // child to exit.
    pid_t the_child;
    the_child = fork();

    // If the fork failed
    if (the_child < 0) {
        perror("fork");
        output->fatal(CALL_INFO, 1, "Fork failed to launch the traced process. errno = %d, errstr = %s\n", errno, strerror(errno));
    }

    // If this is the parent, return the PID of our child process
    if(the_child != 0) {
        // Set the member variable child_pid in case the waitpid() below fails
        // this allows the fatal process to kill the process and prevent it
        // from becoming a zombie process.  Because as we all know, zombies are
        // bad and eat your brains...
        child_pid = the_child;

        // Pin frontend waits a second here for the child to start. This does
        // not appear to be necessary so commenting it out.
        // sleep(1);
        int pstat;
        pid_t check = waitpid(the_child, &pstat, WNOHANG);
        // If the child process is Stopped or Terminated
        if (check > 0) {
            // There are 3 possible results
            // If the child exitted
            if (WIFEXITED(pstat) == true) {
                output->fatal(CALL_INFO, 1, "Launching trace child failed!  Child Exited with status %d\n", WEXITSTATUS(pstat));
            // If the child had an error
            } else if (WIFSIGNALED(pstat) == true) {
                output->fatal(CALL_INFO, 1, "Launching trace child failed!  Child Terminated With Signal %d; Core Dump File Created = %d\n", WTERMSIG(pstat), WCOREDUMP(pstat));
            // If the child stopped with an error
            } else if (WIFSTOPPED(pstat) == true) {
                output->fatal(CALL_INFO, 1, "Launching trace child failed!  Child Stopped with Signal  %d\n", WSTOPSIG(pstat));
            // If there was some other error
            } else {
                output->fatal(CALL_INFO, 1, "Launching trace child failed!  Unknown Problem; pstat = %d\n", pstat);
            }
        // If waitpid returned an error
        } else if (check < 0) {
            perror("waitpid");
            output->fatal(CALL_INFO, 1, "Waitpid returned an error, errno = %d.  Did the child ever even start?\n", errno);
        }
        // Return the PID of the child process
        return (int) the_child;

    // If this is the child
    } else {
        std::string shmem_region_name = tunnelmgr->getRegionName();
        std::string mpimode_str = std::to_string(mpimode);
        std::string mpitracerank_str = std::to_string(mpitracerank);
        // Tell the EPA Runtime library to not use its default tool
        setenv("METASIM_CACHE_SIMULATION", "0", 1);
        // Tell the EPA Runtime library to use the Ariel Tool
        setenv("METASIM_ARIEL_FRONTEND", "1", 1);
        // Pass the shared memory location to EPA library
        setenv("METASIM_SST_SHMEM", shmem_region_name.c_str(), 1);
        // Tell the EPA Runtime library if we're using MPI
        setenv("METASIM_SST_USE_MPI", mpimode_str.c_str(), 1);
        // Pass the rank the user wants to trace to the EPA library
        if (mpimode == 1)
            setenv("METASIM_SST_TRACE_RANK", mpitracerank_str.c_str(), 1);
        // If startup_mode (arielmode) is 0, then delay tracing until
        // ariel_enable is called -- cannot autodetect so fail on 2
        if (startup_mode == 2)
            output->fatal(CALL_INFO, 1, "EPA Frontend cannot autodetect arielmode\n");
        else if (startup_mode == 1) // If trace entire thing
            setenv("EPA_SLICER_START_OFF", "0", 1);
        else
            setenv("EPA_SLICER_START_OFF", "1", 1);
        //Do I/O redirection before exec
        if ("" != redirect_info.stdin_file) {
            output->verbose(CALL_INFO, 1, 0, "Redirecting child stdin from file %s\n", redirect_info.stdin_file.c_str());
            if (!freopen(redirect_info.stdin_file.c_str(), "r", stdin)) {
                output->fatal(CALL_INFO, 1, 0, "Failed to redirect stdin\n");
            }
        }
        if ("" != redirect_info.stdout_file) {
            output->verbose(CALL_INFO, 1, 0, "Redirecting child stdout to file %s\n", redirect_info.stdout_file.c_str());
            std::string mode = "w+";
            if (redirect_info.stdoutappend) {
                mode = "a+";
            }
            if (!freopen(redirect_info.stdout_file.c_str(), mode.c_str(), stdout)) {
                output->fatal(CALL_INFO, 1, 0, "Failed to redirect stdout\n");
            }
        }
        if ("" != redirect_info.stderr_file) {
            output->verbose(CALL_INFO, 1, 0, "Redirecting child stderr from file %s\n", redirect_info.stderr_file.c_str());
            std::string mode = "w+";
            if (redirect_info.stderrappend) {
                mode = "a+";
            }
            if (!freopen(redirect_info.stderr_file.c_str(), mode.c_str(), stderr)) {
                output->fatal(CALL_INFO, 1, 0, "Failed to redirect stderr\n");
            }
        }

        output->verbose(CALL_INFO, 1, 0, "Launching executable: %s...\n", app);

#if defined(SST_COMPILE_MACOSX)
#else
#if defined(HAVE_SET_PTRACER)
        prctl(PR_SET_PTRACER, getppid(), 0, 0 ,0);
#endif // End of HAVE_SET_PTRACER
#endif // End SST_COMPILE_MACOSX (else branch)

        int ret_code = execvp(app, args);
        perror("execve");

        output->verbose(CALL_INFO, 1, 0,
            "Call to execvp returned: %d\n", ret_code);

        output->fatal(CALL_INFO, -1,
            "Error executing: %s under an EPA fork\n",
            app);
    }

    return 0;
}

EPAFrontend::~EPAFrontend() {
    // Everything loaded by calls to the core are deleted by the core
    // (subcomponents, component extension, etc.)
    delete tunnelmgr;
}

void EPAFrontend::emergencyShutdown() {
    delete tunnelmgr; // Clean up tmp file
    ArielFrontendCommon::emergencyShutdown();
}

void EPAFrontend::setForkArguments() {
    // Set all the arguments that will be passed to fork in `execute_args`

    // First, allocate it. The number of args is going to be
    //  - the ones required for mpi
    //  - the ones required for the app

    uint32_t mpi_arg_count = 0;
    if (mpimode == 1) {
        // MPI: We need one argument for the launcher, one for the number of
        // ranks, one for the rank to trace, and one for the "--" (required by
        // mpilauncher)
        mpi_arg_count = 4;
    }

    // Allocate
    execute_args = (char**) malloc(sizeof(char*) * (mpi_arg_count
        + appargcount + 3));
    uint32_t arg = 0; // Track current arg

    // Next, set the arguments.
    // Start with MPI
    if (mpimode == 1) {
        // Prepend mpilauncher to execute_args
        output->verbose(CALL_INFO, 1, 0, "Processing mpilauncher arguments...\n");
        std::string mpiranks_str = std::to_string(mpiranks);
        std::string mpitracerank_str = std::to_string(mpitracerank);

        size_t mpilauncher_size = sizeof(char) * (mpilauncher.size() + 2);
        execute_args[arg] = (char*) malloc(mpilauncher_size);
        snprintf(execute_args[arg], mpilauncher_size, "%s", mpilauncher.c_str());
        arg++;

        size_t mpiranks_str_size = sizeof(char) * (mpiranks_str.size() + 2);
        execute_args[arg] = (char*) malloc(mpiranks_str_size);
        snprintf(execute_args[arg], mpiranks_str_size, "%s", mpiranks_str.c_str());
        arg++;

        size_t mpitracerank_str_size = sizeof(char) * (mpitracerank_str.size() + 2);
        execute_args[arg] = (char*) malloc(mpitracerank_str_size);
        snprintf(execute_args[arg], mpitracerank_str_size, "%s", mpitracerank_str.c_str());
        arg++;

        execute_args[arg++] = const_cast<char*>("--");
    }

    // Application and application arguments
    execute_args[arg++] = (char*) malloc(sizeof(char) * (executable.size() + 2));
    strcpy(execute_args[arg-1], executable.c_str());

    for (auto itr = app_arguments.begin(); itr != app_arguments.end(); itr++) {
        std::string app_arg = (*itr);
        execute_args[arg] = (char*) malloc(sizeof(char) *
            (app_arg.size() + 1));
        strcpy(execute_args[arg], app_arg.c_str());
        arg++;
    }

    // The list of arguments must be NULL terminated for execution
    execute_args[arg] = NULL;
}
