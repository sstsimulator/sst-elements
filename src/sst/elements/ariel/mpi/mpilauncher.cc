#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <ostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <algorithm>
#include <array>
#include <cassert>
#include <cerrno>
#include <csignal>
#include <cstring>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <sys/wait.h>

/*
 *  SLURM-specific MPI launcher for Ariel simulations
 *  Ariel forks this process which initiates mpirun
 *
 *  If one rank is found in the MPI allocation, all ranks
 *  will run there. If multiple ranks are found, a single rank
 *  will run on the node with SST, and the remaining
 *  ranks will be distributed on the other nodes.
 */

int pid = 0; // global so we can use it in the signal handler

// Catch SIGTERM so we can try and shut down the child process
void signalHandler(int signum) {
    std::cout << "Caught signal " << signum << ", exiting gracefully." << std::endl;
    if (pid != 0) {
        kill(pid, signum);
    }
    exit(0);
}

int main(int argc, char *argv[]) {
    // PIN Example:  mpilauncher 8 3 /path/to/pin -t fesimple -- ./myapp -i input1 --otherarg input2
    // EPA Example:  mpilauncher 8 3 -- ./myapp.arielinst -i input1 --otherarg input2
    if (argc < 4 || std::string(argv[1]).compare("-H") == 0) {
        std::cout << "Usage: " << argv[0] << " <nprocs> <tracerank> [<pin-binary> [pin args]] -- <program-binary> [program args]\n";
        exit(1);
    }

    // Set up signal handler
    signal(SIGTERM, signalHandler);

    // Get node that SST is running on.
    // All processes will run on the same node.
    std::array<char, 128> buffer;
    gethostname(buffer.data(), 128);
    std::string host = buffer.data();

    // Check inputs
    int procs = atoi(argv[1]);
    int tracerank = atoi(argv[2]);

    if (procs < 1) {
        printf("Error: %s: <nprocs> must be positive\n", argv[0]);
        exit(1);
    }

    if (tracerank < 0 || tracerank >= procs) {
        printf("Error: %s: <trancerank> must be in [0,nprocs)\n", argv[0]);
        exit(1);
    }

    // To make it easier to build the final command, check if we are using
    // pin or an EPA tool.
    bool instrument_with_pin = true;
    if (std::string(argv[3]).compare("--") == 0)
        instrument_with_pin = false;

    // Build two commands -- the pin command (if it exists) which describes how
    // to use pin, and the target command, which describes how to run the
    // actual application.
    //
    // Parse the arguments. Build the pin command first (until we hit the --).
    // Then build the target command. If there is no pin, the pin command will
    // just be "-- "
    std::string pin_cmd = "";
    std::string target_cmd = "";
    bool building_pin = true; // Keep track of which command we are building
    std::string arg;

    for (int i = 3; i < argc; i++) {
        arg = argv[i];
        if (building_pin) {
            pin_cmd += arg;
            pin_cmd += " ";
        } else {
            target_cmd += arg;
            target_cmd += " ";
        }

        if (arg == "--")
            building_pin = false;
    }

    // Build the final MPI Command
    std::stringstream mpi_cmd;
    mpi_cmd << "mpirun --oversubscribe";

    if (instrument_with_pin) {
        // For PIN instrumentation, we need to run the regular binary, but
        // instrument the trace rank with the pintool (fesimple). Do this by
        // starting the ranks before the tracerank, adding the tracerank, and
        // adding the remaining ranks
        // e.g. mpirun -np M ./myapp -i input1 --anotherarg input2 :
        //      -np 1 pin -folow_execv -t fesimple.so [fesimple args] --
        //          ./myapp -i input1 --anotherarg input2 :
        //      -np N-M-1 ./myapp -i input1 --anotherarg input2

        // In order to trace the appropriate rank, determine how many
        // should launch before the traced rank, and how many should launch
        // after
        int ranks_before = tracerank;
        int ranks_after = procs - tracerank - 1;
        if (ranks_after < 0) {
            ranks_after = 0;
        }

        // Add processes before the traced rank
        if (ranks_before > 0) {
            mpi_cmd << " -H " << host
                    << " -np " << ranks_before
                    << " " << target_cmd
                    << " : ";
        }

        // Add the traced process
        mpi_cmd << " -H " << host
                << " -np 1"
                << " " << pin_cmd // Should include "--"
                << " " << target_cmd;

        // Add processes after the traced rank
        if (ranks_after > 0) {
            mpi_cmd << " : -H " << host
                    << " -np " << ranks_after
                    << " " << target_cmd;
        }
    } else {
        // For EPA-based instrumentation, then run the instrumented application
        // just like you would run the original application
        // e.g. mpirun -np N ./myapp.arielinst -i input1 --anotherarg input2
        mpi_cmd << " -H " << host
               << " -np " << procs
               << " " << target_cmd;
    }


    // Execute the command
    // Use execve to make sure that the child processes exits when killed by SST
    // I am lazily assuming that there are no spaces in any of the arguments.
    printf("MPI Command: %s\n", mpi_cmd.str().c_str());

    // Get a mutable copy
    char* cmd_copy = new char[mpi_cmd.str().length() + 1];
    std::strcpy(cmd_copy, mpi_cmd.str().c_str());

    // Calculate an upper bound for the number of arguments
    const int MAX_ARGS = std::strlen(cmd_copy) / 2 + 2;

    // Allocate memory for the pointers
    char** exec_argv = new char*[MAX_ARGS];
    for (int i = 0;i < MAX_ARGS; i++) {
        exec_argv[i] = NULL;
    }

    // Temporary variable to hold each word
    char* word;

    // Counter for the number of words
    int exec_argc = 0;

    // Use strtok to split the string by spaces
    word = std::strtok(cmd_copy, " ");
    while (word != nullptr) {
        // Allocate memory for the word and copy it
        exec_argv[exec_argc] = new char[std::strlen(word) + 1];
        std::strcpy(exec_argv[exec_argc], word);

        // Move to the next word
        word = std::strtok(nullptr, " ");
        exec_argc++;
    }

    assert(exec_argv[exec_argc] == NULL);

    // Forking child process so we can use the parent to kill it if we need to
    pid = fork();
    if (pid == -1) {
        printf("mpilauncher.cc: fork error: %d, %s\n", errno, strerror(errno));
        exit(-1);
    } else if (pid > 1) { // Parent
        int status;
        waitpid(pid, &status, 0);
        if (!WIFEXITED(status)) {
            printf("Warning: mpilauncher.cc: Forked process did not exit normally.\n");
        } if (WEXITSTATUS(status) != 0) {
            printf("Warning: mpilauncher.cc: Forked process has non-zero exit code: %d\n", WEXITSTATUS(status));
        }
        exit(0);
    } else { // Child
        int ret = execvp(exec_argv[0], exec_argv);
        printf("Error: mpilauncher.cc: This should be unreachable. execvp error: %d, %s\n", errno, strerror(errno));
        exit(1);
    }

    // TODO cleanup?
    // Should not arrive here
    return 1;
}
