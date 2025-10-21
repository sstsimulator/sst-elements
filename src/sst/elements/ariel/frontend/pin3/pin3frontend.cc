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


#include <cstdlib>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <cstddef>
#include <sst_config.h>

#include "pin3frontend.h"

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

Pin3Frontend::Pin3Frontend(ComponentId_t id, Params& params, uint32_t cores,
    uint32_t maxCoreQueueLen, uint32_t defMemPool) :
    ArielFrontendCommon(id, params, cores, maxCoreQueueLen, defMemPool) {

    // Create output
    verbosemode = params.find<int>("verbose", 0);
    output = new SST::Output("Pin3Frontend[@f:@l:@p] ", verbosemode, 0, SST::Output::STDOUT);

    // Parse parameters that all frontends have
    parseCommonSubComponentParams(params);

    // Parse all other parameters
    parsePin3Params(params);

    output->verbose(CALL_INFO, 1, 0, "Completed processing application arguments.\n");

    // Create Tunnel Manager and set the tunnel
    tunnelmgr = new SST::Core::Interprocess::MMAPParent<ArielTunnel>(id,
        core_count, max_core_queue_len);
    std::string shmem_region_name = tunnelmgr->getRegionName();
    tunnel = tunnelmgr->getTunnel();
    output->verbose(CALL_INFO, 1, 0, "Base pipe name: %s\n", shmem_region_name.c_str());

    // Put together execute_args for fork
    setForkArguments();
    // If mpi, use mpi launcher. Otherwise launch pin
    app_name = (mpimode == 1) ? mpilauncher : applauncher;

    // Remember that the list of arguments must be NULL terminated for execution
    //execute_args[(pin_arg_count - 1) + appargcount] = NULL;

    output->verbose(CALL_INFO, 1, 0, "Completed initialization of the Ariel CPU.\n");
    fflush(stdout);
}

Pin3Frontend::~Pin3Frontend() {
    // Everything loaded by calls to the core are deleted by the core
    // (subcomponents, component extension, etc.)
    delete tunnelmgr;
}

void Pin3Frontend::emergencyShutdown() {
    delete tunnelmgr; // Clean up tmp file
    ArielFrontendCommon::emergencyShutdown();
}

int Pin3Frontend::forkChildProcess(const char* app, char** args,
    std::map<std::string, std::string>& app_env,
    ariel_redirect_info_t redirect_info) {

    // If user only wants to init the simulation then we do NOT fork the binary
    if(isSimulationRunModeInit())
        return 0;

    int next_arg_index = 0;
    int next_line_index = 0;

    char* full_execute_line = (char*) malloc(sizeof(char) * 16384);

    memset(full_execute_line, 0, sizeof(char) * 16384);

    while(NULL != args[next_arg_index]) {
        int copy_char_index = 0;

        if(0 != next_line_index) {
                full_execute_line[next_line_index++] = ' ';
        }

        while('\0' != args[next_arg_index][copy_char_index]) {
                full_execute_line[next_line_index++] = args[next_arg_index][copy_char_index++];
        }

        next_arg_index++;
    }

    full_execute_line[next_line_index] = '\0';

    output->verbose(CALL_INFO, 1, 0, "Executing PIN command: %s\n", full_execute_line);
    free(full_execute_line);

    pid_t the_child;

    // Fork this binary, then exec to get around waiting for
    // child to exit.
    the_child = fork();
    if ( the_child < 0 ) {
        perror("fork");
        output->fatal(CALL_INFO, 1, "Fork failed to launch the traced process. errno = %d, errstr = %s\n", errno, strerror(errno));
    }

    if(the_child != 0) {
        // Set the member variable child_pid in case the waitpid() below fails
        // this allows the fatal process to kill the process and prevent it
        // from becoming a zombie process.  Because as we all know, zombies are
        // bad and eat your brains...
        child_pid = the_child;

        // This is the parent, return the PID of our child process
    /* Wait a second, and check to see that the child actually started */
    sleep(1);
    int pstat;
    pid_t check = waitpid(the_child, &pstat, WNOHANG);
    if ( check > 0 ) {
        // The child process is Stopped or Terminated.
        // Ther are 3 possible results
        if (WIFEXITED(pstat) == true) {
            output->fatal(CALL_INFO, 1,
                    "Launching trace child failed!  Child Exited with status %d\n",
                    WEXITSTATUS(pstat));
        }
        else if (WIFSIGNALED(pstat) == true) {
            output->fatal(CALL_INFO, 1,
                    "Launching trace child failed!  Child Terminated With Signal %d; Core Dump File Created = %d\n",
                    WTERMSIG(pstat), WCOREDUMP(pstat));
        }
        else if (WIFSTOPPED(pstat) == true) {
            output->fatal(CALL_INFO, 1,
                    "Launching trace child failed!  Child Stopped with Signal  %d\n",
                    WSTOPSIG(pstat));
        }
        else {
            output->fatal(CALL_INFO, 1,
                "Launching trace child failed!  Unknown Problem; pstat = %d\n",
                pstat);
        }

    } else if ( check < 0 ) {
        perror("waitpid");
        output->fatal(CALL_INFO, 1,
                "Waitpid returned an error, errno = %d.  Did the child ever even start?\n", errno);
    }
        return (int) the_child;
    } else {
        //Do I/O redirection before exec
        if ("" != redirect_info.stdin_file){
            output->verbose(CALL_INFO, 1, 0, "Redirecting child stdin from file %s\n", redirect_info.stdin_file.c_str());
            if (!freopen(redirect_info.stdin_file.c_str(), "r", stdin)) {
                output->fatal(CALL_INFO, 1, 0, "Failed to redirect stdin\n");
            }
        }
        if ("" != redirect_info.stdout_file){
            output->verbose(CALL_INFO, 1, 0, "Redirecting child stdout to file %s\n", redirect_info.stdout_file.c_str());
            std::string mode = "w+";
            if (redirect_info.stdoutappend) {
                mode = "a+";
            }
            if (!freopen(redirect_info.stdout_file.c_str(), mode.c_str(), stdout)) {
                output->fatal(CALL_INFO, 1, 0, "Failed to redirect stdout\n");
            }
        }
        if ("" != redirect_info.stderr_file){
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

        if(0 == app_env.size()) {
#if defined(SST_COMPILE_MACOSX)
        char *dyldpath = getenv("DYLD_LIBRARY_PATH");

        if(dyldpath) {
            setenv("PIN_APP_DYLD_LIBRARY_PATH", dyldpath, 1);
            setenv("PIN_DYLD_RESTORE_REQUIRED", "t", 1);
            unsetenv("DYLD_LIBRARY_PATH");
        }
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
                "Error executing: %s under a PIN fork\n",
                app);
        } else {
            char** execute_env_cp = (char**) malloc(sizeof(char*) * (app_env.size() + 1));
            uint32_t next_env_cp_index = 0;

            for(auto env_itr = app_env.begin(); env_itr != app_env.end(); env_itr++) {
                size_t nv_pair_size = sizeof(char) * (2 + env_itr->first.size() + env_itr->second.size());
                char* execute_env_nv_pair = (char*) malloc(nv_pair_size);

                output->verbose(CALL_INFO, 2, 0, "Env: %s=%s\n",
                        env_itr->first.c_str(), env_itr->second.c_str());

                snprintf(execute_env_nv_pair, nv_pair_size, "%s=%s", env_itr->first.c_str(),
                        env_itr->second.c_str());

                execute_env_cp[next_env_cp_index] = execute_env_nv_pair;
                next_env_cp_index++;
            }

            execute_env_cp[app_env.size()] = NULL;

            int ret_code = execve(app, args, execute_env_cp);
            perror("execvep");

            output->verbose(CALL_INFO, 1, 0, "Call to execvpe returned %d\n", ret_code);
            output->fatal(CALL_INFO, -1, "Error executing %s under a PIN fork\n", app);
        }
    }

    return 0;
}


void Pin3Frontend::setForkArguments() {
    // Set all the arguments that will be passed to fork in `execute_args`

    // First, allocate it. The number of args is going to be:
    //  - the ones required for mpi
    //  - the ones required for pin
    //  - the ones required for the app
    //
    // MPI: We need one argument for the launcher, one for the number of ranks,
    //      and one for the rank to trace
    uint32_t mpi_arg_count = 0;
    if (mpimode == 1)
        mpi_arg_count = 3;

    // PIN: magic number 37 + the arguments for pin
    const uint32_t pin_arg_count = 37 + launch_param_count;

    // Allocate
    execute_args = (char**) malloc(sizeof(char*) * (mpi_arg_count +
      pin_arg_count + appargcount));


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
    }

    // Pin + pin arguments:
    //    pin -follow_execv [other launch args] -t arieltool
    size_t execute_args_size = sizeof(char) * (applauncher.size() + 2);
    execute_args[arg] = (char*) malloc(execute_args_size);
    snprintf(execute_args[arg], execute_args_size, "%s", applauncher.c_str());
    arg++;

#if 0
    execute_args[arg++] = const_cast<char*>("-pause_tool");
    execute_args[arg++] = const_cast<char*>("15");
#endif

    execute_args[arg++] = const_cast<char*>("-follow_execv");

    for (auto itr = launch_params.begin(); itr != launch_params.end(); itr++) {
        std::string launch_p = (*itr);
        execute_args[arg] = (char*) malloc( sizeof(char) *
            (launch_p.size() + 1) );
        strcpy(execute_args[arg], launch_p.c_str());
        arg++;
    }

    execute_args[arg++] = const_cast<char*>("-t");
    execute_args[arg++] = (char*) malloc(sizeof(char) * (arieltool.size() + 1));
    strcpy(execute_args[arg-1], arieltool.c_str());

    // pintool arguments
    size_t buff8size = sizeof(char)*8;
    execute_args[arg++] = const_cast<char*>("-w");
    execute_args[arg++] = (char*) malloc(buff8size);
    snprintf(execute_args[arg-1], buff8size, "%d", writepayloadtrace);

    execute_args[arg++] = const_cast<char*>("-E");
    execute_args[arg++] = (char*) malloc(buff8size);
    snprintf(execute_args[arg-1], buff8size, "%d", instrument_instructions);

    std::string shmem_region_name = tunnelmgr->getRegionName();
    execute_args[arg++] = const_cast<char*>("-p");
    execute_args[arg++] = (char*) malloc(sizeof(char) * (shmem_region_name.length() + 1));
    strcpy(execute_args[arg-1], shmem_region_name.c_str());

    execute_args[arg++] = const_cast<char*>("-v");
    execute_args[arg++] = (char*) malloc(buff8size);
    snprintf(execute_args[arg-1], buff8size, "%d", verbosemode);

    execute_args[arg++] = const_cast<char*>("-t");
    execute_args[arg++] = (char*) malloc(buff8size);
    snprintf(execute_args[arg-1], buff8size, "%" PRIu32, profilefunctions);

    execute_args[arg++] = const_cast<char*>("-c");
    execute_args[arg++] = (char*) malloc(buff8size);
    snprintf(execute_args[arg-1], buff8size, "%" PRIu32, core_count);

    execute_args[arg++] = const_cast<char*>("-s");
    execute_args[arg++] = (char*) malloc(buff8size);
    snprintf(execute_args[arg-1], buff8size, "%" PRIu32, startup_mode);

    execute_args[arg++] = const_cast<char*>("-m");
    execute_args[arg++] = (char*) malloc(buff8size);
    snprintf(execute_args[arg-1], buff8size, "%" PRIu32, intercept_mem_allocations);
    execute_args[arg++] = const_cast<char*>("-k");
    execute_args[arg++] = (char*) malloc(buff8size);
    snprintf(execute_args[arg-1], buff8size, "%" PRIu32, keep_malloc_stack_trace);

    if (malloc_map_filename != "") {
        execute_args[arg++] = const_cast<char*>("-u");
        execute_args[arg++] = (char*) malloc(sizeof(char) * (malloc_map_filename.size() + 1));
        strcpy(execute_args[arg-1], malloc_map_filename.c_str());
    }

    execute_args[arg++] = const_cast<char*>("-d");
    execute_args[arg++] = (char*) malloc(buff8size);
    snprintf(execute_args[arg-1], buff8size, "%" PRIu32, def_mem_pool);

    execute_args[arg++] = const_cast<char*>("--");

    // Application and application arguments
    execute_args[arg++] = (char*) malloc(sizeof(char) * (executable.size() + 1));
    strcpy(execute_args[arg-1], executable.c_str());

    for (auto itr = app_arguments.begin(); itr != app_arguments.end(); itr++) {
        std::string app_arg = (*itr);
        execute_args[arg] = (char*) malloc( sizeof(char) *
            (app_arg.size() + 1) );
        strcpy(execute_args[arg], app_arg.c_str());
        arg++;
    }

    // The list of arguments must be NULL terminated for execution
    execute_args[arg] = NULL;

}

void Pin3Frontend::parsePin3Params(Params& params) {
    // Parse pin
    applauncher = params.find<std::string>("launcher", PINTOOL_EXECUTABLE);

    // Parse pin parameters
    launch_param_count = (uint32_t) params.find<uint32_t>("launchparamcount", 0);
    size_t param_name_buffer_size = sizeof(char) * 512;
    char* param_name_buffer = (char*) malloc(param_name_buffer_size);

    for(uint32_t aa = 0; aa < launch_param_count; aa++) {
        snprintf(param_name_buffer, param_name_buffer_size, "launchparam%" PRIu32, aa);
        std::string launch_p = params.find<std::string>(param_name_buffer, "");

        if("" == launch_p) {
            output->fatal(CALL_INFO, -1, "Error: launch parameter %" PRId32 " is empty string, this must be set to a value.\n",
                    aa);
        }
        launch_params.push_back(launch_p);
    }

    free(param_name_buffer);

    // Parse pintool
    size_t tool_path_size = sizeof(char) * 1024;
    char* tool_path = (char*) malloc(tool_path_size);
#ifdef SST_COMPILE_MACOSX
    snprintf(tool_path, tool_path_size, "%s/fesimple.dylib", ARIEL_STRINGIZE(ARIEL_TOOL_DIR));
#else
    snprintf(tool_path, tool_path_size, "%s/fesimple.so", ARIEL_STRINGIZE(ARIEL_TOOL_DIR));
#endif
    arieltool = params.find<std::string>("arieltool", tool_path);
    if("" == arieltool) {
        output->fatal(CALL_INFO, -1, "The arieltool parameter specifying which PIN tool to run was not specified\n");
    }
    free(tool_path);

    // Parse parameters to pass to pintool
    if ( params.find<int>("writepayloadtrace") == 0 )
        writepayloadtrace = 0;
    else
        writepayloadtrace = 1;
    instrument_instructions = params.find<int>("instrument_instructions", 1);
    profilefunctions = (uint32_t) params.find<uint32_t>("profilefunctions", 0);
    intercept_mem_allocations = (uint32_t) params.find<uint32_t>("arielinterceptcalls", 0);

    switch(intercept_mem_allocations) {
    case 0:
        output->verbose(CALL_INFO, 1, 0, "Interception and instrumentation of multi-level memory and malloc/free calls is DISABLED.\n");
        break;
    default:
        output->verbose(CALL_INFO, 1, 0, "Interception and instrumentation of multi-level memory and malloc/free calls is ENABLED.\n");
        break;
    }

    keep_malloc_stack_trace = (uint32_t) params.find<uint32_t>("arielstack", 0);
    output->verbose(CALL_INFO, 1, 0, "Tracking the stack and dumping on malloc calls is %s.\n",
            keep_malloc_stack_trace == 1 ? "ENABLED" : "DISABLED");

    malloc_map_filename = params.find<std::string>("mallocmapfile", "");
    if (malloc_map_filename == "") {
        output->verbose(CALL_INFO, 1, 0, "Malloc map file is DISABLED\n");
    } else {
        output->verbose(CALL_INFO, 1, 0, "Malloc map file is ENABLED, using file '%s'\n", malloc_map_filename.c_str());
    }

}
