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

// This is a simple program that reads from stdin and writes to the specified output stream.
// Specify -o to write to stdout and -e to write to stderr.
#include <ostream>
#include <cstdlib>
#include <iostream>
#include <string>
#include <sys/poll.h>

// Detect whether we are reading stdin from a file or not
int stdin_is_redirected() {
    struct pollfd fds;
    int ret;
    fds.fd = 0; /* this is STDIN */
    fds.events = POLLIN;
    return poll(&fds, 1, 0);
}

int main (int argc, char** argv)
{
    std::string default_output ("This is the default output of the testio program.\n");
    std::string line;

    // Copy from stdin to which ever outputs the user specified
    // If input is not being read from a file, use the default output.
    int is_redirected = stdin_is_redirected();
    if (is_redirected != 0 and is_redirected != 1) {
        std::cout << "`poll` returned error code: " << is_redirected << std::endl;
        return EXIT_FAILURE;
    }

    // Print to both stdout and stderr
    if (is_redirected) {
         while (std::getline(std::cin, line) && !line.empty()) {
            std::cout << "STDOUT: " << line << std::endl;
            std::cerr << "STDERR: " << line << std::endl;
         }
    }
    else {
        std::cout << "STDOUT: " << default_output;
        std::cerr << "STDERR: " << default_output;
    }

    return EXIT_SUCCESS;
}
