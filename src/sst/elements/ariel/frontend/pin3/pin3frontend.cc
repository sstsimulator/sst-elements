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

Pin3Frontend::Pin3Frontend(ComponentId_t id, Params& params, uint32_t cores, uint32_t maxCoreQueueLen, uint32_t defMemPool) :
            ArielFrontend(id, params, cores, maxCoreQueueLen, defMemPool) {

    int verbosity = params.find<int>("verbose", 0);
    output = new SST::Output("Pin3Frontend[@f:@l:@p] ", verbosity, 0, SST::Output::STDOUT);

    int instrument_instructions = params.find<int>("instrument_instructions", 1);
    core_count = cores;

    /////////////////////////////////////////////////////////////////////////////////////

    size_t tool_path_size = sizeof(char) * 1024;
    char* tool_path = (char*) malloc(tool_path_size);

#ifdef SST_COMPILE_MACOSX
    snprintf(tool_path, tool_path_size, "%s/fesimple.dylib", ARIEL_STRINGIZE(ARIEL_TOOL_DIR));
#else
    snprintf(tool_path, tool_path_size, "%s/fesimple.so", ARIEL_STRINGIZE(ARIEL_TOOL_DIR));
#endif

    std::string ariel_tool = params.find<std::string>("arieltool", tool_path);
    if("" == ariel_tool) {
        output->fatal(CALL_INFO, -1, "The arieltool parameter specifying which PIN tool to run was not specified\n");
    }

    free(tool_path);

    std::string executable = params.find<std::string>("executable", "");
    if("" == executable) {
        output->fatal(CALL_INFO, -1, "The input deck did not specify an executable to be run against PIN\n");
    }

    redirect_info.stdin_file = params.find<std::string>("appstdin", "");
    redirect_info.stdout_file = params.find<std::string>("appstdout", "");
    redirect_info.stderr_file = params.find<std::string>("appstderr", "");
    redirect_info.stdoutappend = params.find<std::uint32_t>("appstdoutappend", "0");
    redirect_info.stderrappend = params.find<std::uint32_t>("appstderrappend", "0");

    uint32_t app_argc = (uint32_t) params.find<uint32_t>("appargcount", 0);
    output->verbose(CALL_INFO, 1, 0, "Model specifies that there are %" PRIu32 " application arguments\n", app_argc);

    uint32_t pin_startup_mode = (uint32_t) params.find<uint32_t>("arielmode", 2);
    uint32_t intercept_mem_allocations = (uint32_t) params.find<uint32_t>("arielinterceptcalls", 0);

    switch(intercept_mem_allocations) {
    case 0:
        output->verbose(CALL_INFO, 1, 0, "Interception and instrumentation of multi-level memory and malloc/free calls is DISABLED.\n");
        break;
    default:
        output->verbose(CALL_INFO, 1, 0, "Interception and instrumentation of multi-level memory and malloc/free calls is ENABLED.\n");
        break;
    }

    uint32_t keep_malloc_stack_trace = (uint32_t) params.find<uint32_t>("arielstack", 0);
    output->verbose(CALL_INFO, 1, 0, "Tracking the stack and dumping on malloc calls is %s.\n",
            keep_malloc_stack_trace == 1 ? "ENABLED" : "DISABLED");

    std::string malloc_map_filename = params.find<std::string>("mallocmapfile", "");
    if (malloc_map_filename == "") {
        output->verbose(CALL_INFO, 1, 0, "Malloc map file is DISABLED\n");
    } else {
        output->verbose(CALL_INFO, 1, 0, "Malloc map file is ENABLED, using file '%s'\n", malloc_map_filename.c_str());
    }

    tunnelmgr = new SST::Core::Interprocess::MMAPParent<ArielTunnel>(id, core_count, maxCoreQueueLen);

    std::string shmem_region_name = tunnelmgr->getRegionName();
    tunnel = tunnelmgr->getTunnel();
    output->verbose(CALL_INFO, 1, 0, "Base pipe name: %s\n", shmem_region_name.c_str());

    // MPI Launcher options
    mpimode = params.find<int>("mpimode", 0);
    if (mpimode) {
        mpilauncher = params.find<std::string>("mpilauncher",  ARIEL_STRINGIZE(MPILAUNCHER_EXECUTABLE));
        mpiranks = params.find<int>("mpiranks", 1);
        mpitracerank = params.find<int>("mpitracerank", 0);
    }

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


    appLauncher = params.find<std::string>("launcher", PINTOOL_EXECUTABLE);

    const uint32_t launch_param_count = (uint32_t) params.find<uint32_t>("launchparamcount", 0);
    const uint32_t pin_arg_count = 37 + launch_param_count;

    uint32_t mpi_args = 0;
    if (mpimode == 1) {
        // We need one argument for the launcher, one for the number of ranks,
        // and one for the rank to trace
        mpi_args = 3;
    }

    execute_args = (char**) malloc(sizeof(char*) * (mpi_args + pin_arg_count + app_argc));
    uint32_t arg = 0; // Track current arg

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

    const uint32_t profileFunctions = (uint32_t) params.find<uint32_t>("profilefunctions", 0);

    output->verbose(CALL_INFO, 1, 0, "Processing application arguments...\n");

    size_t execute_args_size = sizeof(char) * (appLauncher.size() + 2);
    execute_args[arg] = (char*) malloc(execute_args_size);
    snprintf(execute_args[arg], execute_args_size, "%s", appLauncher.c_str());
    arg++;



#if 0
    execute_args[arg++] = const_cast<char*>("-pause_tool");
    execute_args[arg++] = const_cast<char*>("15");
#endif

    execute_args[arg++] = const_cast<char*>("-follow_execv");

    size_t param_name_buffer_size = sizeof(char) * 512;
    char* param_name_buffer = (char*) malloc(param_name_buffer_size);

    for(uint32_t aa = 0; aa < launch_param_count; aa++) {
        snprintf(param_name_buffer, param_name_buffer_size, "launchparam%" PRIu32, aa);
        std::string launch_p = params.find<std::string>(param_name_buffer, "");

        if("" == launch_p) {
            output->fatal(CALL_INFO, -1, "Error: launch parameter %" PRId32 " is empty string, this must be set to a value.\n",
                    aa);
        }

        execute_args[arg] = (char*) malloc( sizeof(char) * (launch_p.size() + 1) );
        strcpy(execute_args[arg], launch_p.c_str());
        arg++;
    }

    free(param_name_buffer);

    execute_args[arg++] = const_cast<char*>("-t");
    execute_args[arg++] = (char*) malloc(sizeof(char) * (ariel_tool.size() + 1));
    strcpy(execute_args[arg-1], ariel_tool.c_str());
    execute_args[arg++] = const_cast<char*>("-w");

    if( params.find<int>("writepayloadtrace") == 0 ) {
        execute_args[arg++] = const_cast<char*>("0");
    } else {
        execute_args[arg++] = const_cast<char*>("1");
    }

    size_t buff8size = sizeof(char)*8;

    execute_args[arg++] = const_cast<char*>("-E");
    execute_args[arg++] = (char*) malloc(buff8size);
    snprintf(execute_args[arg-1], buff8size, "%d", instrument_instructions);
    execute_args[arg++] = const_cast<char*>("-p");
    execute_args[arg++] = (char*) malloc(sizeof(char) * (shmem_region_name.length() + 1));
    strcpy(execute_args[arg-1], shmem_region_name.c_str());
    execute_args[arg++] = const_cast<char*>("-v");
    execute_args[arg++] = (char*) malloc(buff8size);
    snprintf(execute_args[arg-1], buff8size, "%d", verbosity);
    execute_args[arg++] = const_cast<char*>("-t");
    execute_args[arg++] = (char*) malloc(buff8size);
    snprintf(execute_args[arg-1], buff8size, "%" PRIu32, profileFunctions);
    execute_args[arg++] = const_cast<char*>("-c");
    execute_args[arg++] = (char*) malloc(buff8size);
    snprintf(execute_args[arg-1], buff8size, "%" PRIu32, core_count);
    execute_args[arg++] = const_cast<char*>("-s");
    execute_args[arg++] = (char*) malloc(buff8size);
    snprintf(execute_args[arg-1], buff8size, "%" PRIu32, pin_startup_mode);
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
    snprintf(execute_args[arg-1], buff8size, "%" PRIu32, defMemPool);
    execute_args[arg++] = const_cast<char*>("--");
    execute_args[arg++] = (char*) malloc(sizeof(char) * (executable.size() + 1));
    strcpy(execute_args[arg-1], executable.c_str());
    char* argv_buffer = (char*) malloc(sizeof(char) * 256);
    for(uint32_t aa = 0; aa < app_argc ; ++aa) {
        snprintf(argv_buffer, sizeof(char)*256, "apparg%" PRIu32, aa);
        std::string argv_i = params.find<std::string>(argv_buffer, "");

        output->verbose(CALL_INFO, 1, 0, "Found application argument %" PRIu32 " (%s) = %s\n",
                aa, argv_buffer, argv_i.c_str());
        execute_args[arg] = (char*) malloc(sizeof(char) * (argv_i.size() + 1));
        strcpy(execute_args[arg], argv_i.c_str());
        arg++;
    }
    execute_args[arg] = NULL;
    free(argv_buffer);

    const int32_t pin_env_count = params.find<int32_t>("envparamcount", -1);
    if(pin_env_count > -1) {
        char* env_name_buffer = (char*) malloc(sizeof(char) * 256);

        for(int32_t next_env_param = 0; next_env_param < pin_env_count; next_env_param++) {
                snprintf(env_name_buffer, sizeof(char)*256, "envparamname%" PRId32 , next_env_param);

                std::string env_name = params.find<std::string>(env_name_buffer, "");

                if("" == env_name) {
                    output->fatal(CALL_INFO, -1, "Parameter: %s environment variable name is empty",
                            env_name_buffer);
                }

                snprintf(env_name_buffer, sizeof(char)*256, "envparamval%" PRId32, next_env_param);

                std::string env_value = params.find<std::string>(env_name_buffer, "");

                execute_env.insert(std::pair<std::string, std::string>(
                    env_name, env_value));
        }

        free(env_name_buffer);
    }

    output->verbose(CALL_INFO, 1, 0, "Completed processing application arguments.\n");

    // Remember that the list of arguments must be NULL terminated for execution
    execute_args[(pin_arg_count - 1) + app_argc] = NULL;

    output->verbose(CALL_INFO, 1, 0, "Completed initialization of the Ariel CPU.\n");
    fflush(stdout);
}

void Pin3Frontend::init(unsigned int phase)
{
    if ( phase == 0 ) {
        output->verbose(CALL_INFO, 1, 0, "Launching PIN...\n");
        // Init the child_pid = 0, this prevents problems in emergencyShutdown()
        // if forkPINChild() calls fatal (i.e. the child_pid would not be set)
        child_pid = 0;
        if (mpimode == 1) {
            // Ariel will fork the MPI launcher which will itself fork pin
            child_pid = forkPINChild(mpilauncher.c_str(), execute_args, execute_env, redirect_info);
        } else {
            child_pid = forkPINChild(appLauncher.c_str(), execute_args, execute_env, redirect_info);
        }
        output->verbose(CALL_INFO, 1, 0, "Returned from launching PIN.  Waiting for child to attach.\n");

        tunnel->waitForChild();
        output->verbose(CALL_INFO, 1, 0, "Child has attached!\n");
    }
}

void Pin3Frontend::finish() {
    // If the simulation ended early, e.g. by using --stop-at, the child
    // may still be executing. It will become a zombie if we do not
    // kill it.
    if (child_pid != 0) {
        kill(child_pid, SIGTERM);
    }
}

ArielTunnel* Pin3Frontend::getTunnel() {
    return tunnel;
}

int Pin3Frontend::forkPINChild(const char* app, char** args, std::map<std::string, std::string>& app_env, redirect_info_t redirect_info) {
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

Pin3Frontend::~Pin3Frontend() {
    // Everything loaded by calls to the core are deleted by the core (subcomponents, component extension, etc.)
    delete tunnelmgr;
}

void Pin3Frontend::emergencyShutdown() {
    // If child_pid = 0, dont kill (this would kill all processes of the group)
    if (child_pid != 0) {
        kill(child_pid, SIGKILL);
    }

    delete tunnelmgr; // Clean up tmp file
}
