// Copyright 2009-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2015, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef COMPONENTS_PORTALS_ARGS_H
#define COMPONENTS_PORTALS_ARGS_H

#include <libgen.h>
#include <getopt.h>

// This contains all the arguments needed by both the sdlgen and the
// programatic generator function.  Some of them aren't used for the
// generator function.
struct portals_args {
    int x;
    int y;
    int z;
    int radix;
    int timing_set;
    int noise_runs;
    char* noise_interval;
    char* noise_duration;
    char* msg_rate;
    int latency;
    int  msg_size;
    int chunk_size;
    char* output;
    int help;
    char* application;
    int coalesce;
    int enable_putv;
    int ranks;
    int new_format;
    int self_partition;
};


static void
print_usage(char *argv0)
{
    char *app = basename(argv0);

    fprintf(stderr, "Usage: %s --application=<application> [OPTION]...\n", app);
    fprintf(stderr, "Generate SDL file for triggered CPU/NIC\n\n");
    fprintf(stderr, "Mandatory arguments to long options are mandatory for short options too.\n");
    fprintf(stderr, "  -x, --xdim=COUNT       Size of x dimension (default: 8)\n");
    fprintf(stderr, "  -y, --ydim=COUNT       Size of y dimension (default: 8)\n");
    fprintf(stderr, "  -z, --zdim=COUNT       Size of z dimension (default: 8)\n");
    fprintf(stderr, "  -r, --radix=COUNT      Radix of tree based algorithms (default: 4)\n");
    fprintf(stderr, "  -t, --timing_set=COUNT Timing set to use (default: 2)\n");
    fprintf(stderr, "      --noise_runs=COUNT Number of iterations when noise enabled (default: 0)\n");
    fprintf(stderr, "      --noise_interval=STRING Interval for noise when enabled\n");
    fprintf(stderr, "      --noise_duration=STRING Duration for noise when enabled\n");
    fprintf(stderr, "      --msg_rate=STRING  Message rate\n");
    fprintf(stderr, "      --latency=COUNT   Latency (in ns)\n");
    fprintf(stderr, "      --message_size=SIZE Size in bytes of message\n");
    fprintf(stderr, "      --chunk_size=SIZE Size in bytes of pipeline chunk\n");
    fprintf(stderr, "      --application=STRING Application to run (required)\n");
    fprintf(stderr, "      --coalesce=0/1 Enable/Disable coalescing of portals commands (default: 1/true)\n");
    fprintf(stderr, "      --enable_putv=0/1 Enable/Disable coalescing of portals commands (default: 0/false)\n");
    fprintf(stderr, "      --output=FILENAME  Output should be sent to FILENAME (default: stdout)\n");
    fprintf(stderr, "      --ranks=COUNT If >1, will pre-partition for COUNT ranks (default: 1)\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "NOTE: If noise_runs is non-zero, noise_interval and noise_duration\n");
    fprintf(stderr, "must be specified\n");
}


// #ifndef __cplusplus
// void parse_partals_args(int argc, const char** argv, struct portals_args* args);
// #else
// extern "C" void parse_partals_args(int argc, const char** argv, struct portals_args* args);
// #endif

static struct option longopts[] = {
    { "xdim",              required_argument, NULL, 'x' },
    { "ydim",              required_argument, NULL, 'y' },
    { "zdim",              required_argument, NULL, 'z' },
    { "radix",             required_argument, NULL, 'r' },
    { "timing_set",        required_argument, NULL, 't' },
    { "noise_runs",        required_argument, NULL, 'n' },
    { "noise_interval",    required_argument, NULL, 'i' },
    { "noise_duration",    required_argument, NULL, 'd' },
    { "msg_rate",          required_argument, NULL, 'm' },
    { "latency",           required_argument, NULL, 'l' },
    { "message_size",      required_argument, NULL, 'q' },
    { "chunk_size",        required_argument, NULL, 'w' },
    { "output",            required_argument, NULL, 'o' },
    { "help",              no_argument,       NULL, 'h' },
    { "application",       required_argument, NULL, 'a' },
    { "coalesce",          required_argument, NULL, 'c' },
    { "enable_putv",       required_argument, NULL, 'p' },
    { "ranks",             required_argument, NULL, 'k' },
    { "new_format",        no_argument,       NULL, 'f' },
    { "self_partition",    no_argument,       NULL, 's' },
    { NULL,                0,                 NULL, 0   }
};


void parse_partals_args(int argc, char** argv, struct portals_args* args); // To get rid of gcc warning
void parse_partals_args(int argc, char** argv, struct portals_args* args)
{
    int ch;
    
    args->x = 8;
    args->y = 8;
    args->z = 8;
    args->radix = 4;
    args->timing_set = 2;
    args->noise_runs = 0;
    args->noise_interval = NULL;
    args->noise_duration = NULL;
    args->msg_rate = (char*)"5MHz";
    args->latency = 500;
    args->msg_size = 1 * 1024 * 1024;
    args->chunk_size = 16 * 1024;
    args->help = 0;
    args->application = NULL;
    args->coalesce = 0;
    args->enable_putv = 0;
    args->ranks = 1;
    args->new_format = 0;
    args->self_partition = 0;
    
    while ((ch = getopt_long(argc, argv, "hx:y:z:r:", longopts, NULL)) != -1) {
        switch (ch) {
        case 'x':
            args->x = atoi(optarg);
            break;
        case 'y':
            args->y = atoi(optarg);
            break;
        case 'z':
            args->z = atoi(optarg);
            break;
        case 'r':
            args->radix = atoi(optarg);
            break;
        case 't':
            args->timing_set = atoi(optarg);
            break;
        case 'n':
            args->noise_runs = atoi(optarg);
            break;
        case 'i':
            args->noise_interval = optarg;
            break;
        case 'd':
            args->noise_duration = optarg;
            break;
        case 'l':
            args->latency = atoi(optarg);
            break;
        case 'q':
            args->msg_size = atoi(optarg);
            break;
        case 'w':
            args->chunk_size = atoi(optarg);
            break;
        case 'm':
            args->msg_rate = optarg;
            break;
        case 'o':
            args->output = optarg;
            break;
        case 'h':
	    args->help = 1;
	    break;
        case 'a':
            args->application = optarg;
            break;
        case 'c':
            args->coalesce = atoi(optarg);
            break;
        case 'p':
            args->enable_putv = atoi(optarg);
            break;
        case 'k':
            args->ranks = atoi(optarg);
            break;
        case 'f':
            args->new_format = 1;
            break;
        case 's':
            args->self_partition = 1;
            break;
        default:
	    break;
        }
    }
    argc -= optind;
    argv += optind;

    /* backward compatibility */
    if (argc == 3 || argc == 4) {
        args->x = atoi(argv[0]);
        args->y = atoi(argv[1]);
        args->z = atoi(argv[2]);
        if (argc == 4) args->radix = atoi(argv[3]);
    }

}


#endif // #ifndef COMPONENTS_PORTALS_ARGS_H

