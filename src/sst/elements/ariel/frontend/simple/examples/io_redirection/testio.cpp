// This is a simple program that copies stdin to stdout.
// Specify -e to write to stderr instead of stdout.
#include <iostream>
#include <string>
#include <sys/poll.h>

int stdin_is_redirected() {
    struct pollfd fds;
    int ret;
    fds.fd = 0; /* this is STDIN */
    fds.events = POLLIN;
    ret = poll(&fds, 1, 0);
    if (ret != 0 && ret != 1) {
        std::cerr << "Error" << std::endl;
    }
    return ret;
}

int main (int argc, char** argv)
{
    std::string option_o ("-o"); // Copy stdin to stdout
    std::string option_e ("-e"); // Copy stdin to stderr
    std::string default_output ("This is the default output of the testio program.\n");
    bool out = false, err = false;
    std::string line;

    // Users must specify at least one of stdout and stderr to write to
    if (argc <= 1) {
        std::cerr << "Error: Please specify at least of one of [-o, -e]" << std::endl;
            return EXIT_FAILURE;
    }

    // Parse arguments. Only -o and -e are allowed.
    // -oe and -eo are not allowed for simplicity of implementation
    for (int i = 1; i < argc; i++) {
        if (option_o.compare(argv[i]) == 0) {
            out = true;
        }
        else if (option_e.compare(argv[i]) == 0) {
            err = true;
        }
        else {
            std::cerr << "Unrecognized argument: " << argv[i] << std::endl;
            return EXIT_FAILURE;
        }
    }

    // Copy from stdin to which ever outputs the user specified
    // If input is not being read from a file, use the default output.
    if (stdin_is_redirected()) {
        if (out && err) {
            while (std::getline(std::cin, line) && !line.empty()) {
                std::cout << line << std::endl;
                std::cerr << line << std::endl;
            }
        }
        else if (out) {
            while (std::getline(std::cin, line) && !line.empty()) {
                std::cout << line << std::endl;
            }
        }
        else if (err) {
            while (std::getline(std::cin, line) && !line.empty()) {
                std::cerr << line << std::endl;
            }
        }
    }
    else {
        if (out && err) {
            std::cout << default_output;
            std::cerr << default_output;
        }
        else if (out) {
            std::cout << default_output;
        }
        else if (err) {
            std::cerr << default_output;
        }

    }

    return EXIT_SUCCESS;
}
